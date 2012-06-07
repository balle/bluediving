/*
 *
 *  Bluetooth Generic Object Exchange Profile
 *
 *  Copyright (C) 2003  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation;
 *
 *  Software distributed under the License is distributed on an "AS
 *  IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 *  implied. See the License for the specific language governing
 *  rights and limitations under the License.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <time.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>

#include <openobex/obex.h>

#include "goep.h"


#define OBEX_TIMEOUT  1


/* Connection states */
enum {
	OBEX_CONNECTED = 1,	/* Equal to TCP_ESTABLISHED */
	OBEX_OPEN,
	OBEX_BOUND,
	OBEX_LISTEN,
	OBEX_CONNECT,
	OBEX_CONNECT2,
	OBEX_CONFIG,
	OBEX_DISCONN,
	OBEX_CLOSED
};

/* Request modes */
enum {
	OBEX_IDLE = 0,
	OBEX_DONE,
	OBEX_ERROR,
	OBEX_REQUEST
};


struct obex_context {
	unsigned long state;	/* Connection state */
	uint32_t cid;		/* Connection identifier */

	unsigned long mode;	/* Request mode */
	int error;
	time_t time;

	uint8_t command;
	uint8_t response;

	unsigned char *data_buf;
	uint32_t data_len;

	uint32_t hdr_length;
	struct tm hdr_time;
};


#define OBEX_MAX_HANDLES  1

static obex_t *obex_handles[OBEX_MAX_HANDLES] = { NULL };


static unsigned char target_ftseq[] = { 0xF9, 0xEC, 0x7B, 0xC4, 0x95, 0x3C, 0x11, 0xD2,
					0x98, 0x4E, 0x52, 0x54, 0x00, 0xDC, 0x9E, 0x09 };


#if 0
static int obex_error(uint8_t code)
{
	switch (code) {
	case OBEX_RSP_SUCCESS:
		return 0;
	case OBEX_RSP_NOT_FOUND:
		return ENOENT;
	case OBEX_RSP_FORBIDDEN:
		return EPERM;
	case OBEX_RSP_NOT_IMPLEMENTED:
		return ENOSYS;
	case OBEX_RSP_INTERNAL_SERVER_ERROR:
		return EINTR;
	default:
		return EIO;
	}
}
#endif

#if 0
#define OBEX_RSP_CONTINUE               0x10
#define OBEX_RSP_SWITCH_PRO             0x11
#define OBEX_RSP_CREATED                0x21
#define OBEX_RSP_ACCEPTED               0x22
#define OBEX_RSP_NO_CONTENT             0x24
#define OBEX_RSP_BAD_REQUEST            0x40
#define OBEX_RSP_UNAUTHORIZED           0x41
#define OBEX_RSP_PAYMENT_REQUIRED       0x42
#define OBEX_RSP_FORBIDDEN              0x43
#define OBEX_RSP_METHOD_NOT_ALLOWED     0x45
#define OBEX_RSP_CONFLICT               0x49
#define OBEX_RSP_DATABASE_FULL          0x60
#define OBEX_RSP_DATABASE_LOCKED        0x61
#endif

static void obex_request_done(obex_t *handle, obex_object_t *object, int command, int response)
{
	struct obex_context *context;

	obex_headerdata_t hd;
	uint32_t hl;
	uint8_t hi;
	int secs;

	if (!(context = OBEX_GetUserData(handle)))
		return;

	switch (command) {
	case OBEX_CMD_CONNECT:
		//printf("Cmd: Connect\n");

		if (response != OBEX_RSP_SUCCESS)
			break;

		while (OBEX_ObjectGetNextHeader(handle, object, &hi, &hd, &hl)) {
			switch (hi) {
			case OBEX_HDR_CONNECTION:
				context->cid = hd.bq4;
				break;
			case OBEX_HDR_WHO:
				break;
			}
		}

		context->state = OBEX_CONNECTED;
		break;

	case OBEX_CMD_DISCONNECT:
		//printf("Cmd: Disconnect\n");

		context->state = OBEX_CLOSED;
		break;

	case OBEX_CMD_PUT:
		//printf("Cmd: Put\n");
		//printf("Rsp: %02x\n", response);

		context->response = response;

		if (response != OBEX_RSP_SUCCESS) {
			context->mode = OBEX_ERROR;
			break;
		}

		context->mode = OBEX_DONE;
		break;

	case OBEX_CMD_GET:
		//printf("Cmd: Get\n");
		//printf("Rsp: %02x\n", response);

		context->response = response;

		if (response != OBEX_RSP_SUCCESS) {
			context->mode = OBEX_ERROR;
			break;
		}

		while (OBEX_ObjectGetNextHeader(handle, object, &hi, &hd, &hl)) {
			switch (hi) {
			case OBEX_HDR_NAME:
				//printf("OBEX Header Name: %s\n", hd.bs);
				break;
			case OBEX_HDR_CONNECTION:
				break;
			case OBEX_HDR_LENGTH:
				context->hdr_length = hd.bq4;
				break;
			case OBEX_HDR_TIME:
				atotm((char *) hd.bs, &context->hdr_time);
				break;
			case OBEX_HDR_BODY:
				secs = time(0) - context->time;
				context->data_buf = malloc(hl + 1);
				context->data_len = hl;
				memset(context->data_buf, 0, hl + 1);
				memcpy(context->data_buf, hd.bs, hl);
				break;
			case OBEX_HDR_TYPE:
				/* FIXME: interpret mime type */
				break;
			default:
				printf("Hdr: Unknown header %02x\n", hi);
				break;
			}
		}

		context->mode = OBEX_DONE;
		break;

	case OBEX_CMD_SETPATH:
		//printf("Cmd: Setpath\n");

		if (response != OBEX_RSP_SUCCESS) {
			context->mode = OBEX_ERROR;
			break;
		}

		while (OBEX_ObjectGetNextHeader(handle, object, &hi, &hd, &hl)) {
			switch (hi) {
			case OBEX_HDR_CONNECTION:
				break;
			}
		}

		context->mode = OBEX_DONE;
		break;

	default:
		break;
	}
}

static void obex_event(obex_t *handle, obex_object_t *object, int mode, int event, int command, int response)
{
	switch (event) {
	case OBEX_EV_PROGRESS:
		//printf("Evt: Progress\n");
		break;

	case OBEX_EV_ABORT:
		printf("Evt: Abort\n");
		break;

	case OBEX_EV_REQDONE:
		//printf("Evt: Request done\n");
		obex_request_done(handle, object, command, response);
		break;

	case OBEX_EV_REQHINT:
		printf("Evt: Request hint\n");
		/* Accept any command. Not really good :) */
		OBEX_ObjectSetRsp(object, OBEX_RSP_CONTINUE, OBEX_RSP_SUCCESS);
		break;

	case OBEX_EV_REQ:
		printf("Evt: Request\n");
		break;

	case OBEX_EV_LINKERR:
		printf("Evt: Link error\n");
		OBEX_TransportDisconnect(handle);
		break;

	case OBEX_EV_PARSEERR:
		printf("Evt: Parse error\n");
		OBEX_TransportDisconnect(handle);
		break;

	case OBEX_EV_STREAMEMPTY:
		printf("Evt: Stream empty\n");
		break;

	default:
		printf("Evt: Unknown event %02x\n", event);
		break;
	}
}

int obex_put(int od, const char *type, const char *name, unsigned char *data, size_t size)
{
	obex_t *handle = obex_handles[0];
	struct obex_context *context;
	char *unicode;
	int err, len, ulen;

	obex_object_t *object;
	obex_headerdata_t hd;

	if (!(context = OBEX_GetUserData(handle)))
		return -1;

	if (context->state != OBEX_CONNECTED)
		return -1;

	if (context->mode != OBEX_IDLE)
		return -1;

	context->command = OBEX_CMD_PUT;
	context->response = 0;

	//context->data_buf = data;
	//context->data_len = size;

	if (!(object = OBEX_ObjectNew(handle, OBEX_CMD_PUT)))
		return -1;

	if (context->cid > 0) {
		hd.bq4 = context->cid;
		OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_CONNECTION, hd, 4, OBEX_FL_FIT_ONE_PACKET);
	}

	if (type) {
		hd.bs = (uint8_t *) type;
		OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_TYPE, hd, strlen(type), OBEX_FL_FIT_ONE_PACKET);
	}

	if (name) {
		ulen = (strlen(name) + 1) * 2;
		unicode = malloc(ulen);
		if (!unicode) {
			OBEX_ObjectDelete(handle, object);
			return -1;
		}

		len = OBEX_CharToUnicode((uint8_t *) unicode, (uint8_t *) name, ulen);
		hd.bs = (uint8_t *) unicode;
		OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_NAME, hd, len, OBEX_FL_FIT_ONE_PACKET);

		free(unicode);
	}

	if (size) {
		hd.bq4 = size;
		OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_LENGTH, hd, 4, 0/*OBEX_FL_FIT_ONE_PACKET*/);

		hd.bs = data;
		OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_BODY, hd, size, 0/*OBEX_FL_FIT_ONE_PACKET*/);
	}

	if ((err = OBEX_Request(handle, object)) < 0)
		return err;

	context->time = time(0);

	while (1) {
		OBEX_HandleInput(handle, OBEX_TIMEOUT);
		if (context->response)
			break;
	}

	//printf("Rsp: %02x\n", context->response);
	//printf("Err: %s\n", strerror(obex_error(context->response)));

	if (context->response != OBEX_RSP_SUCCESS)
		err = -1;

	context->mode = OBEX_IDLE;

	return err;
}

int obex_get(int od, const char *type, const char *name, unsigned char **data, size_t *size)
{
	obex_t *handle = obex_handles[0];
	struct obex_context *context;
	char *unicode;
	int err, len, ulen;

	obex_object_t *object;
	obex_headerdata_t hd;

	if (!(context = OBEX_GetUserData(handle)))
		return -1;

	if (context->state != OBEX_CONNECTED)
		return -1;

	if (context->mode != OBEX_IDLE)
		return -1;

	context->command = OBEX_CMD_GET;
	context->response = 0;

	if (!(object = OBEX_ObjectNew(handle, OBEX_CMD_GET)))
		return -1;

	if (context->cid > 0) {
		hd.bq4 = context->cid;
		OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_CONNECTION, hd, 4, OBEX_FL_FIT_ONE_PACKET);
	}

	if (type) {
		hd.bs = (uint8_t *) type;
		OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_TYPE, hd, strlen(type) + 1, OBEX_FL_FIT_ONE_PACKET);
	}

	if (name) {
		ulen = (strlen(name) + 1) * 2;
		unicode = malloc(ulen);
		if (!unicode) {
			OBEX_ObjectDelete(handle, object);
			return -1;
		}

		len = OBEX_CharToUnicode((uint8_t *) unicode, (uint8_t *) name, ulen);
		hd.bs = (uint8_t *) unicode;
		OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_NAME, hd, len, OBEX_FL_FIT_ONE_PACKET);

		free(unicode);
	}

	if ((err = OBEX_Request(handle, object)) < 0)
		return err;

	context->time = time(0);

	while (1) {
		OBEX_HandleInput(handle, OBEX_TIMEOUT);
		if (context->response)
			break;
	}

	//printf("Rsp: %02x\n", context->response);
	//printf("Err: %s\n", strerror(obex_error(context->response)));

	if (context->response == OBEX_RSP_SUCCESS) {
		*data = context->data_buf;
		*size = context->data_len;
	} else {
		free(context->data_buf);
		err = -1;
	}

	context->mode = OBEX_IDLE;

	return err;
}

int obex_setpath(int od, const char *path, uint8_t flags)
{
	unsigned char taildata[2] = { 0x00, 0x00 };
	obex_t *handle = obex_handles[0];
	struct obex_context *context;
	char unicode[200];
	int err, size;

	obex_object_t *object;
	obex_headerdata_t hd;

	if (!(context = OBEX_GetUserData(handle)))
		return -1;

	if (context->state != OBEX_CONNECTED)
		return -1;

	if (context->mode != OBEX_IDLE)
		return -1;

	context->mode = OBEX_REQUEST;

	if (!(object = OBEX_ObjectNew(handle, OBEX_CMD_SETPATH)))
		return -1;

	if (context->cid > 0) {
		hd.bq4 = context->cid;
		OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_CONNECTION, hd, 4, OBEX_FL_FIT_ONE_PACKET);
	}

	if (path) {
		size = OBEX_CharToUnicode((uint8_t *) unicode, (uint8_t *) path, sizeof(unicode));
		hd.bs = (uint8_t *) unicode;
		OBEX_ObjectAddHeader(handle, object,
			OBEX_HDR_NAME, hd, size, OBEX_FL_FIT_ONE_PACKET);
	}

	taildata[0] = flags;
	OBEX_ObjectSetNonHdrData(object, taildata, sizeof(taildata));

	if ((err = OBEX_Request(handle, object)) < 0)
		return err;

	while (1) {
		OBEX_HandleInput(handle, OBEX_TIMEOUT);

		if (context->mode == OBEX_ERROR) {
			err = -EIO;
			break;
		}

		if (context->mode == OBEX_DONE)
			break;
	}

	context->mode = OBEX_IDLE;

	return err;
}

int obex_connect(int od, int mode)
{
	obex_t *handle = obex_handles[0];
	struct obex_context *context;
	int err;

	obex_object_t *object;
	obex_headerdata_t hd;

	if (!(context = OBEX_GetUserData(handle)))
		return -1;

	if (!(object = OBEX_ObjectNew(handle, OBEX_CMD_CONNECT)))
		return -1;

	if (mode == OBEX_FILE_TRANSFER) {
		hd.bs = target_ftseq;
		if (OBEX_ObjectAddHeader(handle, object,
				OBEX_HDR_TARGET, hd, 16, OBEX_FL_FIT_ONE_PACKET) < 0) {
			OBEX_ObjectDelete(handle, object);
			return -1;
		}
	} else
		context->cid = 0;

	if ((err = OBEX_Request(handle, object)) < 0)
		return err;

	while (1) {
		OBEX_HandleInput(handle, OBEX_TIMEOUT);

		if (context->state == OBEX_CLOSED) {
			err = -ENODEV;
			break;
		}

		if (context->state == OBEX_CONNECTED)
			break;
	}

	return err;
}

int obex_disconnect(int od)
{
	obex_t *handle = obex_handles[0];
	struct obex_context *context;
	int err;

	obex_object_t *object;

	if (!(context = OBEX_GetUserData(handle)))
		return -1;

	if (!(object = OBEX_ObjectNew(handle, OBEX_CMD_DISCONNECT)))
		return -1;

	context->state = OBEX_DISCONN;

	if ((err = OBEX_Request(handle, object)) < 0)
		return err;

	while (1) {
		OBEX_HandleInput(handle, OBEX_TIMEOUT);

		if (context->state == OBEX_CLOSED)
			break;
	}

	return err;
}

int obex_open(bdaddr_t *src, bdaddr_t *dst, uint8_t channel)
{
	obex_t *handle;
	struct obex_context *context;

	if (!(context = malloc(sizeof(struct obex_context))))
		return -1;

	if (!(handle = OBEX_Init(OBEX_TRANS_BLUETOOTH, obex_event, 0)))
		return -1;

	context->state = OBEX_OPEN;
	context->mode = OBEX_IDLE;

	OBEX_SetUserData(handle, context);

	OBEX_SetTransportMTU(handle, OBEX_MAXIMUM_MTU, OBEX_MAXIMUM_MTU);

	if (BtOBEX_TransportConnect(handle, src, dst, channel) < 0)
		return -1;

	obex_handles[0] = handle;

	return 0;
}

void obex_close(int od)
{
	obex_t *handle = obex_handles[0];
	struct obex_context *context;

	context = OBEX_GetUserData(handle);

	if (context->state == OBEX_CONNECTED)
		obex_disconnect(od);

	OBEX_SetUserData(handle, NULL);

	free(context);

	obex_handles[0] = NULL;

	OBEX_Cleanup(handle);
}

struct tm *atotm(const char *nptr, struct tm *tm)
{
	if (sscanf(nptr, "%4d%2d%2dT%2d%2d%2d",
			&tm->tm_year, &tm->tm_mon, &tm->tm_mday,
			&tm->tm_hour, &tm->tm_min, &tm->tm_sec) == 6) {
		tm->tm_year -= 1900;
		tm->tm_mon--;
	}
	tm->tm_isdst = 0;

	return tm;
}

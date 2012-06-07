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
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include "goep.h"


int sdp_get_rfcomm_channel(bdaddr_t *src, bdaddr_t *dst, uint16_t service, uint8_t *channel)
{
	sdp_session_t *s;
	sdp_list_t *srch, *attrs, *rsp;
	uuid_t svclass;
	uint16_t attr;
	int err;

	if (!(s = sdp_connect(src, dst, 0)))
		return -1;

	sdp_uuid16_create(&svclass, service);
	srch = sdp_list_append(NULL, &svclass);

	attr = SDP_ATTR_PROTO_DESC_LIST;
	attrs = sdp_list_append(NULL, &attr);

	err = sdp_service_search_attr_req(s, srch, SDP_ATTR_REQ_INDIVIDUAL, attrs, &rsp);

	sdp_close(s);
	
	if (err)
		return 0;

	for (; rsp; rsp = rsp->next) {
		sdp_record_t *rec = (sdp_record_t *) rsp->data;
		sdp_list_t *protos;

		if (!sdp_get_access_protos(rec, &protos)) {
			uint8_t ch = sdp_get_proto_port(protos, RFCOMM_UUID);
			if (ch > 0) {
				*channel = ch;
				return 1;
			}
		}
	}

	return 0;
}

int sdp_get_opp_channel(bdaddr_t *src, bdaddr_t *dst, uint8_t *channel)
{
	return sdp_get_rfcomm_channel(src, dst, OBEX_OBJPUSH_SVCLASS_ID, channel);
}

int sdp_get_ftp_channel(bdaddr_t *src, bdaddr_t *dst, uint8_t *channel)
{
	return sdp_get_rfcomm_channel(src, dst, OBEX_FILETRANS_SVCLASS_ID, channel);
}

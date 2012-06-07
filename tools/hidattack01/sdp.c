/*
 *  sdp HID keyboard
 *
 *  (c) Collin R. Mulliner <collin@betaversion.net>
 *  http://www.mulliner.org/bluetooth/
 *
 *  License: GPLv2
 *
 *  Parts taken from BlueZ(.org)
 * 
 */

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <netinet/in.h>
#include <netdb.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <bluetooth/rfcomm.h>
#include <openobex/obex.h>

sdp_record_t *sdp_record;
sdp_session_t *sdp_session;

/*
 *  100% taken from bluez-utils (sdptool)
 */
static void add_lang_attr(sdp_record_t *r)
{
	sdp_lang_attr_t base_lang;
	sdp_list_t *langs = 0;

	/* UTF-8 MIBenum (http://www.iana.org/assignments/character-sets) */
	base_lang.code_ISO639 = (0x65 << 8) | 0x6e;
	base_lang.encoding = 106;
	base_lang.base_offset = SDP_PRIMARY_LANG_BASE;
	langs = sdp_list_append(0, &base_lang);
	sdp_set_lang_attr(r, langs);
	sdp_list_free(langs, 0);
}

/*
 *  100% taken from bluez-utils (sdptool)
 */
/*
static sdp_data_t *access_proto_to_dataseq(sdp_record_t *rec, sdp_list_t *proto)
{
	sdp_data_t *seq = NULL;
	void *dtds[10], *values[10];
	void **seqDTDs, **seqs;
	int i, seqlen;
	sdp_list_t *p;

	seqlen = sdp_list_len(proto);
	seqDTDs = (void **)malloc(seqlen * sizeof(void *));
	seqs = (void **)malloc(seqlen * sizeof(void *));
	for (i = 0, p = proto; p; p = p->next, i++) {
		sdp_list_t *elt = (sdp_list_t *)p->data;
		sdp_data_t *s;
		int pslen = 0;
		for (; elt && pslen < sizeof(dtds); elt = elt->next, pslen++) {
			sdp_data_t *d = (sdp_data_t *)elt->data;
			dtds[pslen] = &d->dtd;
			switch (d->dtd) {
			case SDP_UUID16:
				values[pslen] = &((uuid_t *)d)->value.uuid16;
				break;
			case SDP_UUID32:
				values[pslen] = &((uuid_t *)d)->value.uuid32;
				break;
			case SDP_UUID128:
				values[pslen] = &((uuid_t *)d)->value.uuid128;
				break;
			case SDP_UINT8:
				values[pslen] = &d->val.uint8;
				break;
			case SDP_UINT16:
				values[pslen] = &d->val.uint16;
				break;
			case SDP_SEQ8:
			case SDP_SEQ16:
			case SDP_SEQ32:
				values[pslen] = d;
				break;
			// FIXME: more
			}
		}
		s = sdp_seq_alloc(dtds, values, pslen);
		if (s) {
			seqDTDs[i] = &s->dtd;
			seqs[i] = s;
		}
	}
	seq = sdp_seq_alloc(seqDTDs, seqs, seqlen);
	free(seqDTDs);
	free(seqs);
	return seq;
}
*/

/*
 *  add additional access protos
 */
/*
static int sdp_set_add_access_protos(sdp_record_t *rec, const sdp_list_t *ap)
{
	const sdp_list_t *p;
	sdp_data_t *protos = 0;
	sdp_data_t *protosg = 0;

	for (p = ap; p; p = p->next) {
		sdp_data_t *seq = access_proto_to_dataseq(rec, (sdp_list_t *)p->data);
		protos = sdp_seq_append(protos, seq);
	}
	protosg = sdp_data_alloc(SDP_SEQ8, protos);	
	sdp_attr_add(rec, SDP_ATTR_ADD_PROTO_DESC_LIST, protosg);
	return 0;
}
*/

/*
 *  add keyboard descriptor
 */
void sdp_add_keyboard()
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, hidkb_uuid, l2cap_uuid, hidp_uuid;
	sdp_profile_desc_t profile[1];
	sdp_list_t *aproto, *proto[3];
	sdp_data_t *channel, *lang_lst, *lang_lst2, *hid_spec_lst, *hid_spec_lst2;
	int i;
	uint8_t dtd = SDP_UINT16;
	uint8_t dtd2 = SDP_UINT8;
	uint8_t dtd_data = SDP_TEXT_STR8;
	sdp_session_t *session;
	void *dtds[2];
	void *values[2];
	void *dtds2[2];
	void *values2[2];
	int leng[2];
	uint8_t hid_spec_type = 0x22;
	uint16_t hid_attr_lang[] = {0x409,0x100};
	static const uint8_t ctrl = 0x11;
	static const uint8_t intr = 0x13;
	static const uint16_t hid_attr[] = {0x100,0x111,0x40,0x0d,0x01,0x01};
	static const uint16_t hid_attr2[] = {0x0,0x01,0x100,0x1f40,0x01,0x01};
	// taken from Apple Wireless Keyboard
	const uint8_t hid_spec[] = { 
		0x05, 0x01, // usage page
		0x09, 0x06, // keyboard
		0xa1, 0x01, // key codes
		0x85, 0x01, // minimum
		0x05, 0x07, // max
		0x19, 0xe0, // logical min
		0x29, 0xe7, // logical max
		0x15, 0x00, // report size
		0x25, 0x01, // report count
		0x75, 0x01, // input data variable absolute
		0x95, 0x08, // report count
		0x81, 0x02, // report size
		0x75, 0x08, 
		0x95, 0x01, 
		0x81, 0x01, 
		0x75, 0x01, 
		0x95, 0x05,
		0x05, 0x08,
		0x19, 0x01,
		0x29, 0x05, 
		0x91, 0x02,
		0x75, 0x03,
		0x95, 0x01,
		0x91, 0x01,
		0x75, 0x08,
		0x95, 0x06,
		0x15, 0x00,
		0x26, 0xff,
		0x00, 0x05,
		0x07, 0x19,
		0x00, 0x2a,
		0xff, 0x00,
		0x81, 0x00,
		0x75, 0x01,
		0x95, 0x01,
		0x15, 0x00,
		0x25, 0x01,
		0x05, 0x0c,
		0x09, 0xb8,
		0x81, 0x06,
		0x09, 0xe2,
		0x81, 0x06,
		0x09, 0xe9,
		0x81, 0x02,
		0x09, 0xea,
		0x81, 0x02,
		0x75, 0x01,
		0x95, 0x04,
		0x81, 0x01,
		0xc0         // end tag
	};


	if (!sdp_session) {
		printf("%s: sdp_session invalid\n", (char*)__func__);
		exit(-1);
	}
	session = sdp_session;

	sdp_record = sdp_record_alloc();
	if (!sdp_record) {
		perror("add_keyboard sdp_record_alloc: ");
		exit(-1);
	}

	memset((void*)sdp_record, 0, sizeof(sdp_record_t));
	sdp_record->handle = 0xffffffff;
	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(sdp_record, root);

	add_lang_attr(sdp_record);
	
	sdp_uuid16_create(&hidkb_uuid, HID_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &hidkb_uuid);
	sdp_set_service_classes(sdp_record, svclass_id);

	sdp_uuid16_create(&profile[0].uuid, HID_PROFILE_ID);
	profile[0].version = 0x0100;
	pfseq = sdp_list_append(0, profile);
	sdp_set_profile_descs(sdp_record, pfseq);

	// PROTO
	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[1] = sdp_list_append(0, &l2cap_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &ctrl);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(0, proto[1]);

	sdp_uuid16_create(&hidp_uuid, HIDP_UUID);
	proto[2] = sdp_list_append(0, &hidp_uuid);
	apseq = sdp_list_append(apseq, proto[2]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(sdp_record, aproto);

	// ATTR_ADD_PROTO
	proto[1] = sdp_list_append(0, &l2cap_uuid);
	channel = sdp_data_alloc(SDP_UINT8, &intr);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(0, proto[1]);

	sdp_uuid16_create(&hidp_uuid, HIDP_UUID);
	proto[2] = sdp_list_append(0, &hidp_uuid);
	apseq = sdp_list_append(apseq, proto[2]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_add_access_protos(sdp_record, aproto);
	
	sdp_set_info_attr(sdp_record, "Collin's Fake Bluetooth Keyboard", 
		"MUlliNER.ORG", "http://www.mulliner.org/bluetooth/");

	for (i = 0; i < sizeof(hid_attr)/2; i++) {
		sdp_attr_add_new(sdp_record, SDP_ATTR_HID_DEVICE_RELEASE_NUMBER+i, SDP_UINT16, &hid_attr[i]);
	}

	dtds[0] = &dtd2;
	values[0] = &hid_spec_type;
	dtds[1] = &dtd_data;
	values[1] = (uint8_t*)hid_spec;
	leng[0] = 0;
	leng[1] = sizeof(hid_spec);
	hid_spec_lst = sdp_seq_alloc_with_length(dtds, values, leng, 2);
	hid_spec_lst2 = sdp_data_alloc(SDP_SEQ8, hid_spec_lst);	
	sdp_attr_add(sdp_record, SDP_ATTR_HID_DESCRIPTOR_LIST, hid_spec_lst2);
	
	for (i = 0; i < sizeof(hid_attr_lang)/2; i++) {
		dtds2[i] = &dtd;
		values2[i] = &hid_attr_lang[i];
	}
	lang_lst = sdp_seq_alloc(dtds2, values2, sizeof(hid_attr_lang)/2);
	lang_lst2 = sdp_data_alloc(SDP_SEQ8, lang_lst);	
	sdp_attr_add(sdp_record, SDP_ATTR_HID_LANG_ID_BASE_LIST, lang_lst2);

	sdp_attr_add_new(sdp_record, SDP_ATTR_HID_SDP_DISABLE, SDP_UINT16, &hid_attr2[0]);
	for (i = 0; i < sizeof(hid_attr2)/2-1; i++) {
		sdp_attr_add_new(sdp_record, SDP_ATTR_HID_REMOTE_WAKEUP+i, SDP_UINT16, &hid_attr2[i+1]);
	}
	
	if (sdp_record_register(session, sdp_record, 0) < 0) {
		printf("%s: HID Device (Keyboard) Service Record registration failed\n", (char*)__func__);
		exit(-1);
	}
}

void sdp_remove()
{
	if (sdp_record && sdp_record_unregister(sdp_session, sdp_record)) {
		printf("%s: HID Device (Keyboard) Service Record unregistration failed\n", (char*)__func__);
	}
	
	sdp_close(sdp_session);
}

int sdp_open()
{
	if (!sdp_session) {
		sdp_session = sdp_connect(BDADDR_ANY, BDADDR_LOCAL, 0);
	}
	if (!sdp_session) {
		printf("%s: sdp_session invalid\n", (char*)__func__);
		//exit(-1);
	}
	else {
		return(1);
	}
	return(0);
}

#ifdef SDP_MAIN
int main()
{
	sdp_open();
	sdp_add_keyboard();
	sleep(60);	
	sdp_remove();
	return(1);
}
#endif

/*
 * HID stuff
 *
 * Collin R. Mulliner <collin@betaversion.net>
 * http://www.mulliner.org/bluetooth/
 *
 * License: GPLv2
 *
 * Most parts taken from BlueZ
 * 
 */

#ifndef __HID_H__
#define __HID_H__

/* HIDP L2CAP PSMs */
#define L2CAP_PSM_HIDP_CTRL 0x11
#define L2CAP_PSM_HIDP_INTR 0x13

/* HIDP header masks */
#define HIDP_HEADER_TRANS_MASK                  0xf0
#define HIDP_HEADER_PARAM_MASK                  0x0f

/* HIDP transaction types */
#define HIDP_TRANS_HANDSHAKE                    0x00
#define HIDP_TRANS_HID_CONTROL                  0x10
#define HIDP_TRANS_GET_REPORT                   0x40
#define HIDP_TRANS_SET_REPORT                   0x50
#define HIDP_TRANS_GET_PROTOCOL                 0x60
#define HIDP_TRANS_SET_PROTOCOL                 0x70
#define HIDP_TRANS_GET_IDLE                     0x80
#define HIDP_TRANS_SET_IDLE                     0x90
#define HIDP_TRANS_DATA                         0xa0
#define HIDP_TRANS_DATC                         0xb0

/* HIDP handshake results */
#define HIDP_HSHK_SUCCESSFUL                    0x00
#define HIDP_HSHK_NOT_READY                     0x01
#define HIDP_HSHK_ERR_INVALID_REPORT_ID         0x02
#define HIDP_HSHK_ERR_UNSUPPORTED_REQUEST       0x03
#define HIDP_HSHK_ERR_INVALID_PARAMETER         0x04
#define HIDP_HSHK_ERR_UNKNOWN                   0x0e
#define HIDP_HSHK_ERR_FATAL                     0x0f

/* HIDP control operation parameters */
#define HIDP_CTRL_NOP                           0x00
#define HIDP_CTRL_HARD_RESET                    0x01
#define HIDP_CTRL_SOFT_RESET                    0x02
#define HIDP_CTRL_SUSPEND                       0x03
#define HIDP_CTRL_EXIT_SUSPEND                  0x04
#define HIDP_CTRL_VIRTUAL_CABLE_UNPLUG          0x05

/* HIDP data transaction headers */
#define HIDP_DATA_RTYPE_MASK                    0x03
#define HIDP_DATA_RSRVD_MASK                    0x0c
#define HIDP_DATA_RTYPE_OTHER                   0x00
#define HIDP_DATA_RTYPE_INPUT                   0x01
#define HIDP_DATA_RTYPE_OUPUT                   0x02
#define HIDP_DATA_RTYPE_FEATURE                 0x03

/* HIDP protocol header parameters */
#define HIDP_PROTO_BOOT                         0x00
#define HIDP_PROTO_REPORT                       0x01


static const unsigned char keycode2hidp[256] = {
0,41,30,31,32,33,34,35,36,37,38,39,45,46,42,43,20,
26,8,21,23,28,24,12,18,19,47,48,40,224,4,22,7,
9,10,11,13,14,15,51,52,53,225,49,29,27,6,25,5,
17,16,54,55,56,229,85,226,44,57,58,59,60,61,62,63,
64,65,66,67,83,71,95,96,97,86,92,93,94,87,89,90,
91,98,99,0,148,100,68,69,135,146,147,138,136,139,140,88,
228,84,70,230,0,74,82,75,80,79,77,81,78,73,76,
0,127,129,128,102,103,0,72,0,133,144,145,137,227,231,101,120,
121,118,122,119,124,116,125,126,123,117,0,251,0,248,0,
0,0,0,0,0,0,240,0,249,0,0,0,0,0,241,242,
0,236,0,235,232,234,233,0,0,0,0,0,0,250,0,0,247,
245,246,0,0,0,0,104,105,106,107,108,109,110,111,112,113,
114,115,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static const unsigned char hidp2keycode[256] = {
          0,  0,  0,  0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38,
         50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44,  2,  3,
          4,  5,  6,  7,  8,  9, 10, 11, 28,  1, 14, 15, 57, 12, 13, 26,
         27, 43, 43, 39, 40, 41, 51, 52, 53, 58, 59, 60, 61, 62, 63, 64,
         65, 66, 67, 68, 87, 88, 99, 70,119,110,102,104,111,107,109,106,
        105,108,103, 69, 98, 55, 74, 78, 96, 79, 80, 81, 75, 76, 77, 71,
         72, 73, 82, 83, 86,127,116,117,183,184,185,186,187,188,189,190,
        191,192,193,194,134,138,130,132,128,129,131,137,133,135,136,113,
        115,114,  0,  0,  0,121,  0, 89, 93,124, 92, 94, 95,  0,  0,  0,
        122,123, 90, 91, 85,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
         29, 42, 56,125, 97, 54,100,126,164,166,165,163,161,115,114,113,
        150,158,159,128,136,177,178,176,142,152,173,140
};

#endif

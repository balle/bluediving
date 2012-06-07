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

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>


#define OBEX_FILE_TRANSFER  1


int obex_open(bdaddr_t *src, bdaddr_t *dst, uint8_t channel);
int obex_connect(int od, int mode);
int obex_disconnect(int od);
int obex_setpath(int od, const char *path, uint8_t flags);
int obex_get(int od, const char *type, const char *name, unsigned char **data, size_t *size);
int obex_put(int od, const char *type, const char *name, unsigned char *data, size_t size);
void obex_close(int od);

int sdp_get_rfcomm_channel(bdaddr_t *src, bdaddr_t *dst, uint16_t service, uint8_t *channel);
int sdp_get_opp_channel(bdaddr_t *src, bdaddr_t *dst, uint8_t *channel);
int sdp_get_ftp_channel(bdaddr_t *src, bdaddr_t *dst, uint8_t *channel);

struct tm *atotm(const char *nptr, struct tm *tm);

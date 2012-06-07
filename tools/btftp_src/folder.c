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
#include <time.h>
#include <sys/param.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "goep.h"
#include "ftp.h"


void print_folder_listing(const char *buffer, int size)
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	xmlChar *prop;

	time_t t;
	struct tm mtime;
	char name[MAXPATHLEN + 1], tmp[20];
	unsigned int fsize;

	doc = xmlParseMemory(buffer, size);
	if (doc == NULL) {
		printf("Error\n");
		return;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		printf("Error\n");
		xmlFreeDoc(doc);
		return;
	}
	if (xmlStrcmp(cur->name, (const xmlChar *) "folder-listing")) {
		printf("No folder listing\n");
		xmlFreeDoc(doc);
		return;
	}

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if (xmlIsBlankNode(cur)) {
			cur = cur->next;
			continue;
		}

		if (!xmlStrcmp(cur->name, (const xmlChar *) "parent-folder")) {

			time(&t);
			strftime(tmp, sizeof(tmp), "%b %e %R", localtime(&t));
			printf("drwxr-xr-x %10s %s ..\n", "", tmp);

		} else if (!xmlStrcmp(cur->name, (const xmlChar *) "folder")) {

			prop = xmlGetProp(cur, (const xmlChar *) "name");
			snprintf(name, MAXPATHLEN, "%s", prop);
			xmlFree(prop);

			prop = xmlGetProp(cur, (const xmlChar *) "modified");
			if (prop) {
				atotm((char *) prop, &mtime);
				xmlFree(prop);

				strftime(tmp, sizeof(tmp), "%b %e %R", &mtime);
			} else {
				time(&t);
				strftime(tmp, sizeof(tmp), "%b %e %R", localtime(&t));
			}

			printf("drwxr-xr-x %10s %s %s\n", "", tmp, name);

		} else if (!xmlStrcmp(cur->name, (const xmlChar *) "file")) {

			prop = xmlGetProp(cur, (const xmlChar *) "name");
			snprintf(name, MAXPATHLEN, "%s", prop);
			xmlFree(prop);

			prop = xmlGetProp(cur, (const xmlChar *) "size");
			fsize = atol((char *) prop);
			xmlFree(prop);

			prop = xmlGetProp(cur, (const xmlChar *) "modified");
			if (prop) {
				atotm((char *) prop, &mtime);
				xmlFree(prop);

				strftime(tmp, sizeof(tmp), "%b %e %R", &mtime);
			} else {
				time(&t);
				strftime(tmp, sizeof(tmp), "%b %e %R", localtime(&t));
			}

			printf("-rw-r--r-- %10d %s %s\n", fsize, tmp, name);

		};

		cur = cur->next;
	}

	xmlFreeDoc(doc);
}

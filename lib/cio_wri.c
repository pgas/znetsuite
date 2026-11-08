/*
 * CIO_WRI.C - RomWBW Output Buffer to Character Device
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"

cio_write(u, buf, len) unsigned char u;
char *buf;
int len;
{
  int i;
  for (i = 0; i < len; i++) {
    cio_out(u, (unsigned char)buf[i]);
  }
}


/*
 * CIO_PUTS.C - RomWBW Output String to Character Device
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"

cio_puts(u, s) unsigned char u;
char *s;
{
  while (*s) {
    cio_out(u, (unsigned char)*s++);
  }
}

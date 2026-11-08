/*
 * CIO_FLSH.C - Flush Pending Input from Character Device
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"

cio_flush(u) unsigned char u;
{
  unsigned int count;
  for (count = 0; count < 10000; count++) {
    if (cio_ist(u)) {
      cio_in(u);
      count = 0;
    }
  }
}


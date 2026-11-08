/*
 * CIO_TO.C - RomWBW Timed Character Input
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"

int cio_get_timeout(u, max_loops, abort_u, abort_ch)
unsigned char u;
long max_loops;
unsigned char abort_u;
int abort_ch;
{
  long count;
  for (count = 0; count < max_loops; count++) {
    if (abort_ch >= 0 && cio_ist(abort_u)) {
      int ck = cio_in(abort_u) & 0xFF;
      if (ck == abort_ch)
        return -2; /* User abort */
    }
    if (cio_ist(u)) {
      return cio_in(u) & 0xFF;
    }
  }
  return -1; /* Timed out */
}

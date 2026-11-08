/*
 * CIO_LINE.C - RomWBW Timed Line Input
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"

int cio_readline(u, buf, maxlen, max_loops, abort_u, abort_ch)
unsigned char u;
char *buf;
int maxlen;
long max_loops;
unsigned char abort_u;
int abort_ch;
{
  int pos = 0;
  int ch;

  while (1) {
    ch = cio_get_timeout(u, max_loops, abort_u, abort_ch);
    if (ch == -2)
      return -2; /* Aborted */
    if (ch == -1) {
      if (pos > 0) {
        buf[pos] = '\0';
        return pos;
      }
      return -1; /* Timed out */
    }
    if (ch == '\r')
      continue;
    if (ch == '\n') {
      buf[pos] = '\0';
      return pos;
    }
    if (pos < maxlen - 1) {
      buf[pos++] = (char)ch;
    }
  }
}

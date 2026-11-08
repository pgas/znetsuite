/*
 * CIO_OK.C - Wait for OK or ERROR response from AT modem
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"
#include <string.h>

int cio_wait_ok(u, max_loops)
unsigned char u;
long max_loops;
{
  char line[64];
  int pos = 0;
  int ch;

  while (1) {
    ch = cio_get_timeout(u, max_loops, 0, -1);
    if (ch < 0)
      return 0;
    if (ch == '\r')
      continue;
    if (ch == '\n') {
      line[pos] = '\0';
      if (strcmp(line, "OK") == 0)
        return 1;
      if (strncmp(line, "ERROR", 5) == 0)
        return 0;
      pos = 0;
    } else if (pos < 62) {
      line[pos++] = (char)ch;
    }
  }
}


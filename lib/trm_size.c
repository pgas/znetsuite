/*
 * TRM_SIZE.C - VT100/ANSI Dynamic Screen Dimension Detection via DSR CPR
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"
#include "term.h"

/* Query VT100 / ANSI terminal dimensions via Cursor Position Report */
term_detect_size() {
  int pos = 0;
  int r = 0;
  int c = 0;
  int ch;
  long timeout;
  char *p;
  char buf[32];

  /* Drain pending console input */
  while (cio_ist(CIO_CONSOLE)) {
    cio_in(CIO_CONSOLE);
  }

  /* Move cursor to bottom-right boundary and query position */
  term_puts("\033[999;999H\033[6n");

  /* Read response: ESC [ <row> ; <col> R */
  for (timeout = 0; timeout < 50000L; timeout++) {
    if (cio_ist(CIO_CONSOLE)) {
      ch = cio_in(CIO_CONSOLE) & 0xFF;
      if (pos < sizeof(buf) - 1) {
        buf[pos++] = (char)ch;
        if (ch == 'R')
          break;
      }
    }
  }
  buf[pos] = '\0';

  /* Parse response numbers */
  p = buf;
  while (*p && *p != '[')
    p++;
  if (*p == '[') {
    p++;
    while (*p >= '0' && *p <= '9') {
      r = r * 10 + (*p - '0');
      p++;
    }
    if (*p == ';') {
      p++;
      while (*p >= '0' && *p <= '9') {
        c = c * 10 + (*p - '0');
        p++;
      }
    }
  }

  /* Validate boundaries; fallback to 80x24 */
  if (r >= 10 && r <= 150 && c >= 30 && c <= 250) {
    term_rows = r;
    term_cols = c;
  } else {
    term_rows = 24;
    term_cols = 80;
  }
}


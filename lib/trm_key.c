/*
 * TRM_KEY.C - VT100/ANSI Keystroke & Escape Sequence Parser
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"
#include "term.h"

/* Check if character is available from console */
int term_has_key() { return cio_ist(CIO_CONSOLE); }

/* Wait briefly for next byte of an ANSI escape sequence over serial */
static int term_wait_seq(loops) int loops;
{
  int i;
  for (i = 0; i < loops; i++) {
    if (cio_ist(CIO_CONSOLE))
      return cio_in(CIO_CONSOLE) & 0xFF;
  }
  return -1;
}

/* Read next keystroke, parsing ANSI escape sequences */
int term_getkey() {
  int c;
  int c2;
  int c3;

  while (!cio_ist(CIO_CONSOLE)) {
    /* Wait for key */
  }

  c = cio_in(CIO_CONSOLE) & 0xFF;

  if (c == KEY_ESC) {
    c2 = term_wait_seq(4000);
    if (c2 == '[' || c2 == 'O') {
      c3 = term_wait_seq(25000);
      if (c3 == 'A')
        return KEY_UP;
      if (c3 == 'B')
        return KEY_DOWN;
      if (c3 == 'C')
        return KEY_RIGHT;
      if (c3 == 'D')
        return KEY_LEFT;
      if (c3 == 'H')
        return KEY_HOME;
      if (c3 == 'F')
        return KEY_END;
      if (c3 == 'Z')
        return KEY_BTAB;
      if (c3 >= '1' && c3 <= '8') {
        term_wait_seq(25000); /* Consume trailing '~' */
        if (c3 == '3')
          return KEY_DEL;
        if (c3 == '5')
          return KEY_PGUP;
        if (c3 == '6')
          return KEY_PGDN;
        if (c3 == '1' || c3 == '7')
          return KEY_HOME;
        if (c3 == '4' || c3 == '8')
          return KEY_END;
      }
      return 0;
    }
    return KEY_ESC;
  }

  return c;
}


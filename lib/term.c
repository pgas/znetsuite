/*
 * TERM.C - Terminal & ANSI Escape Handling
 *
 *  VT100/ANSI terminal handling

 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "term.h"
#include "cio.h"
#include <stdio.h>

/* Terminal dimensions (defaults to standard 80x24) */
int term_rows = 24;
int term_cols = 80;

/* Format an integer into string buffer */
static term_itoa(buf, val) char *buf;
int val;
{
  char tmp[10];
  int i = 0;
  int j = 0;

  if (val == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return;
  }
  if (val < 0) {
    buf[j++] = '-';
    val = -val;
  }
  while (val > 0) {
    tmp[i++] = (char)('0' + (val % 10));
    val /= 10;
  }
  while (i > 0) {
    buf[j++] = tmp[--i];
  }
  buf[j] = '\0';
}

/* Direct output of single character to console (no function pointer overhead)
 */
term_putc(c) int c;
{
  cio_out(CIO_CONSOLE, (unsigned char)c);
}

/* Direct output of string to console */
term_puts(s) char *s;
{
  cio_puts(CIO_CONSOLE, s);
}

/* Clear screen and home cursor */
term_cls() { term_puts(TERM_CLS); }

/* Move cursor to top-left (1,1) */
term_home() { term_puts(TERM_HOME); }

/* Position cursor at 1-based (row, col) */
term_goto(row, col) int row;
int col;
{
  char num[10];

  term_puts("\033[");
  term_itoa(num, row);
  term_puts(num);
  term_putc(';');
  term_itoa(num, col);
  term_puts(num);
  term_putc('H');
}

/* Clear from cursor to end of current line */
term_clear_eol() { term_puts(TERM_CLEAR_EOL); }

/* Clear entire current line */
term_clear_line() { term_puts(TERM_CLEAR_LINE); }

/* Hide text cursor */
term_cursor_hide() { term_puts(TERM_CURSOR_HIDE); }

/* Show text cursor */
term_cursor_show() { term_puts(TERM_CURSOR_SHOW); }

/* Set text attribute or color */
term_attr(attr_str) char *attr_str;
{
  term_puts(attr_str);
}

/* Reset all text attributes to default */
term_reset() { term_puts(TERM_ATTR_RESET); }

/* Check if character is available from console */
int term_has_key() { return cio_ist(CIO_CONSOLE); }

/* Query VT100 / ANSI terminal dimensions via Cursor Position Report */
term_detect_size() {
  char buf[32];
  int pos = 0;
  long timeout;
  int r = 0;
  int c = 0;
  char *p;

  /* Drain pending console input */
  while (cio_ist(CIO_CONSOLE)) {
    cio_in(CIO_CONSOLE);
  }

  /* Move cursor to bottom-right boundary and query position */
  term_puts("\033[999;999H\033[6n");

  /* Read response: ESC [ <row> ; <col> R */
  for (timeout = 0; timeout < 50000L; timeout++) {
    if (cio_ist(CIO_CONSOLE)) {
      int ch = cio_in(CIO_CONSOLE) & 0xFF;
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

/* Read next keystroke, parsing ANSI escape sequences */
int term_getkey() {
  int c;

  while (!cio_ist(CIO_CONSOLE)) {
    /* Wait for key */
  }

  c = cio_in(CIO_CONSOLE) & 0xFF;

  if (c == KEY_ESC) {
    int i;
    for (i = 0; i < 600; i++) {
      if (cio_ist(CIO_CONSOLE))
        break;
    }
    if (cio_ist(CIO_CONSOLE)) {
      int c2 = cio_in(CIO_CONSOLE) & 0xFF;
      if (c2 == '[' || c2 == 'O') {
        for (i = 0; i < 600; i++) {
          if (cio_ist(CIO_CONSOLE))
            break;
        }
        if (cio_ist(CIO_CONSOLE)) {
          int c3 = cio_in(CIO_CONSOLE) & 0xFF;
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
          if (c3 == '5') {
            if (cio_ist(CIO_CONSOLE))
              cio_in(CIO_CONSOLE); /* Consume '~' */
            return KEY_PGUP;
          }
          if (c3 == '6') {
            if (cio_ist(CIO_CONSOLE))
              cio_in(CIO_CONSOLE); /* Consume '~' */
            return KEY_PGDN;
          }
          if (c3 == '1' || c3 == '7') {
            if (cio_ist(CIO_CONSOLE))
              cio_in(CIO_CONSOLE); /* Consume '~' */
            return KEY_HOME;
          }
          if (c3 == '4' || c3 == '8') {
            if (cio_ist(CIO_CONSOLE))
              cio_in(CIO_CONSOLE); /* Consume '~' */
            return KEY_END;
          }
        }
      }
    }
    return KEY_ESC;
  }

  return c;
}

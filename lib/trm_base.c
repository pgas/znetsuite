/*
 * TRM_BASE.C - Core Terminal State & ANSI Escape Primitives
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"
#include "term.h"

/* Terminal dimensions (defaults to standard 80x24) */
int term_rows = 24;
int term_cols = 80;

/* Direct output of single character to console */
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


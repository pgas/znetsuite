/*
 * TRM_GOTO.C - VT100/ANSI Cursor Positioning
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "fmt.h"
#include "term.h"

/* Position cursor at 1-based (row, col) */
term_goto(row, col) int row;
int col;
{
  char num[10];

  term_puts("\033[");
  fmt_int(num, row);
  term_puts(num);
  term_putc(';');
  fmt_int(num, col);
  term_puts(num);
  term_putc('H');
}


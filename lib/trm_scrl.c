/*
 * TRM_SCRL.C - VT100 Hardware Scrolling Regions
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "fmt.h"
#include "term.h"

/* Set VT100 hardware scrolling region [top..bot] (1-based) */
term_scroll_region(top, bot) int top;
int bot;
{
  char num[10];

  term_puts("\033[");
  fmt_int(num, top);
  term_puts(num);
  term_putc(';');
  fmt_int(num, bot);
  term_puts(num);
  term_putc('r');
}

/* Reset VT100 scrolling region to full screen */
term_scroll_reset() { term_puts("\033[r"); }

/* Scroll region [top..bot] up by 1 line (content moves up, bottom line blank) */
term_scroll_up(top, bot) int top;
int bot;
{
  term_scroll_region(top, bot);
  term_goto(bot, 1);
  term_puts("\033D");
  term_scroll_reset();
}

/* Scroll region [top..bot] down by 1 line (content moves down, top line blank) */
term_scroll_down(top, bot) int top;
int bot;
{
  term_scroll_region(top, bot);
  term_goto(top, 1);
  term_puts("\033M");
  term_scroll_reset();
}


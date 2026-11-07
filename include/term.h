/*
 * TERM.H - Terminal & ANSI Escape Handling for ZNetSuite
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#ifndef _TERM_H
#define _TERM_H

/* Standard control characters */
#define KEY_CTRL_A         0x01
#define KEY_CTRL_B         0x02
#define KEY_CTRL_C         0x03
#define KEY_CTRL_D         0x04
#define KEY_CTRL_E         0x05
#define KEY_CTRL_F         0x06
#define KEY_TAB            0x09
#define KEY_LF             0x0A
#define KEY_CTRL_L         0x0C
#define KEY_CR             0x0D
#define KEY_CTRL_U         0x15
#define KEY_ESC            0x1B
#define KEY_BS             0x7F

/* Extended pseudo-keys returned by term_getkey() */
#define KEY_UP             0x101
#define KEY_DOWN           0x102
#define KEY_LEFT           0x103
#define KEY_RIGHT          0x104
#define KEY_PGUP           0x105
#define KEY_PGDN           0x106
#define KEY_HOME           0x107
#define KEY_END            0x108
#define KEY_BTAB           0x109

/* ANSI / VT100 Escape Sequences */
#define TERM_CLS           "\033[2J\033[H"
#define TERM_HOME          "\033[H"
#define TERM_CLEAR_EOL     "\033[K"
#define TERM_CLEAR_LINE    "\033[2K"
#define TERM_CLEAR_DOWN    "\033[J"
#define TERM_CURSOR_HIDE   "\033[?25l"
#define TERM_CURSOR_SHOW   "\033[?25h"
#define TERM_CURSOR_SAVE   "\033[s"
#define TERM_CURSOR_RESTORE "\033[u"

/* Text attributes */
#define TERM_ATTR_RESET    "\033[0m"
#define TERM_ATTR_BOLD     "\033[1m"
#define TERM_ATTR_DIM      "\033[2m"
#define TERM_ATTR_UNDER    "\033[4m"
#define TERM_ATTR_REVERSE  "\033[7m"

/* Standard ANSI 8 colors */
#define TERM_FG_BLACK      "\033[30m"
#define TERM_FG_RED        "\033[31m"
#define TERM_FG_GREEN      "\033[32m"
#define TERM_FG_YELLOW     "\033[33m"
#define TERM_FG_BLUE       "\033[34m"
#define TERM_FG_MAGENTA    "\033[35m"
#define TERM_FG_CYAN       "\033[36m"
#define TERM_FG_WHITE      "\033[37m"

#define TERM_BG_BLACK      "\033[40m"
#define TERM_BG_RED        "\033[41m"
#define TERM_BG_GREEN      "\033[42m"
#define TERM_BG_YELLOW     "\033[43m"
#define TERM_BG_BLUE       "\033[44m"
#define TERM_BG_MAGENTA    "\033[45m"
#define TERM_BG_CYAN       "\033[46m"
#define TERM_BG_WHITE      "\033[47m"

/* Current detected terminal dimensions */
extern int term_rows;
extern int term_cols;

/* Direct character output */
extern term_putc(/* int ch */);
extern term_puts(/* char *s */);

/* Cursor & Screen manipulation */
extern term_cls();
extern term_home();
extern term_goto(/* int row, int col */);
extern term_clear_eol();
extern term_clear_line();
extern term_cursor_hide();
extern term_cursor_show();
extern term_attr(/* char *attr_str */);
extern term_reset();

/* Dynamic window dimension detection via VT100 CPR query */
extern term_detect_size();

/* Keystroke parser (reads raw char or ANSI escape sequences) */
extern int term_getkey();
extern int term_has_key();

#endif /* _TERM_H */


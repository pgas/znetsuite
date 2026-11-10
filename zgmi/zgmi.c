/*
 * ZGMI - Gemini Protocol Client for RomWBW CP/M
 *
 * Minimalist Gemini client communicating with an AT-command serial-to-Wi-Fi
 * bridge (ttl2wifi / Pico W) on RomWBW character I/O.
 *
 * Features:
 * - Flicker-free differential VT100/ANSI screen rendering & hardware scroll
 * - Sliding RAM window + lazy CP/M BDOS disk spooling for large documents
 *   with full forward/backward scrollback on 32K and 62K TPA devices
 * - Plain-text configuration file (ZGMI.CFG) + CLI overrides
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"
#include "fmt.h"
#include "spool.h"
#include "term.h"
#include <string.h>

/* Shared static buffers to keep Z80 stack frames small (< 128 bytes) */
static char rx_line[160];
static char cmd_buf[160];
static char val_buf[128];

/* Pointer to CP/M command tail buffer */
static unsigned char *tail = (unsigned char *)0x0080;

/* Send AT command and wait for OK/ERROR response */
static int send_at(u, cmd) unsigned char u;
char *cmd;
{
  cio_send_cmd(u, cmd);
  return cio_wait_ok(u, 15000L);
}

/* Initialize serial bridge: sync AT, ATE0, AT+PAGE=0 */
int bridge_init(u)
unsigned char u;
{
  int attempt;
  cio_flush(u);

  for (attempt = 0; attempt < 3; attempt++) {
    if (send_at(u, "AT"))
      break;
  }

  send_at(u, "ATE0");
  send_at(u, "AT+PAGE=0");
  cio_flush(u);
  return 1;
}

/* Restore serial bridge command echo and saved profile on exit */
static bridge_restore(u) unsigned char u;
{
  cio_flush(u);
  send_at(u, "ATE1");
  send_at(u, "ATZ");
  cio_flush(u);
}

/* Fetch and process a command stream from bridge */
fetch_stream(u, cmd) unsigned char u;
char *cmd;
{
  int res;
  int was_spooling = 0;
  char *p;

  show_status("Connecting to bridge...");
  cio_flush(u);
  cio_send_cmd(u, cmd);

  clear_doc();
  doc_top = 0;
  header_dirty = 1;
  prev_doc_top = -1;
  prev_cur_link = -1;

  for (;;) {
    res = cio_readline(u, rx_line, sizeof(rx_line), 100000L, CIO_CONSOLE,
                       KEY_CTRL_C);
    if (res == -2) {
      cio_out(u, KEY_CTRL_C);
      add_doc_line("[Download aborted by user]");
      break;
    }
    if (res < 0) {
      if (total_lines == 0) {
        add_doc_line("[Connection timed out]");
      }
      break;
    }

    if (strcmp(rx_line, "OK") == 0) {
      break;
    }
    if (strncmp(rx_line, "ERROR", 5) == 0) {
      add_doc_line(rx_line);
      break;
    }

    /* Track URL metadata from bridge */
    if (strncmp(rx_line, "+GO [", 5) == 0) {
      p = strchr(rx_line, ':');
      if (p) {
        p++;
        while (*p == ' ')
          p++;
        strncpy(current_url, p, sizeof(current_url) - 1);
        current_url[sizeof(current_url) - 1] = '\0';
      }
      continue;
    }
    if (strncmp(rx_line, "+BACK: ", 7) == 0) {
      strncpy(current_url, rx_line + 7, sizeof(current_url) - 1);
      current_url[sizeof(current_url) - 1] = '\0';
      continue;
    }
    if (strncmp(rx_line, "+REDIRECT: ", 11) == 0) {
      strncpy(current_url, rx_line + 11, sizeof(current_url) - 1);
      current_url[sizeof(current_url) - 1] = '\0';
      continue;
    }
    if (strncmp(rx_line, "+GEMINI: ", 9) == 0) {
      if (rx_line[9] != '2') {
        add_doc_line(rx_line);
      }
      continue;
    }

    if (!doc_truncated) {
      add_doc_line(rx_line);
      if (spool_active && !was_spooling) {
        show_status("Spooling to disk...");
        was_spooling = 1;
      }
    }
  }

  if (spool_active) {
    spool_finish_write();
  }

  if (total_lines == 0) {
    add_doc_line("[Empty document]");
  }

  if (total_links > 0) {
    cur_link = 0;
  }
}

/* Fetch current_url over bridge and snap link visibility */
static fetch_url(u) unsigned char u;
{
  strcpy(cmd_buf, "AT+GEMINI=\"");
  strcat(cmd_buf, current_url);
  strcat(cmd_buf, "\"");
  fetch_stream(u, cmd_buf);
  ensure_link_visible();
}

/* Main interaction loop */
event_loop(u) unsigned char u;
{
  int screen_lines;
  int old_top;
  int key;
  int lid;
  char num_buf[16];

  term_cursor_hide();
  update_screen(1);

  for (;;) {
    key = term_getkey();
    screen_lines = term_rows - 2;
    old_top = doc_top;

    switch (key) {
    /* Vim: scroll down 1 line */
    case 'j':
    case 'J':
    case KEY_DOWN:
      if (doc_top + screen_lines < total_lines) {
        doc_top++;
        update_screen(0);
      }
      break;

    /* Vim: scroll up 1 line */
    case 'k':
    case 'K':
    case KEY_UP:
      if (doc_top > 0) {
        doc_top--;
        update_screen(0);
      }
      break;

    /* Vim: half-page down */
    case 'd':
    case 'D':
    case KEY_CTRL_D:
      doc_top += screen_lines / 2;
      if (doc_top + screen_lines > total_lines)
        doc_top = total_lines - screen_lines;
      if (doc_top < 0)
        doc_top = 0;
      if (doc_top != old_top)
        update_screen(0);
      break;

    /* Vim: half-page up */
    case 'u':
    case 'U':
    case KEY_CTRL_U:
      doc_top -= screen_lines / 2;
      if (doc_top < 0)
        doc_top = 0;
      if (doc_top != old_top)
        update_screen(0);
      break;

    /* Vim / Pager: full page down */
    case KEY_CTRL_F:
    case 'f':
    case 'F':
    case ' ':
    case KEY_PGDN:
      doc_top += screen_lines - 1;
      if (doc_top + screen_lines > total_lines)
        doc_top = total_lines - screen_lines;
      if (doc_top < 0)
        doc_top = 0;
      if (doc_top != old_top)
        update_screen(0);
      break;

    /* Vim / Pager: full page up */
    case KEY_CTRL_B:
    case 'w':
    case 'W':
    case KEY_PGUP:
      doc_top -= screen_lines - 1;
      if (doc_top < 0)
        doc_top = 0;
      if (doc_top != old_top)
        update_screen(0);
      break;

    /* Edit current URL on line above helpbar */
    case 'e':
    case 'E':
      if (prompt_edit_url(current_url)) {
        fetch_url(u);
        update_screen(1);
      }
      break;

    /* Vim: jump to top */
    case 'g':
    case KEY_HOME:
      if (doc_top != 0) {
        doc_top = 0;
        update_screen(0);
      }
      break;

    /* Vim: jump to bottom */
    case 'G':
    case KEY_END:
      doc_top = total_lines - screen_lines;
      if (doc_top < 0)
        doc_top = 0;
      if (doc_top != old_top)
        update_screen(0);
      break;

    /* Redraw and re-detect terminal size */
    case KEY_CTRL_L:
      term_detect_size();
      term_cursor_hide();
      ensure_link_visible();
      update_screen(1);
      break;

    /* Tab: move selection to next URL/link */
    case KEY_TAB:
      if (total_links > 0) {
        select_next_link();
        update_screen(0);
      }
      break;

    /* Shift-Tab / 'p': move selection to previous URL/link */
    case KEY_BTAB:
    case 'p':
    case 'P':
      if (total_links > 0) {
        select_prev_link();
        update_screen(0);
      }
      break;

    /* Enter: open / follow selected URL */
    case KEY_CR:
    case KEY_LF:
      if (total_links > 0 && cur_link >= 0 && cur_link < total_links) {
        lid = links[cur_link].id;
        strcpy(cmd_buf, "AT+GO=");
        fmt_int(num_buf, lid);
        strcat(cmd_buf, num_buf);
        fetch_stream(u, cmd_buf);
        ensure_link_visible();
        update_screen(1);
      }
      break;

    /* Back in history */
    case 'b':
    case 'B':
    case 'H':
      fetch_stream(u, "AT+BACK");
      ensure_link_visible();
      update_screen(1);
      break;

    /* Reload current page */
    case 'r':
    case 'R':
      if (current_url[0]) {
        fetch_url(u);
        update_screen(1);
      }
      break;

    /* Quit */
    case 'q':
    case 'Q':
    case KEY_CTRL_C:
      spool_cleanup();
      bridge_restore(u);
      term_scroll_reset();
      term_goto(term_rows, 1);
      term_reset();
      term_clear_eol();
      term_cursor_show();
      term_puts("\r\n");
      return;
    }
  }
}

usage() {
  cio_printf(
      "Usage: ZGMI [-u unit] [-s spool_kb] [-f spool_file] [-c cfg] <url>\n"
      "Options:\n"
      "  -u <unit>   RomWBW CIO serial unit for bridge (default: 1)\n"
      "  -s <kb>     Max disk spool size in KB, 0=off (default: 128)\n"
      "  -f <file>   Spool filename, e.g. B:ZGMI.$$$ (default: ZGMI.$$$)\n"
      "  -c <file>   Configuration file (default: ZGMI.CFG, A:ZGMI.CFG)\n"
      "Examples:\n"
      "  ZGMI gemini://geminiprotocol.net\n"
      "  ZGMI -u 1 -s 256 gemini.circumlunar.space\n");
}

main() {
  int arg_idx = 0;
  unsigned char dev_cnt;
  unsigned char dtype, dmode, dbase;
  char opt;

  cio_printf("\nZGMI v1.1 - Gemini Protocol Client (ZNetSuite)\n");

  if (!hbios_detect()) {
    cio_printf("Error: RomWBW HBIOS not detected.\n");
    return 1;
  }

  /* Load ZGMI.CFG from current drive, or fallback to A:ZGMI.CFG */
  if (!load_config_file("ZGMI.CFG")) {
    load_config_file("A:ZGMI.CFG");
  }

  cmd_buf[0] = '\0';

  /* Parse CP/M command tail */
  while (fmt_cpm_arg(tail, arg_idx, rx_line, sizeof(rx_line)) > 0) {
    if (strcmp(rx_line, "?") == 0 || strcmp(rx_line, "/?") == 0) {
      usage();
      return 0;
    }
    if (rx_line[0] == '-' && rx_line[1] && rx_line[2] == '\0') {
      opt = rx_line[1] | 0x20;
      if (opt == 'h') {
        usage();
        return 0;
      }
      if (fmt_cpm_arg(tail, ++arg_idx, val_buf, sizeof(val_buf)) <= 0) {
        cio_printf("Error: Missing argument for -%c\n", (int)opt);
        return 1;
      }
      if (opt == 'u') {
        target_unit = (unsigned char)fmt_parse_num(val_buf);
      } else if (opt == 's') {
        cfg_max_spool_kb = fmt_parse_int(val_buf);
        if (cfg_max_spool_kb < 0)
          cfg_max_spool_kb = 0;
      } else if (opt == 'f') {
        strncpy(cfg_spool_file, val_buf, sizeof(cfg_spool_file) - 1);
        cfg_spool_file[sizeof(cfg_spool_file) - 1] = '\0';
      } else if (opt == 'c') {
        if (!load_config_file(val_buf)) {
          cio_printf("Error: Could not open %s\n", val_buf);
          return 1;
        }
      }
    } else {
      strncpy(cmd_buf, rx_line, sizeof(cmd_buf) - 1);
      cmd_buf[sizeof(cmd_buf) - 1] = '\0';
    }
    arg_idx++;
  }

  if (!cmd_buf[0]) {
    if (cfg_home_url[0]) {
      strcpy(cmd_buf, cfg_home_url);
    } else {
      usage();
      return 0;
    }
  }

  dev_cnt = (unsigned char)hbios_ciocnt();
  if (target_unit >= dev_cnt) {
    cio_printf(
        "Error: Device unit %02X out of range (system has %d CIO units)\n",
        (int)target_unit, (int)dev_cnt);
    return 1;
  }

  if (hbios_device(target_unit, &dtype, &dmode, &dbase) != 0) {
    cio_printf("Error: Device unit %02X not available.\n", (int)target_unit);
    return 1;
  }

  /* Initialize TPA RAM window and normalize URL */
  spool_init_ram();
  normalize_url(cmd_buf, current_url);

  cio_printf("Connecting to bridge on CIO unit %02X (%s)...\n",
             (unsigned int)target_unit, hbios_devname(dtype));
  if (!bridge_init(target_unit)) {
    cio_printf("Warning: No response to AT check; continuing anyway...\n");
  }

  /* Query VT100 / ANSI terminal dimensions via DSR CPR */
  term_detect_size();

  /* Clear screen and hide cursor */
  term_cls();
  term_cursor_hide();

  /* Initial fetch and enter interactive loop */
  fetch_url(target_unit);
  event_loop(target_unit);

  return 0;
}

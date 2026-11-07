/*
 * ZGMI - Gemini Protocol Client for RomWBW CP/M
 *
 * Minimalist Gemini client communicating with an AT-command serial-to-Wi-Fi
 * bridge (ttl2wifi / Pico W) on RomWBW character I/O.
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include <stdio.h>
#include <string.h>
#include "cio.h"
#include "term.h"
#include "fmt.h"

#define MAX_TEXT        20000
#define MAX_LINES       700
#define MAX_LINKS       120

/* Global document buffer */
char text_pool[MAX_TEXT];
int text_pool_len;
char *doc_lines[MAX_LINES];
int total_lines;

struct link_entry {
  int line;
  int id;
} links[MAX_LINKS];
int total_links;
int cur_link;

int doc_top;
char current_url[128];
unsigned char target_unit;

/* Pointer to CP/M command tail buffer */
static unsigned char *tail = (unsigned char *)0x0080;

/* Parse link index: looks for "[ n]" or "[n]" or "=>" */
parse_link_num(s)
char *s;
{
  int n = 0;
  while (*s == ' ') s++;
  if (*s == '[') {
    s++;
    while (*s == ' ') s++;
    if (*s < '0' || *s > '9') return 0;
    while (*s >= '0' && *s <= '9') {
      n = n * 10 + (*s - '0');
      s++;
    }
    if (*s == ']') return n;
  } else if (s[0] == '=' && s[1] == '>') {
    return total_links + 1;
  }
  return 0;
}

/* Clear document buffer */
clear_doc()
{
  text_pool_len = 0;
  total_lines = 0;
  total_links = 0;
  cur_link = -1;
  doc_top = 0;
}

/* Add a line of text to document buffer */
add_doc_line(line)
char *line;
{
  int len;
  int lid;

  if (total_lines >= MAX_LINES) return 0;

  len = strlen(line);
  if (text_pool_len + len + 1 >= MAX_TEXT) return 0;

  strcpy(&text_pool[text_pool_len], line);
  doc_lines[total_lines] = &text_pool[text_pool_len];
  text_pool_len += len + 1;

  lid = parse_link_num(line);
  if (lid > 0 && total_links < MAX_LINKS) {
    links[total_links].line = total_lines;
    links[total_links].id = lid;
    total_links++;
  }

  total_lines++;
  return 1;
}

/* Display loading status on bottom row */
show_status(msg)
char *msg;
{
  term_goto(term_rows, 1);
  term_attr(TERM_ATTR_REVERSE);
  term_putc(' ');
  term_puts(msg);
  term_putc(' ');
  term_reset();
  term_clear_eol();
}

/* Read character with timeout loop; checks abort_u for abort_ch */
int cio_get_timeout(u, max_loops, abort_u, abort_ch)
unsigned char u;
long max_loops;
unsigned char abort_u;
int abort_ch;
{
  long count;
  for (count = 0; count < max_loops; count++) {
    if (abort_ch >= 0 && cio_ist(abort_u)) {
      int ck = cio_in(abort_u) & 0xFF;
      if (ck == abort_ch)
        return -2; /* User abort */
    }
    if (cio_ist(u)) {
      return cio_in(u) & 0xFF;
    }
  }
  return -1; /* Timed out */
}

/* Read one line from unit ending in '\n' (strips '\r') */
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

/* Send string followed by CRLF */
cio_send_cmd(u, cmd)
unsigned char u;
char *cmd;
{
  cio_puts(u, cmd);
  cio_out(u, '\r');
  cio_out(u, '\n');
}

/* Wait for OK or ERROR response from AT modem/bridge */
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

/* Initialize serial bridge: sync AT, ATE0, AT+PAGE=0 */
bridge_init(u)
unsigned char u;
{
  int attempt;
  cio_flush(u);

  /* Send dummy attention checks */
  for (attempt = 0; attempt < 3; attempt++) {
    cio_send_cmd(u, "AT");
    if (cio_wait_ok(u, 20000L)) break;
  }

  /* Disable command echo */
  cio_send_cmd(u, "ATE0");
  cio_wait_ok(u, 20000L);

  /* Disable screen pager on bridge */
  cio_send_cmd(u, "AT+PAGE=0");
  cio_wait_ok(u, 20000L);

  cio_flush(u);
  return 1;
}

/* Fetch and process a command stream from bridge */
fetch_stream(u, cmd)
unsigned char u;
char *cmd;
{
  char line[160];
  int res;

  show_status("Connecting to bridge...");
  cio_flush(u);
  cio_send_cmd(u, cmd);

  clear_doc();

  while (1) {
    res = cio_readline(u, line, sizeof(line), 100000L, CIO_CONSOLE, KEY_CTRL_C);
    if (res == -2) {
      /* User aborted with Ctrl-C */
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

    /* Check for termination */
    if (strcmp(line, "OK") == 0) {
      break;
    }
    if (strncmp(line, "ERROR", 5) == 0) {
      add_doc_line(line);
      break;
    }

    /* Track URL metadata from bridge */
    if (strncmp(line, "+GO [", 5) == 0) {
      char *p = strchr(line, ':');
      if (p) {
        p++;
        while (*p == ' ') p++;
        strncpy(current_url, p, sizeof(current_url) - 1);
        current_url[sizeof(current_url) - 1] = '\0';
      }
      continue;
    }
    if (strncmp(line, "+BACK: ", 7) == 0) {
      strncpy(current_url, line + 7, sizeof(current_url) - 1);
      current_url[sizeof(current_url) - 1] = '\0';
      continue;
    }
    if (strncmp(line, "+REDIRECT: ", 11) == 0) {
      strncpy(current_url, line + 11, sizeof(current_url) - 1);
      current_url[sizeof(current_url) - 1] = '\0';
      continue;
    }
    if (strncmp(line, "+GEMINI: ", 9) == 0) {
      if (line[9] != '2') {
        add_doc_line(line);
      }
      continue;
    }

    /* Content line */
    add_doc_line(line);
  }

  if (total_lines == 0) {
    add_doc_line("[Empty document]");
  }

  if (total_links > 0) {
    cur_link = 0;
  }
}

/* Ensure selected link is visible within screen lines */
ensure_link_visible()
{
  int screen_lines = term_rows - 2;
  if (cur_link >= 0 && cur_link < total_links) {
    int l_line = links[cur_link].line;
    if (l_line < doc_top) {
      doc_top = l_line;
    } else if (l_line >= doc_top + screen_lines) {
      doc_top = l_line - screen_lines + 1;
    }
  }
}

/* Render the full screen: Header (row 1), Text (2..term_rows-1), Status (term_rows) */
render_screen()
{
  int r;
  int line_idx;
  int screen_lines = term_rows - 2;
  int line_width = term_cols - 1;
  char num_buf[16];

  /* 1. Header line at row 1 */
  term_home();
  term_attr(TERM_ATTR_REVERSE);
  term_puts(" ZGMI | ");
  term_puts(current_url);
  term_reset();
  term_clear_eol();
  term_puts("\r\n");

  /* 2. Document content rows 2 to term_rows - 1 */
  for (r = 0; r < screen_lines; r++) {
    line_idx = doc_top + r;
    if (line_idx < total_lines) {
      int is_sel = (cur_link >= 0 && cur_link < total_links &&
                    links[cur_link].line == line_idx);
      char *s = doc_lines[line_idx];
      int col = 0;

      if (is_sel) {
        term_attr(TERM_ATTR_REVERSE);
      }

      while (*s && col < line_width) {
        term_putc((unsigned char)*s++);
        col++;
      }

      if (is_sel) {
        while (col < line_width) {
          term_putc(' ');
          col++;
        }
        term_reset();
      }
      term_clear_eol();
      term_puts("\r\n");
    } else {
      term_puts("~");
      term_clear_eol();
      term_puts("\r\n");
    }
  }

  /* 3. Status bar at bottom row */
  term_goto(term_rows, 1);
  term_attr(TERM_ATTR_REVERSE);

  if (total_links > 0 && cur_link >= 0) {
    term_puts(" [Link ");
    fmt_int(num_buf, cur_link + 1);
    term_puts(num_buf);
    term_putc('/');
    fmt_int(num_buf, total_links);
    term_puts(num_buf);
    term_puts("] ");
  } else {
    term_puts(" ");
  }

  term_puts("Line ");
  fmt_int(num_buf, doc_top + 1);
  term_puts(num_buf);
  term_putc('/');
  fmt_int(num_buf, total_lines);
  term_puts(num_buf);
  term_puts(" | j/k:scroll d/u:page Tab:link Enter:open b:back q:quit ");
  term_reset();
  term_clear_eol();
}

/* Main interaction loop */
event_loop(u)
unsigned char u;
{
  char cmd_buf[64];
  char num_buf[16];
  int screen_lines;

  render_screen();

  while (1) {
    int key = term_getkey();
    screen_lines = term_rows - 2;

    switch (key) {
    /* Vim: scroll down 1 line */
    case 'j':
    case 'J':
    case KEY_DOWN:
      if (doc_top + screen_lines < total_lines) {
        doc_top++;
        render_screen();
      }
      break;

    /* Vim: scroll up 1 line */
    case 'k':
    case 'K':
    case KEY_UP:
      if (doc_top > 0) {
        doc_top--;
        render_screen();
      }
      break;

    /* Vim: half-page down */
    case 'd':
    case KEY_CTRL_D:
      doc_top += screen_lines / 2;
      if (doc_top + screen_lines > total_lines)
        doc_top = total_lines - screen_lines;
      if (doc_top < 0) doc_top = 0;
      render_screen();
      break;

    /* Vim: half-page up */
    case 'u':
    case KEY_CTRL_U:
      doc_top -= screen_lines / 2;
      if (doc_top < 0) doc_top = 0;
      render_screen();
      break;

    /* Vim / Pager: full page down */
    case KEY_CTRL_F:
    case ' ':
    case KEY_PGDN:
      doc_top += screen_lines - 1;
      if (doc_top + screen_lines > total_lines)
        doc_top = total_lines - screen_lines;
      if (doc_top < 0) doc_top = 0;
      render_screen();
      break;

    /* Vim / Pager: full page up */
    case KEY_CTRL_B:
    case KEY_PGUP:
      doc_top -= screen_lines - 1;
      if (doc_top < 0) doc_top = 0;
      render_screen();
      break;

    /* Vim: jump to top */
    case 'g':
      doc_top = 0;
      render_screen();
      break;

    /* Vim: jump to bottom */
    case 'G':
      doc_top = total_lines - screen_lines;
      if (doc_top < 0) doc_top = 0;
      render_screen();
      break;

    /* Redraw and re-detect terminal size */
    case KEY_CTRL_L:
      term_detect_size();
      ensure_link_visible();
      render_screen();
      break;

    /* Tab: move selection to next URL/link */
    case KEY_TAB:
      if (total_links > 0) {
        cur_link = (cur_link + 1) % total_links;
        ensure_link_visible();
        render_screen();
      }
      break;

    /* Shift-Tab / 'p': move selection to previous URL/link */
    case KEY_BTAB:
    case 'p':
    case 'P':
      if (total_links > 0) {
        cur_link = (cur_link - 1 + total_links) % total_links;
        ensure_link_visible();
        render_screen();
      }
      break;

    /* Enter: open / follow selected URL */
    case KEY_CR:
    case KEY_LF:
      if (total_links > 0 && cur_link >= 0 && cur_link < total_links) {
        int lid = links[cur_link].id;
        strcpy(cmd_buf, "AT+GO=");
        fmt_int(num_buf, lid);
        strcat(cmd_buf, num_buf);
        fetch_stream(u, cmd_buf);
        ensure_link_visible();
        render_screen();
      }
      break;

    /* Back in history */
    case 'b':
    case 'B':
    case 'H':
      fetch_stream(u, "AT+BACK");
      ensure_link_visible();
      render_screen();
      break;

    /* Reload current page */
    case 'r':
    case 'R':
      if (current_url[0]) {
        strcpy(cmd_buf, "AT+GEMINI=\"");
        strcat(cmd_buf, current_url);
        strcat(cmd_buf, "\"");
        fetch_stream(u, cmd_buf);
        ensure_link_visible();
        render_screen();
      }
      break;

    /* Quit */
    case 'q':
    case 'Q':
    case KEY_CTRL_C:
      term_goto(term_rows, 1);
      term_reset();
      term_clear_eol();
      term_puts("\r\n");
      return;
    }
  }
}

main()
{
  char arg1[128];
  char arg2[128];
  char fetch_cmd[160];
  char *url_ptr;
  char *p;
  unsigned char dev_cnt;
  unsigned char dtype, dmode, dbase;

  printf("\nZGMI v1.0 - Gemini Protocol Client (ZNetSuite)\n");

  if (!hbios_detect()) {
    printf("Error: RomWBW HBIOS not detected.\n");
    return 1;
  }

  dev_cnt = (unsigned char)hbios_ciocnt();
  target_unit = 1; /* Default to auxiliary serial port (Port B / Pico W) */

  /* Parse CP/M command tail */
  if (fmt_cpm_arg(tail, 0, arg1, sizeof(arg1)) <= 0 ||
      arg1[0] == '?' || (arg1[0] == '/' && arg1[1] == '?')) {
    printf("Usage: ZGMI [-u unit] <url>\n");
    printf("Examples:\n");
    printf("  ZGMI gemini://geminiprotocol.net\n");
    printf("  ZGMI gemini.circumlunar.space\n");
    printf("  ZGMI -u 1 geminiprotocol.net\n");
    return 0;
  }

  /* Check for optional -u unit flag */
  if (strcmp(arg1, "-u") == 0 || strcmp(arg1, "-U") == 0) {
    if (fmt_cpm_arg(tail, 1, arg2, sizeof(arg2)) <= 0 ||
        fmt_cpm_arg(tail, 2, arg1, sizeof(arg1)) <= 0) {
      printf("Error: Missing unit or URL for -u option.\n");
      return 1;
    }
    target_unit = (unsigned char)fmt_parse_hex(arg2);
    url_ptr = arg1;
  } else {
    url_ptr = arg1;
  }

  if (target_unit >= dev_cnt) {
    printf("Error: Device unit %02X out of range (system has %d CIO units)\n",
           (int)target_unit, (int)dev_cnt);
    return 1;
  }

  if (hbios_device(target_unit, &dtype, &dmode, &dbase) != 0) {
    printf("Error: Device unit %02X not available.\n", (int)target_unit);
    return 1;
  }

  /* Normalize URL: convert scheme and host portion to lowercase */
  p = url_ptr;
  if (strncasecmp(p, "GEMINI://", 9) == 0) {
    p += 9;
  }
  while (*p && *p != '/') {
    if (*p >= 'A' && *p <= 'Z') {
      *p = *p + ('a' - 'A');
    }
    p++;
  }

  if (strncasecmp(url_ptr, "GEMINI://", 9) != 0) {
    strcpy(current_url, "gemini://");
    strcat(current_url, url_ptr);
  } else {
    strcpy(current_url, "gemini://");
    strcat(current_url, url_ptr + 9);
  }

  fmt_hex8(arg2, target_unit, 0);
  printf("Connecting to bridge on CIO unit %s (%s)...\n",
         arg2, hbios_devname(dtype));
  if (!bridge_init(target_unit)) {
    printf("Warning: No response to AT check; continuing anyway...\n");
  }

  /* Query VT100 / ANSI terminal dimensions via DSR CPR */
  term_detect_size();

  /* Clear screen */
  term_cls();

  /* Prepare initial fetch command */
  strcpy(fetch_cmd, "AT+GEMINI=\"");
  strcat(fetch_cmd, current_url);
  strcat(fetch_cmd, "\"");

  fetch_stream(target_unit, fetch_cmd);
  ensure_link_visible();

  event_loop(target_unit);

  return 0;
}

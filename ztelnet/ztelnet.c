/*
 * ZTELNET - Minimal Telnet Client for RomWBW CP/M
 *
 * Connects to remote Telnet servers and BBSes over any Hayes AT-compatible
 * serial-to-Wi-Fi modem (ttl2wifi, ZiModem, WiModem, tcpser) via ATDT<host>:<port>
 * and performs RFC 854 IAC negotiation directly on the Z80.
 *
 * Features:
 * - RFC 854 IAC negotiation (BINARY, ECHO, SGA, TTYPE, NAWS window size)
 * - Automatic VT100 terminal dimension detection (CPR query) for NAWS
 * - Classic Ctrl+] (0x1D) local escape menu (Ctrl+C passes through to remote)
 * - Automatic hangup via +++ / ATH and remote "NO CARRIER" detection
 * - Plain-text configuration file (ZTELNET.CFG) + CLI overrides
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "ztelnet.h"
#include "cio.h"
#include "cpmbdos.h"
#include "fmt.h"
#include "term.h"
#include <string.h>

/* Global configuration & session state */
unsigned char target_unit = 1;
unsigned int cfg_port = 23;
unsigned char cfg_escape = DEFAULT_ESC;
int local_echo = 0;
int crlf_mode = 0;
char cfg_term[16] = "VT100";
char cfg_host[64] = "";

/* Static buffers to keep Z80 stack frames under 128 bytes */
static struct cpm_fcb cfg_fcb;
static unsigned char cfg_dma[128];
static char rx_line[160];
static char val_buf[64];

/* CP/M command tail at 0x0080 */
static unsigned char *tail = (unsigned char *)0x0080;

/* Copy up to maxlen-1 chars and null-terminate */
static copy_str(dst, src, maxlen) char *dst;
char *src;
int maxlen;
{
  strncpy(dst, src, maxlen - 1);
  dst[maxlen - 1] = '\0';
}

/* Parse a single key=value line from ZTELNET.CFG */
static parse_config_line(line) char *line;
{
  char *eq;
  char *key;
  char *val;
  char *c;
  int len;

  for (c = line; *c; c++) {
    if (*c == '#' || *c == ';') {
      *c = '\0';
      break;
    }
  }

  line = fmt_trim(line);
  if (!line[0])
    return;

  eq = strchr(line, '=');
  if (!eq)
    return;
  *eq = '\0';
  key = fmt_lwr(fmt_trim(line));
  val = fmt_trim(eq + 1);

  if (val[0] == '"') {
    len = strlen(val);
    if (len >= 2 && val[len - 1] == '"') {
      val[len - 1] = '\0';
      val++;
    }
  }

  if (strcmp(key, "unit") == 0) {
    target_unit = (unsigned char)fmt_parse_num(val);
  } else if (strcmp(key, "port") == 0) {
    cfg_port = (unsigned int)fmt_parse_num(val);
  } else if (strcmp(key, "term") == 0 && val[0]) {
    copy_str(cfg_term, val, sizeof(cfg_term));
    fmt_upr(cfg_term);
  } else if (strcmp(key, "host") == 0) {
    copy_str(cfg_host, val, sizeof(cfg_host));
  } else if (strcmp(key, "echo") == 0) {
    local_echo = fmt_parse_num(val) ? 1 : 0;
  } else if (strcmp(key, "crlf") == 0) {
    crlf_mode = fmt_parse_num(val) ? 1 : 0;
  }
}

/* Load configuration file using CP/M BDOS sequential read */
static int load_config_file(fname) char *fname;
{
  int pos = 0;
  int i;
  unsigned char ch;

  cpm_fcb_init(&cfg_fcb, fname);
  if (cpm_bdos(BDOS_OPEN, &cfg_fcb) == 0xFF)
    return 0;

  cpm_bdos(BDOS_DMA, cfg_dma);
  while (cpm_bdos(BDOS_READ, &cfg_fcb) == 0) {
    for (i = 0; i < 128; i++) {
      ch = cfg_dma[i];
      if (ch == 0x1A)
        goto done;
      if (ch == '\r')
        continue;
      if (ch == '\n') {
        rx_line[pos] = '\0';
        parse_config_line(rx_line);
        pos = 0;
      } else if (pos < 158) {
        rx_line[pos++] = (char)ch;
      }
    }
  }

done:
  if (pos > 0) {
    rx_line[pos] = '\0';
    parse_config_line(rx_line);
  }
  cpm_bdos(BDOS_CLOSE, &cfg_fcb);
  return 1;
}

/* Parse "[telnet://]host[:port][/]" into cfg_host and cfg_port */
static parse_target(arg) char *arg;
{
  char *p;
  char *colon;
  int len;

  p = fmt_trim(arg);
  if (p[0] && p[1] && p[2] && p[3] && p[4] && p[5] && p[6] == ':' &&
      p[7] == '/' && p[8] == '/') {
    p += 9;
  }

  len = strlen(p);
  while (len > 0 && p[len - 1] == '/') {
    p[--len] = '\0';
  }

  colon = strchr(p, ':');
  if (colon) {
    *colon = '\0';
    if (colon[1]) {
      cfg_port = (unsigned int)fmt_parse_num(colon + 1);
    }
  }

  copy_str(cfg_host, p, sizeof(cfg_host));
}

/* Escape online modem mode via +++ and send ATH */
static bridge_hangup(u) unsigned char u;
{
  int attempt;

  for (attempt = 0; attempt < 2; attempt++) {
    /* Guard time pause while draining incoming data */
    while (cio_get_timeout(u, 30000L, 0, -1) >= 0)
      ;
    cio_puts(u, "+++");
    if (cio_wait_ok(u, 35000L))
      break;
  }

  cio_send_cmd(u, "ATH");
  cio_wait_ok(u, 20000L);
  cio_flush(u);
}

/* Initialize AT modem: sync AT, disable echo (ATE0), disable bridge pager */
static int bridge_init(u) unsigned char u;
{
  int attempt;
  int ok = 0;

  cio_flush(u);
  for (attempt = 0; attempt < 3; attempt++) {
    cio_send_cmd(u, "AT");
    if (cio_wait_ok(u, 15000L)) {
      ok = 1;
      break;
    }
  }

  if (!ok) {
    /* Modem might still be online from an interrupted session */
    bridge_hangup(u);
    cio_send_cmd(u, "AT");
    ok = cio_wait_ok(u, 15000L);
  }

  cio_send_cmd(u, "ATE0");
  cio_wait_ok(u, 15000L);

  /* Disable ttl2wifi screen pager if supported (ignored on generic modems) */
  cio_send_cmd(u, "AT+PAGE=0");
  cio_wait_ok(u, 15000L);

  cio_flush(u);
  return ok;
}

/* Dial target host:port via ATDT<host>:<port> and wait for CONNECT */
static int bridge_dial(u) unsigned char u;
{
  int res;
  char *line;

  cio_printf("Dialing %s:%u... [Ctrl+C to abort]\n", cfg_host, cfg_port);
  cio_flush(u);

  cio_puts(u, "ATDT");
  cio_puts(u, cfg_host);
  cio_out(u, ':');
  fmt_uint(val_buf, cfg_port);
  cio_send_cmd(u, val_buf);

  for (;;) {
    res = cio_readline(u, rx_line, sizeof(rx_line), 150000L, CIO_CONSOLE,
                       KEY_CTRL_C);
    if (res == -2) {
      cio_out(u, '\r');
      cio_flush(u);
      cio_printf("\nAborted by user.\n");
      return 0;
    }
    if (res < 0) {
      cio_printf("Error: Connection timed out.\n");
      return 0;
    }

    line = fmt_trim(rx_line);
    if (!line[0] || strncmp(line, "ATDT", 4) == 0)
      continue;

    if (strncmp(line, "CONNECT", 7) == 0) {
      cio_printf("Connected (%s, %dx%d). Escape is Ctrl+].\n\n", cfg_term,
                 term_cols, term_rows);
      return 1;
    }

    if (strncmp(line, "NO ", 3) == 0 || strncmp(line, "ERROR", 5) == 0 ||
        strncmp(line, "BUSY", 4) == 0) {
      cio_printf("%s\n", line);
      return 0;
    }
  }
}

/* Handle local Ctrl+] escape menu; returns 1 to disconnect and quit */
static int handle_escape(u) unsigned char u;
{
  int key;

  cio_printf("\r\n\033[7m zTelnet> [q]uit [c]ont [s]end ^] [e]cho:%s [l]f:%s "
             "[n]aws \033[0m ",
             local_echo ? "ON" : "OFF", crlf_mode ? "CRLF" : "CR");

  while (!cio_ist(CIO_CONSOLE))
    ;
  key = cio_in(CIO_CONSOLE) & 0xFF;

  /* Erase menu line on console */
  cio_cputs("\r\033[K");

  if (key == 'q' || key == 'Q') {
    cio_printf("Disconnecting...\n");
    bridge_hangup(u);
    return 1;
  }
  if (key == 's' || key == 'S' || (unsigned char)key == cfg_escape) {
    tn_tx_byte(u, cfg_escape);
  } else if (key == 'e' || key == 'E') {
    local_echo = !local_echo;
  } else if (key == 'l' || key == 'L') {
    crlf_mode = !crlf_mode;
  } else if (key == 'n' || key == 'N') {
    term_detect_size();
    tn_send_naws(u);
  }
  return 0;
}

/* Main full-duplex Telnet session loop */
static telnet_loop(u) unsigned char u;
{
  int ch;
  int burst;

  tn_init();

  for (;;) {
    /* 1. Drain incoming bytes from modem with priority */
    burst = 0;
    while (cio_ist(u) && burst < 64) {
      ch = cio_in(u) & 0xFF;
      if (tn_rx_byte(u, (unsigned char)ch)) {
        return; /* Remote NO CARRIER detected */
      }
      burst++;
    }

    /* 2. Check console for user keystrokes */
    if (cio_ist(CIO_CONSOLE)) {
      ch = cio_in(CIO_CONSOLE) & 0xFF;
      if ((unsigned char)ch == cfg_escape) {
        if (handle_escape(u))
          return;
      } else {
        tn_tx_byte(u, (unsigned char)ch);
      }
    }
  }
}

static usage() {
  cio_printf(
      "Usage: ZTELNET [-u unit] [-p port] [-t term] [-e] [-c cfg] <host[:port]> [port]\n"
      "Options:\n"
      "  -u <unit>   RomWBW CIO serial unit for modem (default: 1)\n"
      "  -p <port>   Remote TCP port (default: 23)\n"
      "  -t <term>   Terminal type for TTYPE (default: VT100)\n"
      "  -e          Enable local character echo\n"
      "  -c <file>   Config file (default: ZTELNET.CFG, A:ZTELNET.CFG)\n"
      "Examples:\n"
      "  ZTELNET bbs.fozztexx.com\n"
      "  ZTELNET telehack.com 23\n");
}

main() {
  int arg_idx = 0;
  int pos_arg = 0;
  unsigned char dev_cnt;
  unsigned char dtype, dmode, dbase;
  char opt;

  cio_printf("\nZTELNET v1.0 - Telnet Client (ZNetSuite)\n");

  if (!hbios_detect()) {
    cio_printf("Error: RomWBW HBIOS not detected.\n");
    return 1;
  }

  /* Load ZTELNET.CFG from current drive, or fallback to A:ZTELNET.CFG */
  if (!load_config_file("ZTELNET.CFG")) {
    load_config_file("A:ZTELNET.CFG");
  }

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
      if (opt == 'e') {
        local_echo = 1;
      } else {
        if (fmt_cpm_arg(tail, ++arg_idx, val_buf, sizeof(val_buf)) <= 0) {
          cio_printf("Error: Missing argument for -%c\n", (int)opt);
          return 1;
        }
        if (opt == 'u') {
          target_unit = (unsigned char)fmt_parse_num(val_buf);
        } else if (opt == 'p') {
          cfg_port = (unsigned int)fmt_parse_num(val_buf);
        } else if (opt == 't') {
          copy_str(cfg_term, val_buf, sizeof(cfg_term));
          fmt_upr(cfg_term);
        } else if (opt == 'c') {
          if (!load_config_file(val_buf)) {
            cio_printf("Error: Could not open %s\n", val_buf);
            return 1;
          }
        }
      }
    } else if (pos_arg == 0) {
      parse_target(rx_line);
      pos_arg++;
    } else if (pos_arg == 1) {
      cfg_port = (unsigned int)fmt_parse_num(rx_line);
      pos_arg++;
    }
    arg_idx++;
  }

  if (!cfg_host[0]) {
    usage();
    return 0;
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

  /* Detect VT100/ANSI console dimensions for NAWS negotiation */
  term_detect_size();

  cio_printf("Initializing modem on CIO unit %02X (%s)...\n",
             (unsigned int)target_unit, hbios_devname(dtype));
  if (!bridge_init(target_unit)) {
    cio_printf("Warning: No response to AT check; continuing anyway...\n");
  }

  if (bridge_dial(target_unit)) {
    telnet_loop(target_unit);
    term_reset();
    term_cursor_show();
    cio_printf("\nZTELNET disconnected.\n");
  }

  return 0;
}

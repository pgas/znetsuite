/*
 * TN_IAC.C - RFC 854 Telnet IAC Negotiation & Stream Processor
 *
 * Implements:
 * - RFC 854 IAC command state machine (DO, DONT, WILL, WONT, SB..SE)
 * - Option negotiation for BINARY (0), ECHO (1), SGA (3), TTYPE (24), NAWS (31)
 * - Dynamic window size reporting (NAWS) from detected VT100 dimensions
 * - In-band "NO CARRIER" hangup detection on line boundaries
 *
 * Part of ZNetSuite (ZTELNET).
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "ztelnet.h"
#include "cio.h"
#include "term.h"

/* Receiver state machine states */
#define ST_DATA        0
#define ST_IAC         1
#define ST_OPT         2
#define ST_SB          3
#define ST_SB_IAC      4

static unsigned char tn_state;
static unsigned char tn_cmd;
static unsigned char sb_buf[16];
static int sb_len;
static int nc_idx;

static char *nc_str = "NO CARRIER";

/* Reset Telnet protocol state before a new session */
tn_init() {
  tn_state = ST_DATA;
  tn_cmd = 0;
  sb_len = 0;
  nc_idx = 0;
}

/* Send a 3-byte Telnet command: IAC <cmd> <opt> */
static tn_send_opt(u, cmd, opt) unsigned char u;
unsigned char cmd;
unsigned char opt;
{
  cio_out(u, TN_IAC);
  cio_out(u, cmd);
  cio_out(u, opt);
}

/* Send a byte inside SB..SE, doubling 0xFF per RFC 855 */
static tn_put_sb_byte(u, b) unsigned char u;
unsigned char b;
{
  cio_out(u, b);
  if (b == TN_IAC) {
    cio_out(u, TN_IAC);
  }
}

/* Send RFC 1073 NAWS subnegotiation: IAC SB NAWS <w_hi> <w_lo> <h_hi> <h_lo> IAC SE */
tn_send_naws(u) unsigned char u;
{
  unsigned int w;
  unsigned int h;

  w = (term_cols > 0) ? (unsigned int)term_cols : 80;
  h = (term_rows > 0) ? (unsigned int)term_rows : 24;

  tn_send_opt(u, TN_SB, OPT_NAWS);
  tn_put_sb_byte(u, (unsigned char)((w >> 8) & 0xFF));
  tn_put_sb_byte(u, (unsigned char)(w & 0xFF));
  tn_put_sb_byte(u, (unsigned char)((h >> 8) & 0xFF));
  tn_put_sb_byte(u, (unsigned char)(h & 0xFF));
  cio_out(u, TN_IAC);
  cio_out(u, TN_SE);
}

/* Send RFC 1091 TERMINAL-TYPE IS subnegotiation: IAC SB TTYPE 0 <term> IAC SE */
static tn_send_ttype(u) unsigned char u;
{
  tn_send_opt(u, TN_SB, OPT_TTYPE);
  cio_out(u, 0); /* IS */
  cio_puts(u, cfg_term);
  cio_out(u, TN_IAC);
  cio_out(u, TN_SE);
}

/* Handle 3-byte option negotiation: IAC <tn_cmd> <opt> */
static tn_handle_opt(u, opt) unsigned char u;
unsigned char opt;
{
  unsigned char reply = 0;

  if (tn_cmd == TN_DO) {
    if (opt == OPT_SGA || opt == OPT_BINARY || opt == OPT_TTYPE ||
        opt == OPT_NAWS) {
      reply = TN_WILL;
    } else {
      reply = TN_WONT;
    }
  } else if (tn_cmd == TN_WILL) {
    if (opt == OPT_ECHO) {
      local_echo = 0;
      reply = TN_DO;
    } else if (opt == OPT_SGA || opt == OPT_BINARY) {
      reply = TN_DO;
    } else {
      reply = TN_DONT;
    }
  } else if (tn_cmd == TN_DONT) {
    reply = TN_WONT;
  } else if (tn_cmd == TN_WONT) {
    reply = TN_DONT;
  }

  if (reply) {
    tn_send_opt(u, reply, opt);
    if (tn_cmd == TN_DO && opt == OPT_NAWS) {
      tn_send_naws(u);
    }
  }
}

/* Transmit one user keystroke to the Telnet stream */
tn_tx_byte(u, b) unsigned char u;
unsigned char b;
{
  if (local_echo) {
    cio_cputc((char)b);
    if (b == '\r' && crlf_mode) {
      cio_cputc('\n');
    }
  }

  cio_out(u, b);
  if (b == TN_IAC) {
    cio_out(u, TN_IAC);
  } else if (b == '\r' && crlf_mode) {
    cio_out(u, '\n');
  }
}

/*
 * Process one incoming byte from the modem serial port.
 * Returns 1 if "NO CARRIER" line was detected, 0 otherwise.
 */
int tn_rx_byte(u, b) unsigned char u;
unsigned char b;
{
  if (tn_state == ST_DATA) {
    if (b == TN_IAC) {
      tn_state = ST_IAC;
    } else {
      cio_cputc((char)b);

      /* Track "NO CARRIER" at the beginning of a line */
      if (b == '\r' || b == '\n') {
        if (nc_idx == 10) {
          return 1;
        }
        nc_idx = 0;
      } else if (nc_idx >= 0 && nc_idx < 10 && (char)b == nc_str[nc_idx]) {
        nc_idx++;
      } else {
        nc_idx = -1;
      }
    }
  } else if (tn_state == ST_IAC) {
    if (b == TN_IAC) {
      cio_cputc((char)TN_IAC);
      tn_state = ST_DATA;
    } else if (b == TN_DO || b == TN_DONT || b == TN_WILL || b == TN_WONT) {
      tn_cmd = b;
      tn_state = ST_OPT;
    } else if (b == TN_SB) {
      sb_len = 0;
      tn_state = ST_SB;
    } else {
      tn_state = ST_DATA;
    }
  } else if (tn_state == ST_OPT) {
    tn_handle_opt(u, b);
    tn_state = ST_DATA;
  } else if (tn_state == ST_SB) {
    if (b == TN_IAC) {
      tn_state = ST_SB_IAC;
    } else if (sb_len < (int)sizeof(sb_buf)) {
      sb_buf[sb_len++] = b;
    }
  } else if (tn_state == ST_SB_IAC) {
    if (b == TN_SE) {
      /* Check for IAC SB TTYPE SEND (24, 1) IAC SE */
      if (sb_len >= 2 && sb_buf[0] == OPT_TTYPE && sb_buf[1] == 1) {
        tn_send_ttype(u);
      }
      tn_state = ST_DATA;
    } else if (b == TN_IAC) {
      if (sb_len < (int)sizeof(sb_buf)) {
        sb_buf[sb_len++] = TN_IAC;
      }
      tn_state = ST_SB;
    } else {
      tn_state = ST_DATA;
    }
  }

  return 0;
}

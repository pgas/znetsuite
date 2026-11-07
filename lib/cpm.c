/*
 * CPM.C - CP/M Command Tail & Hex Parsing
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "fmt.h"

/* Parse hexadecimal string (supports "$1F", "0x1F", or "1F") */
int fmt_parse_hex(s)
char *s;
{
  int val = 0;

  while (*s == ' ' || *s == '\t') s++;

  if (*s == '$') {
    s++;
  } else if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
    s += 2;
  }

  while (*s) {
    char c = *s++;
    if (c >= '0' && c <= '9') {
      val = (val << 4) | (c - '0');
    } else if (c >= 'a' && c <= 'f') {
      val = (val << 4) | (c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
      val = (val << 4) | (c - 'A' + 10);
    } else {
      break;
    }
  }

  return val;
}

/* Extract arg_idx-th (0-based) token from CP/M command tail (buffer at 0x0080) */
int fmt_cpm_arg(tail, arg_idx, buf, maxlen)
unsigned char *tail;
int arg_idx;
char *buf;
int maxlen;
{
  int taillen = tail[0];
  int idx = 1;
  int cur_arg = 0;
  int len = 0;

  while (idx <= taillen) {
    /* Skip whitespace */
    while (idx <= taillen && (tail[idx] == ' ' || tail[idx] == '\t')) {
      idx++;
    }
    if (idx > taillen) break;

    if (cur_arg == arg_idx) {
      /* Extract target token */
      while (idx <= taillen && tail[idx] != ' ' && tail[idx] != '\t' && len < maxlen - 1) {
        buf[len++] = tail[idx++];
      }
      buf[len] = '\0';
      return len;
    }

    /* Skip token */
    while (idx <= taillen && tail[idx] != ' ' && tail[idx] != '\t') {
      idx++;
    }
    cur_arg++;
  }

  buf[0] = '\0';
  return -1;
}

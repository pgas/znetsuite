/*
 * FMT_CPM.C - CP/M Command Tail Argument Parser
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "fmt.h"

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

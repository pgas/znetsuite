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
  unsigned char *p = tail + 1;
  unsigned char *end = p + *tail;
  int len = 0;

  while (p < end) {
    while (p < end && (*p == ' ' || *p == '\t')) {
      p++;
    }
    if (p >= end)
      break;

    if (arg_idx == 0) {
      while (p < end && *p != ' ' && *p != '\t') {
        if (len < maxlen - 1) {
          buf[len++] = (char)*p;
        }
        p++;
      }
      buf[len] = '\0';
      return len;
    }

    while (p < end && *p != ' ' && *p != '\t') {
      p++;
    }
    arg_idx--;
  }

  buf[0] = '\0';
  return -1;
}


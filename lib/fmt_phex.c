/*
 * FMT_PHEX.C - Parse Hexadecimal Number
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "fmt.h"

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

/*
 * FMT_PINT.C - Parse Signed Decimal Integer
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "fmt.h"

int fmt_parse_int(s)
char *s;
{
  int val = 0;
  int neg = 0;

  while (*s == ' ' || *s == '\t') s++;

  if (*s == '-') {
    neg = 1;
    s++;
  } else if (*s == '+') {
    s++;
  }

  while (*s >= '0' && *s <= '9') {
    val = val * 10 + (*s++ - '0');
  }

  return neg ? -val : val;
}

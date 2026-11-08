/*
 * FMT_PNUM.C - Parse Auto-detected Number (Hex or Decimal)
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "fmt.h"

int fmt_parse_num(s)
char *s;
{
  char *p = s;

  while (*p == ' ' || *p == '\t') p++;

  if (*p == '$' || (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))) {
    return fmt_parse_hex(p);
  }

  while (*p) {
    if ((*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')) {
      return fmt_parse_hex(s);
    }
    p++;
  }

  return fmt_parse_int(s);
}

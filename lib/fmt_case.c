/*
 * FMT_CASE.C - In-place Lowercase String Conversion
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "fmt.h"

char *fmt_lwr(s)
char *s;
{
  char *p = s;
  while (*p) {
    if (*p >= 'A' && *p <= 'Z') {
      *p = *p + ('a' - 'A');
    }
    p++;
  }
  return s;
}

/*
 * FMT_UPR.C - In-place Uppercase String Conversion
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "fmt.h"

char *fmt_upr(s)
char *s;
{
  char *p = s;
  while (*p) {
    if (*p >= 'a' && *p <= 'z') {
      *p = *p - ('a' - 'A');
    }
    p++;
  }
  return s;
}


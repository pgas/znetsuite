/*
 * FMT_TRIM.C - In-place String Trimming
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "fmt.h"
#include <string.h>

char *fmt_trim(s)
char *s;
{
  char *start = s;
  char *end;

  while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') {
    start++;
  }

  if (start != s) {
    char *d = s;
    while (*start) {
      *d++ = *start++;
    }
    *d = '\0';
  }

  end = s + strlen(s) - 1;
  while (end >= s && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
    *end-- = '\0';
  }

  return s;
}

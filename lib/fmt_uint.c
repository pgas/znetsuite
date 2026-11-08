/*
 * FMT_UINT.C - Format Unsigned Integer
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "fmt.h"

char *fmt_uint(buf, val)
char *buf;
unsigned int val;
{
  char tmp[10];
  int i = 0;
  int j = 0;

  if (val == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return buf;
  }

  while (val > 0) {
    tmp[i++] = (char)('0' + (val % 10));
    val /= 10;
  }

  while (i > 0) {
    buf[j++] = tmp[--i];
  }
  buf[j] = '\0';

  return buf;
}


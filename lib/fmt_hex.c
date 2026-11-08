/*
 * FMT_HEX.C - Format Hexadecimal (8-bit and 16-bit)
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "fmt.h"

static char hex_digits[] = "0123456789ABCDEF";

char *fmt_hex8(buf, val, prefix)
char *buf;
unsigned char val;
int prefix;
{
  int j = 0;

  if (prefix) {
    buf[j++] = '0';
    buf[j++] = 'x';
  }

  buf[j++] = hex_digits[(val >> 4) & 0x0F];
  buf[j++] = hex_digits[val & 0x0F];
  buf[j] = '\0';

  return buf;
}

char *fmt_hex16(buf, val, prefix)
char *buf;
unsigned int val;
int prefix;
{
  int j = 0;

  if (prefix) {
    buf[j++] = '0';
    buf[j++] = 'x';
  }

  buf[j++] = hex_digits[(val >> 12) & 0x0F];
  buf[j++] = hex_digits[(val >> 8) & 0x0F];
  buf[j++] = hex_digits[(val >> 4) & 0x0F];
  buf[j++] = hex_digits[val & 0x0F];
  buf[j] = '\0';

  return buf;
}

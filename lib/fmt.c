/*
 * FMT.C - Number Parsing & Text Formatting Library
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include <string.h>
#include "fmt.h"
#include "cpm.c"

static char hex_digits[] = "0123456789ABCDEF";

/* Parse signed decimal integer */
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

/* Parse auto-detected number (hex if prefixed with $ / 0x or has a-f, else decimal) */
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

/* Format signed integer into buffer; returns buf pointer */
char *fmt_int(buf, val)
char *buf;
int val;
{
  char tmp[10];
  int i = 0;
  int j = 0;

  if (val == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return buf;
  }

  if (val < 0) {
    buf[j++] = '-';
    val = -val;
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

/* Format unsigned integer into buffer; returns buf pointer */
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

/* Format 8-bit hex (e.g. "1A" or "0x1A") */
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

/* Format 16-bit hex (e.g. "0100" or "0x0100") */
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

/* Format human-readable byte size (e.g. "512 B", "24 KB", "2 MB") */
char *fmt_size(buf, bytes)
char *buf;
long bytes;
{
  char num[16];

  if (bytes < 1024L) {
    fmt_int(num, (int)bytes);
    strcpy(buf, num);
    strcat(buf, " B");
  } else if (bytes < 1048576L) {
    fmt_int(num, (int)(bytes / 1024L));
    strcpy(buf, num);
    strcat(buf, " KB");
  } else {
    fmt_int(num, (int)(bytes / 1048576L));
    strcpy(buf, num);
    strcat(buf, " MB");
  }

  return buf;
}

/* Remove leading and trailing whitespace in place */
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

/* Convert string to lowercase in place */
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

/* Convert string to uppercase in place */
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

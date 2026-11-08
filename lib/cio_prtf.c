/*
 * CIO_PRTF.C - Direct Console Formatted Print (Zero STDIO dependency)
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"
#include <stdarg.h>

/* Console output with LF -> CRLF translation */
void cio_cputc(c)
char c;
{
  if (c == '\n')
    cio_out(CIO_CONSOLE, '\r');
  cio_out(CIO_CONSOLE, (unsigned char)c);
}

/* Write null-terminated string to console */
void cio_cputs(s)
char *s;
{
  while (*s) {
    cio_cputc(*s++);
  }
}

void cio_cputhex(val, width)
unsigned int val;
int width;
{
  static char hex[] = "0123456789ABCDEF";
  int digits = 1;
  if (val > 0xFFF) digits = 4;
  else if (val > 0xFF) digits = 3;
  else if (val > 0xF) digits = 2;
  if (width < digits) width = digits;
  if (width > 2) {
    cio_cputc(hex[(val >> 12) & 0x0F]);
    cio_cputc(hex[(val >> 8) & 0x0F]);
  }
  if (width > 1) {
    cio_cputc(hex[(val >> 4) & 0x0F]);
  }
  cio_cputc(hex[val & 0x0F]);
}

void cio_cputdec(val)
int val;
{
  char buf[10];
  int i = 0;
  if (val == 0) {
    cio_cputc('0');
    return;
  }
  if (val < 0) {
    cio_cputc('-');
    val = -val;
  }
  while (val > 0) {
    buf[i++] = (char)('0' + (val % 10));
    val /= 10;
  }
  while (i > 0) {
    cio_cputc(buf[--i]);
  }
}

void cio_printf(fmt)
char *fmt;
{
  va_list ap;
  char *p;
  char *s;
  int width, left_align;

  va_start(ap, fmt);
  for (p = fmt; *p; p++) {
    if (*p != '%') {
      cio_cputc(*p);
      continue;
    }
    p++;
    left_align = 0;
    width = 0;
    if (*p == '-') {
      left_align = 1;
      p++;
    } else if (*p == '0') {
      p++;
    }
    while (*p >= '0' && *p <= '9') {
      width = width * 10 + (*p++ - '0');
    }
    if (*p == 's') {
      s = va_arg(ap, char *);
      if (!s) s = "(null)";
      if (left_align && width > 0) {
        int len = 0;
        while (*s) { cio_cputc(*s++); len++; }
        while (len++ < width) cio_cputc(' ');
      } else {
        cio_cputs(s);
      }
    } else if (*p == 'd' || *p == 'u') {
      cio_cputdec(va_arg(ap, int));
    } else if (*p == 'x' || *p == 'X') {
      cio_cputhex(va_arg(ap, unsigned int), width ? width : 2);
    } else if (*p == 'c') {
      cio_cputc((char)va_arg(ap, int));
    } else if (*p == '%') {
      cio_cputc('%');
    } else {
      cio_cputc('%');
      cio_cputc(*p);
    }
  }
  va_end(ap);
}

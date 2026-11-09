/*
 * CIO_PRTF.C - Direct Console Formatted Print (Zero STDIO dependency)
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"
#include <stdarg.h>

/* Console output with LF -> CRLF translation */
void cio_cputc(c) char c;
{
  if (c == '\n')
    cio_out(CIO_CONSOLE, '\r');
  cio_out(CIO_CONSOLE, (unsigned char)c);
}

/* Write null-terminated string to console */
void cio_cputs(s) char *s;
{
  while (*s) {
    cio_cputc(*s++);
  }
}

void cio_cputhex(val, width) unsigned int val;
int width;
{
  unsigned char d;
  if (val > 0x0F || width > 1) {
    cio_cputhex(val >> 4, width - 1);
  }
  d = (unsigned char)(val & 0x0F);
  cio_cputc((char)(d < 10 ? '0' + d : 'A' - 10 + d));
}

void cio_cputdec(val) int val;
{
  unsigned int u;
  if (val < 0) {
    cio_cputc('-');
    u = (unsigned int)(-val);
  } else {
    u = (unsigned int)val;
  }
  if (u >= 10) {
    cio_cputdec((int)(u / 10));
  }
  cio_cputc((char)('0' + (u % 10)));
}

void cio_printf(fmt) char *fmt;
{
  va_list ap;
  char *p;
  char *s;
  int width, left_align;
  char c;

  va_start(ap, fmt);
  for (p = fmt; (c = *p) != '\0'; p++) {
    if (c != '%') {
      cio_cputc(c);
      continue;
    }
    c = *++p;
    left_align = 0;
    width = 0;
    if (c == '-') {
      left_align = 1;
      c = *++p;
    } else if (c == '0') {
      c = *++p;
    }
    while (c >= '0' && c <= '9') {
      width = width * 10 + (c - '0');
      c = *++p;
    }
    if (c == 's') {
      s = va_arg(ap, char *);
      if (s) {
        while (*s) {
          cio_cputc(*s++);
          width--;
        }
        if (left_align) {
          while (width-- > 0)
            cio_cputc(' ');
        }
      }
    } else if (c == 'd' || c == 'u') {
      cio_cputdec(va_arg(ap, int));
    } else if (c == 'x' || c == 'X') {
      cio_cputhex(va_arg(ap, unsigned int), width ? width : 2);
    } else if (c == 'c') {
      cio_cputc((char)va_arg(ap, int));
    } else if (c == '%') {
      cio_cputc('%');
    } else if (c) {
      cio_cputc('%');
      cio_cputc(c);
    } else {
      break;
    }
  }
  va_end(ap);
}

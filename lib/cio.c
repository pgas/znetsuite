/*
 * CIO.C - RomWBW Character I/O & Serial Line Interface
 *
 * 100% Pure RomWBW HBIOS (RST 8) Character I/O implementation.
 * Binary compatible with all RomWBW systems:
 *   - SC131 Z180 Pocket Computer (ASCI0 console, ASCI1 aux bridge)
 *   - SimH AltairZ80 emulator (SSER0 console, ACIA0 aux bridge)
 *   - RC2014, Zeta 2, N8VEM, etc.
 *
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"
#include <stdarg.h>

/* Check RomWBW signature 'W' and ~'W' via pointer at 0xFFFC */
int hbios_detect() {
  unsigned char **id_ptr;
  unsigned char *id;
  id_ptr = (unsigned char **)0xFFFC;
  id = *id_ptr;
  if (id && id[0] == 'W' && id[1] == (unsigned char)(~'W' & 0xFF))
    return 1;
  return 0;
}

/* Get number of character I/O devices */
int hbios_ciocnt() {
  /* clang-format off */
#asm
	ld	b,0F8H		; BF_SYSGET
	ld	c,0		; BF_SYSGET_CIOCNT
	rst	8
	ld	l,e
	ld	h,0
#endasm
  /* clang-format on */
}

/* Query device attributes: D=type, C=mode/attrs, L=base port */
int hbios_device(u, dtype, dmode, dbase)
unsigned char u;
unsigned char *dtype;
unsigned char *dmode;
unsigned char *dbase;
{
  /* clang-format off */
#asm
	ld	a,(ix+6)	; unit u
	ld	c,a
	ld	b,06H		; BF_CIODEVICE
	rst	8
	or	a
	jr	nz,hb_dev_err
	push	hl		; save HL (L has base port)
	ld	a,d		; D has device type
	ld	l,(ix+8)
	ld	h,(ix+9)
	ld	(hl),a
	ld	a,c		; C has attributes
	ld	l,(ix+10)
	ld	h,(ix+11)
	ld	(hl),a
	pop	de		; E has base port (from L)
	ld	a,e
	ld	l,(ix+12)
	ld	h,(ix+13)
	ld	(hl),a
	ld	hl,0
	jr	hb_dev_done
hb_dev_err:
	ld	hl,1
hb_dev_done:
#endasm
  /* clang-format on */
}

/* Return human-readable string for character device type */
char *hbios_devname(type)
unsigned char type;
{
  switch (type) {
  case 0x00: return "UART (16C550)";
  case 0x01: return "ASCI (Z180)";
  case 0x02: return "TERM (ANSI)";
  case 0x03: return "PRPCON (PropIO)";
  case 0x04: return "PPPCON (ParPortProp)";
  case 0x05: return "SIO (Zilog)";
  case 0x06: return "ACIA (68B50)";
  case 0x07: return "PIO (Parallel)";
  case 0x08: return "UF (USB FIFO)";
  case 0x09: return "DUART (2681)";
  case 0x0A: return "Z2U (Z280)";
  case 0x0B: return "LPT (Parallel)";
  case 0x0C: return "ESPCON (ESP)";
  case 0x0D: return "ESPSER (ESP)";
  case 0x0E: return "SCON (S100)";
  case 0x0F: return "SSER (SimH)";
  case 0x10: return "EZ80UART (eZ80)";
  case 0x11: return "DLPSER";
  case 0x12: return "TSER";
  case 0x13: return "SCC (Z85C30)";
  default:   return "UNKNOWN";
  }
}

/* Check if byte is available from unit (or CIO_CONSOLE 0x80) */
int cio_ist(u)
unsigned char u;
{
  /* clang-format off */
#asm
	ld	a,(ix+6)
	ld	c,a
	ld	b,02H		; BF_CIOIST
	rst	8
	ld	l,a
	ld	h,0
#endasm
  /* clang-format on */
}

/* Check if unit is ready to transmit byte */
int cio_ost(u)
unsigned char u;
{
  /* clang-format off */
#asm
	ld	a,(ix+6)
	ld	c,a
	ld	b,03H		; BF_CIOOST
	rst	8
	ld	l,a
	ld	h,0
#endasm
  /* clang-format on */
}

/* Read byte from unit (blocking) */
int cio_in(u)
unsigned char u;
{
  /* clang-format off */
#asm
	ld	a,(ix+6)
	ld	c,a
	ld	b,00H		; BF_CIOIN
	rst	8
	ld	l,e
	ld	h,0
#endasm
  /* clang-format on */
}

/* Write byte to unit */
cio_out(u, ch) unsigned char u;
unsigned char ch;
{
  /* clang-format off */
#asm
	ld	a,(ix+6)
	ld	c,a
	ld	a,(ix+8)
	ld	e,a
	ld	b,01H		; BF_CIOOUT
	rst	8
#endasm
  /* clang-format on */
}

/* Write null-terminated string to unit */
cio_puts(u, s) unsigned char u;
char *s;
{
  while (*s) {
    cio_out(u, (unsigned char)*s++);
  }
}

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

/* Write hexadecimal number to console */
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

/* Write decimal integer to console */
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

/* Lightweight console printf (no FILE*, no stdio buffer bloat) */
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

/* Print table of available character devices */
hbios_list_devices(cnt) unsigned char cnt;
{
  unsigned char u, type, mode, base;

  cio_printf("\nAvailable Character Devices (%d):\n", (int)cnt);
  cio_printf(" Unit | Type                 | Base Port\n");
  cio_printf("------+----------------------+----------\n");
  for (u = 0; u < cnt; u++) {
    if (hbios_device(u, &type, &mode, &base) == 0) {
      cio_printf("  %02X  | %-20s |    %02X\n", (unsigned int)u,
                 hbios_devname(type), (unsigned int)base);
    }
  }
  cio_printf("\n");
}

/* Write byte buffer of length len to unit */
cio_write(u, buf, len) unsigned char u;
char *buf;
int len;
{
  int i;
  for (i = 0; i < len; i++) {
    cio_out(u, (unsigned char)buf[i]);
  }
}

/* Flush pending bytes from unit */
cio_flush(u) unsigned char u;
{
  unsigned int count;
  for (count = 0; count < 10000; count++) {
    if (cio_ist(u)) {
      cio_in(u);
      count = 0;
    }
  }
}

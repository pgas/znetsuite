/*
 * HB_DEV.C - RomWBW Character I/O Device Query
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"

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


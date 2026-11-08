/*
 * CIO_IN.C - RomWBW Character Input
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"

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

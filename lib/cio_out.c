/*
 * CIO_OUT.C - RomWBW Character Output
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"

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

/*
 * CIO_OST.C - RomWBW Character Output Status
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"

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

/*
 * HB_CNT.C - RomWBW Character I/O Device Count Query
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"

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


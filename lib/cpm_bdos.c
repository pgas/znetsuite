/*
 * CPM_BDOS.C - Direct CP/M BDOS Entry Call (0x0005)
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cpmbdos.h"

int cpm_bdos(func, parm)
int func;
void *parm;
{
  /* clang-format off */
#asm
	ld	c,(ix+6)
	ld	e,(ix+8)
	ld	d,(ix+9)
	call	0005H
	ld	l,a
	ld	h,0
#endasm
  /* clang-format on */
}


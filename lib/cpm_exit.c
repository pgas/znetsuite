/*
 * CPM_EXIT.C - Minimal CP/M Runtime Exit & RSX Stubs
 *
 * Overrides LIBC.LIB's exit.obj, cleanup.obj, close.obj, and sys_err.obj
 * to avoid pulling ~1.2 KB of unused Unix FD tables (__fcb) and error
 * strings (_sys_err) into ZNetSuite binaries.
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

_initrsx() {}

exit(code) int code;
{
/* clang-format off */
#asm
  rst 0
#endasm
/* clang-format on */
}


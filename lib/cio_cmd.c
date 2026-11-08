/*
 * CIO_CMD.C - Send AT Command Line with CRLF
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"

cio_send_cmd(u, cmd) unsigned char u;
char *cmd;
{
  cio_puts(u, cmd);
  cio_out(u, '\r');
  cio_out(u, '\n');
}

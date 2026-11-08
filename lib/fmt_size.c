/*
 * FMT_SIZE.C - Format Human-Readable Byte Size
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "fmt.h"
#include <string.h>

char *fmt_size(buf, bytes)
char *buf;
long bytes;
{
  char num[16];

  if (bytes < 1024L) {
    fmt_int(num, (int)bytes);
    strcpy(buf, num);
    strcat(buf, " B");
  } else if (bytes < 1048576L) {
    fmt_int(num, (int)(bytes / 1024L));
    strcpy(buf, num);
    strcat(buf, " KB");
  } else {
    fmt_int(num, (int)(bytes / 1048576L));
    strcpy(buf, num);
    strcat(buf, " MB");
  }

  return buf;
}

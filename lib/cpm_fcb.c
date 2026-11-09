/*
 * CPM_FCB.C - Initialize 36-byte CP/M FCB from "[d:]filename.ext"
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cpmbdos.h"
#include <string.h>

cpm_fcb_init(fcb, fname) struct cpm_fcb *fcb;
char *fname;
{
  int i;
  char *p = fname;
  char c;

  memset(fcb, 0, sizeof(struct cpm_fcb));
  while (*p == ' ' || *p == '\t')
    p++;

  if (p[0] && p[1] == ':') {
    c = p[0];
    if (c >= 'a' && c <= 'z')
      c -= ('a' - 'A');
    if (c >= 'A' && c <= 'P')
      fcb->dr = (unsigned char)(c - 'A' + 1);
    p += 2;
  }

  for (i = 0; i < 8; i++) {
    if (*p && *p != '.' && *p != ' ' && *p != '\t') {
      c = *p++;
      if (c >= 'a' && c <= 'z')
        c -= ('a' - 'A');
      fcb->name[i] = c;
    } else {
      fcb->name[i] = ' ';
    }
  }
  while (*p && *p != '.' && *p != ' ' && *p != '\t')
    p++;
  if (*p == '.')
    p++;

  for (i = 0; i < 3; i++) {
    if (*p && *p != ' ' && *p != '\t') {
      c = *p++;
      if (c >= 'a' && c <= 'z')
        c -= ('a' - 'A');
      fcb->ft[i] = c;
    } else {
      fcb->ft[i] = ' ';
    }
  }
}


/*
 * HB_DET.C - RomWBW HBIOS Detection
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"

int hbios_detect() {
  unsigned char **id_ptr;
  unsigned char *id;
  id_ptr = (unsigned char **)0xFFFC;
  id = *id_ptr;
  if (id && id[0] == 'W' && id[1] == (unsigned char)(~'W' & 0xFF))
    return 1;
  return 0;
}

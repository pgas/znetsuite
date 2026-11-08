/*
 * HB_LIST.C - RomWBW List Available Character Devices
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"

hbios_list_devices(cnt) unsigned char cnt;
{
  unsigned char u, type, mode, base;

  cio_printf("\nAvailable Character Devices (%d):\n", (int)cnt);
  cio_printf(" Unit | Type                 | Base Port\n");
  cio_printf("------+----------------------+----------\n");
  for (u = 0; u < cnt; u++) {
    if (hbios_device(u, &type, &mode, &base) == 0) {
      cio_printf("  %02X  | %-20s |    %02X\n", (unsigned int)u,
                 hbios_devname(type), (unsigned int)base);
    }
  }
  cio_printf("\n");
}


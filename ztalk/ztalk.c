/*
 * ZTALK - RomWBW Character Device Terminal Interface
 *
 * Part of ZNetSuite.
 * Based on original HTALK for RomWBW by Tom Plano (tomplano@proton.me).
 *
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"
#include "fmt.h"

#define CTRLC 0x03

/* Command tail at CP/M default buffer 0x0080 */
static unsigned char *tail = (unsigned char *)0x0080;

/* Terminal bridge loop */
talk_bridge(target)
unsigned char target;
{
  int ch;
  while (1) {
    /* 1. Check target device for incoming byte */
    if (cio_ist(target)) {
      ch = cio_in(target);
      cio_out(CIO_CONSOLE, (unsigned char)ch);
    }

    /* 2. Check console for user input */
    if (cio_ist(CIO_CONSOLE)) {
      ch = cio_in(CIO_CONSOLE);
      if (ch == CTRLC)
        break;
      cio_out(target, (unsigned char)ch);
    }
  }
}

main() {
  unsigned char cnt, target, dev_type, dev_mode, dev_base;
  char token[16];

  cio_printf("\nZTALK v1.0 (ZNetSuite)\n");

  if (!hbios_detect()) {
    cio_printf("Error: RomWBW HBIOS not detected.\n");
    return 1;
  }

  cnt = (unsigned char)hbios_ciocnt();

  /* Parse CP/M command tail */
  if (fmt_cpm_arg(tail, 0, token, sizeof(token)) <= 0 ||
      token[0] == '?' || (token[0] == '/' && token[1] == '?')) {
    cio_printf("Usage: ZTALK <device_hex_id>\n");
    hbios_list_devices(cnt);
    return 0;
  }

  target = (unsigned char)fmt_parse_hex(token);
  if (target >= cnt) {
    cio_printf("Error: Device %02X out of range (max %02X)\n", (unsigned int)target, (int)(cnt - 1));
    hbios_list_devices(cnt);
    return 1;
  }

  hbios_device(target, &dev_type, &dev_mode, &dev_base);
  cio_printf("Connecting to device %02X (%s)... [Ctrl+C to exit]\n\n",
             (unsigned int)target, hbios_devname(dev_type));

  talk_bridge(target);

  cio_printf("\n\nZTALK disconnected.\n");
  return 0;
}

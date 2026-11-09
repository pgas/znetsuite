/*
 * HB_NAME.C - RomWBW Device Type Names
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"

static char *dev_names[] = {
  "UART (16C550)",
  "ASCI (Z180)",
  "TERM (ANSI)",
  "PRPCON (PropIO)",
  "PPPCON (ParPortProp)",
  "SIO (Zilog)",
  "ACIA (68B50)",
  "PIO (Parallel)",
  "UF (USB FIFO)",
  "DUART (2681)",
  "Z2U (Z280)",
  "LPT (Parallel)",
  "ESPCON (ESP)",
  "ESPSER (ESP)",
  "SCON (S100)",
  "SSER (SimH)",
  "EZ80UART (eZ80)",
  "DLPSER",
  "TSER",
  "SCC (Z85C30)"
};

char *hbios_devname(type)
unsigned char type;
{
  if (type < 20)
    return dev_names[type];
  return "UNKNOWN";
}


/*
 * HB_NAME.C - RomWBW Device Type Names
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"

char *hbios_devname(type)
unsigned char type;
{
  switch (type) {
  case 0x00: return "UART (16C550)";
  case 0x01: return "ASCI (Z180)";
  case 0x02: return "TERM (ANSI)";
  case 0x03: return "PRPCON (PropIO)";
  case 0x04: return "PPPCON (ParPortProp)";
  case 0x05: return "SIO (Zilog)";
  case 0x06: return "ACIA (68B50)";
  case 0x07: return "PIO (Parallel)";
  case 0x08: return "UF (USB FIFO)";
  case 0x09: return "DUART (2681)";
  case 0x0A: return "Z2U (Z280)";
  case 0x0B: return "LPT (Parallel)";
  case 0x0C: return "ESPCON (ESP)";
  case 0x0D: return "ESPSER (ESP)";
  case 0x0E: return "SCON (S100)";
  case 0x0F: return "SSER (SimH)";
  case 0x10: return "EZ80UART (eZ80)";
  case 0x11: return "DLPSER";
  case 0x12: return "TSER";
  case 0x13: return "SCC (Z85C30)";
  default:   return "UNKNOWN";
  }
}


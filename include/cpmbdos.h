/*
 * CPMBDOS.H - Lightweight CP/M BDOS & FCB Interface (Zero STDIO dependency)
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#ifndef _CPMBDOS_H
#define _CPMBDOS_H

/* CP/M 2.2 BDOS function codes */
#define BDOS_OPEN  15
#define BDOS_CLOSE 16
#define BDOS_DEL   19
#define BDOS_READ  20
#define BDOS_WRITE 21
#define BDOS_MAKE  22
#define BDOS_DMA   26
#define BDOS_RRAN  33
#define BDOS_WRAN  34

/* Standard 36-byte CP/M File Control Block */
struct cpm_fcb {
  unsigned char dr;
  char name[8];
  char ft[3];
  unsigned char ex;
  unsigned char s1;
  unsigned char s2;
  unsigned char rc;
  unsigned char dm[16];
  unsigned char cr;
  unsigned char r0;
  unsigned char r1;
  unsigned char r2;
};

/* Direct CP/M BDOS call (C = func, DE = parm, returns A in L) */
extern int cpm_bdos(/* int func, void *parm */);

/* Initialize a 36-byte CP/M FCB from "[d:]filename.ext" */
extern cpm_fcb_init(/* struct cpm_fcb *fcb, char *fname */);

#endif /* _CPMBDOS_H */


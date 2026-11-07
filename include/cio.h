/*
 * CIO.H - RomWBW Character I/O & Serial Line Interface
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#ifndef _CIO_H
#define _CIO_H

#define CIO_CONSOLE         0x80

#define BF_CIOIN            0x00
#define BF_CIOOUT           0x01
#define BF_CIOIST           0x02
#define BF_CIOOST           0x03
#define BF_CIODEVICE        0x06
#define BF_SYSGET           0xF8
#define BF_SYSGET_CIOCNT    0x00

/* System & device detection */
extern int hbios_detect();
extern int hbios_ciocnt();
extern int hbios_device(/* unsigned char u, unsigned char *dtype, unsigned char *dmode, unsigned char *dbase */);
extern char *hbios_devname(/* unsigned char dtype */);
extern hbios_list_devices(/* unsigned char cnt */);

/* Direct HBIOS character I/O primitives */
extern int cio_ist(/* unsigned char u */);
extern int cio_ost(/* unsigned char u */);
extern int cio_in(/* unsigned char u */);
extern cio_out(/* unsigned char u, unsigned char ch */);
extern cio_puts(/* unsigned char u, char *s */);
extern cio_write(/* unsigned char u, char *buf, int len */);
extern cio_flush(/* unsigned char u */);

/* Direct console output & lightweight formatted print (zero stdio dependency) */
extern void cio_cputc(/* char ch */);
extern void cio_cputs(/* char *s */);
extern void cio_cputhex(/* unsigned int val, int width */);
extern void cio_cputdec(/* int val */);
extern void cio_printf(/* char *fmt, ... */);

/* High-level timed and AT-modem helpers */
extern int cio_get_timeout(/* unsigned char u, long max_loops, unsigned char abort_u, int abort_ch */);
extern int cio_readline(/* unsigned char u, char *buf, int maxlen, long max_loops, unsigned char abort_u, int abort_ch */);
extern cio_send_cmd(/* unsigned char u, char *cmd */);
extern int cio_wait_ok(/* unsigned char u, long max_loops */);

#endif /* _CIO_H */


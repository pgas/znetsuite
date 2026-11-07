/*
 * FMT.H - Number Parsing & Text Formatting Library
 *
 * Part of ZNetSuite.
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#ifndef _FMT_H
#define _FMT_H

/* Parsing */
extern int fmt_parse_num(/* char *s */);
extern int fmt_parse_hex(/* char *s */);
extern int fmt_parse_int(/* char *s */);

/* Integer & Hex Presentation (returns pointer to buf) */
extern char *fmt_int(/* char *buf, int val */);
extern char *fmt_uint(/* char *buf, unsigned int val */);
extern char *fmt_hex8(/* char *buf, unsigned char val, int prefix */);
extern char *fmt_hex16(/* char *buf, unsigned int val, int prefix */);
extern char *fmt_size(/* char *buf, long bytes */);

/* String Manipulation */
extern char *fmt_trim(/* char *s */);
extern char *fmt_lwr(/* char *s */);
extern char *fmt_upr(/* char *s */);

/* CP/M Command Tail Parser (0-based argument index from buffer at 0x0080) */
extern int fmt_cpm_arg(/* unsigned char *tail, int arg_idx, char *buf, int maxlen */);

#endif /* _FMT_H */


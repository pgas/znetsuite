/*
 * ZTELNET.H - Telnet Protocol Definitions & State for ZNetSuite
 *
 * Part of ZNetSuite (ZTELNET).
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#ifndef _ZTELNET_H
#define _ZTELNET_H

/* RFC 854 Telnet Command Codes */
#define TN_SE          240
#define TN_NOP         241
#define TN_BRK         243
#define TN_IP          244
#define TN_AO          245
#define TN_AYT         246
#define TN_EC          247
#define TN_EL          248
#define TN_GA          249
#define TN_SB          250
#define TN_WILL        251
#define TN_WONT        252
#define TN_DO          253
#define TN_DONT        254
#define TN_IAC         255

/* Telnet Option Codes */
#define OPT_BINARY     0
#define OPT_ECHO       1
#define OPT_SGA        3
#define OPT_TTYPE      24
#define OPT_NAWS       31

/* Default local escape key: Ctrl+] (0x1D) */
#define DEFAULT_ESC    0x1D

/* Configuration & session state */
extern unsigned char target_unit;
extern unsigned int cfg_port;
extern unsigned char cfg_escape;
extern int local_echo;
extern int crlf_mode;
extern char cfg_term[16];
extern char cfg_host[64];

/* Telnet IAC processor functions (tn_iac.c) */
extern tn_init();
extern tn_send_naws(/* unsigned char u */);
extern tn_tx_byte(/* unsigned char u, unsigned char b */);
extern int tn_rx_byte(/* unsigned char u, unsigned char b */);

#endif /* _ZTELNET_H */

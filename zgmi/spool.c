/*
 * SPOOL.C - Lazy CP/M Disk Spooler, Sliding RAM Window & Config Loader
 *
 * Implements:
 * - Direct CP/M BDOS file I/O (zero stdio overhead)
 * - ZGMI.CFG key=value configuration parser
 * - Sliding RAM window with lazy creation of ZGMI.$$$ when a document
 *   exceeds RAM, plus 16-line checkpoint indexing for fast forward and
 *   backward random-access scrollback.
 *
 * Part of ZNetSuite (ZGMI).
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "spool.h"
#include "cpmbdos.h"
#include "fmt.h"
#include "term.h"
#include <string.h>

/* Linker-provided end of BSS segment (start of free CP/M TPA) */
extern char _Hbss[];

/* Configuration parameters */
unsigned char target_unit = 1;
int cfg_max_spool_kb = 128;
int cfg_ram_kb = 0; /* 0 = auto-size to available CP/M TPA */
unsigned int cfg_ram_limit = 16384;
int cfg_hw_scroll = 1;
char cfg_spool_file[20] = "ZGMI.$$$";
char cfg_home_url[128] = "";

/* Sliding RAM window buffer */
char *text_pool;
unsigned int text_pool_len;
int win_start;
int win_count;
char *win_lines[WIN_LINES];

/* Document-wide line, checkpoint, and link tracking */
unsigned int ckpt_rec[MAX_CKPTS];
unsigned char ckpt_off[MAX_CKPTS];
int total_lines;
int doc_truncated;

struct link_entry links[MAX_LINKS];
int total_links;
int cur_link;

/* Spool file state */
static struct cpm_fcb spool_fcb;
static struct cpm_fcb cfg_fcb;
static unsigned char spool_dma[128];
static char io_line_buf[160];
static int spool_pos;
static unsigned int spool_recs;
static unsigned int max_spool_recs;
int spool_active;
int spool_full;

/* Parse a single line from ZGMI.CFG */
static parse_config_line(line) char *line;
{
  char *eq;
  char *key;
  char *val;
  char *c;

  for (c = line; *c; c++) {
    if (*c == '#' || *c == ';') {
      *c = '\0';
      break;
    }
  }

  line = fmt_trim(line);
  if (!line[0])
    return;

  eq = strchr(line, '=');
  if (!eq)
    return;
  *eq = '\0';
  key = fmt_lwr(fmt_trim(line));
  val = fmt_trim(eq + 1);

  if (val[0] == '"') {
    int len = strlen(val);
    if (len >= 2 && val[len - 1] == '"') {
      val[len - 1] = '\0';
      val++;
    }
  }

  if (strcmp(key, "unit") == 0) {
    target_unit = (unsigned char)fmt_parse_num(val);
  } else if (strcmp(key, "max_spool") == 0 ||
             strcmp(key, "spool_size") == 0) {
    cfg_max_spool_kb = fmt_parse_int(val);
    if (cfg_max_spool_kb < 0)
      cfg_max_spool_kb = 0;
    if (cfg_max_spool_kb > 4096)
      cfg_max_spool_kb = 4096;
  } else if (strcmp(key, "spool_file") == 0 || strcmp(key, "spool") == 0) {
    if (val[0]) {
      strncpy(cfg_spool_file, val, sizeof(cfg_spool_file) - 1);
      cfg_spool_file[sizeof(cfg_spool_file) - 1] = '\0';
    }
  } else if (strcmp(key, "ram_window") == 0 || strcmp(key, "ram") == 0) {
    cfg_ram_kb = fmt_parse_int(val);
    if (cfg_ram_kb < 0)
      cfg_ram_kb = 0;
    if (cfg_ram_kb > 48)
      cfg_ram_kb = 48;
  } else if (strcmp(key, "hw_scroll") == 0 || strcmp(key, "scroll") == 0) {
    cfg_hw_scroll = fmt_parse_int(val) ? 1 : 0;
  } else if (strcmp(key, "home") == 0 || strcmp(key, "url") == 0) {
    strncpy(cfg_home_url, val, sizeof(cfg_home_url) - 1);
    cfg_home_url[sizeof(cfg_home_url) - 1] = '\0';
  }
}

/* Compute usable TPA RAM between _Hbss and CP/M BDOS base (0x0006) */
spool_init_ram() {
  unsigned int bdos_base = *(unsigned int *)0x0006;
  unsigned int free_base = (unsigned int)_Hbss;
  unsigned int max_avail = 2048;

  text_pool = _Hbss;

  /* Reserve 1536 bytes below BDOS for Z80 stack */
  if (bdos_base > free_base + 1536) {
    max_avail = bdos_base - free_base - 1536;
  }
  if (max_avail > 32768) {
    max_avail = 32768;
  }

  if (cfg_ram_kb > 0) {
    unsigned int req = (unsigned int)cfg_ram_kb << 10;
    cfg_ram_limit = (req < max_avail) ? req : max_avail;
  } else {
    cfg_ram_limit = max_avail;
  }
}

/* Load configuration file using CP/M BDOS sequential read */
int load_config_file(fname) char *fname;
{
  int pos = 0;
  int i;
  unsigned char ch;

  cpm_fcb_init(&cfg_fcb, fname);
  if (cpm_bdos(BDOS_OPEN, &cfg_fcb) == 0xFF)
    return 0;

  cpm_bdos(BDOS_DMA, spool_dma);
  while (cpm_bdos(BDOS_READ, &cfg_fcb) == 0) {
    for (i = 0; i < 128; i++) {
      ch = spool_dma[i];
      if (ch == 0x1A) {
        io_line_buf[pos] = '\0';
        parse_config_line(io_line_buf);
        cpm_bdos(BDOS_CLOSE, &cfg_fcb);
        return 1;
      }
      if (ch == '\r')
        continue;
      if (ch == '\n') {
        io_line_buf[pos] = '\0';
        parse_config_line(io_line_buf);
        pos = 0;
      } else if (pos < 126) {
        io_line_buf[pos++] = (char)ch;
      }
    }
    cpm_bdos(BDOS_DMA, spool_dma);
  }

  if (pos > 0) {
    io_line_buf[pos] = '\0';
    parse_config_line(io_line_buf);
  }
  cpm_bdos(BDOS_CLOSE, &cfg_fcb);
  return 1;
}

/* Close and delete temporary spool file if active */
spool_cleanup() {
  if (spool_active) {
    cpm_bdos(BDOS_CLOSE, &spool_fcb);
    cpm_fcb_init(&spool_fcb, cfg_spool_file);
    cpm_bdos(BDOS_DEL, &spool_fcb);
    spool_active = 0;
  }
  spool_pos = 0;
  spool_recs = 0;
  spool_full = 0;
}

/* Lazily create the spool file on disk when document exceeds RAM */
static int spool_create() {
  if (spool_active)
    return 1;
  if (cfg_max_spool_kb <= 0) {
    spool_full = 1;
    return 0;
  }

  max_spool_recs = (unsigned int)cfg_max_spool_kb << 3;
  cpm_fcb_init(&spool_fcb, cfg_spool_file);
  cpm_bdos(BDOS_DEL, &spool_fcb);
  cpm_fcb_init(&spool_fcb, cfg_spool_file);
  if (cpm_bdos(BDOS_MAKE, &spool_fcb) == 0xFF) {
    spool_full = 1;
    return 0;
  }
  spool_pos = 0;
  spool_recs = 0;
  spool_active = 1;
  return 1;
}

/* Write a single byte to sequential spool stream */
static int spool_putc(c) char c;
{
  if (spool_full)
    return 0;
  spool_dma[spool_pos++] = (unsigned char)c;
  if (spool_pos >= 128) {
    if (spool_recs >= max_spool_recs) {
      spool_full = 1;
      spool_pos = 0;
      return 0;
    }
    cpm_bdos(BDOS_DMA, spool_dma);
    if (cpm_bdos(BDOS_WRITE, &spool_fcb) != 0) {
      spool_full = 1;
      spool_pos = 0;
      return 0;
    }
    spool_recs++;
    spool_pos = 0;
  }
  return 1;
}

/* Write one null-terminated line to the spool file and record checkpoints */
static int spool_write_raw(line_idx, s) int line_idx;
char *s;
{
  int len;
  unsigned int needed_recs;

  if (spool_full)
    return 0;

  len = strlen(s) + 1;
  needed_recs = (unsigned int)((spool_pos + len) >> 7);
  if (spool_recs + needed_recs >= max_spool_recs) {
    spool_full = 1;
    return 0;
  }

  if ((line_idx & 15) == 0) {
    int ck = line_idx >> 4;
    if (ck < MAX_CKPTS) {
      ckpt_rec[ck] = spool_recs;
      ckpt_off[ck] = (unsigned char)spool_pos;
    }
  }

  while (*s) {
    if (!spool_putc(*s++))
      return 0;
  }
  if (!spool_putc('\0'))
    return 0;

  return 1;
}

/* Flush trailing bytes to spool file and re-open for random record reads */
spool_finish_write() {
  if (!spool_active)
    return;

  if (spool_pos > 0 && !spool_full) {
    while (spool_pos < 128) {
      spool_dma[spool_pos++] = 0;
    }
    cpm_bdos(BDOS_DMA, spool_dma);
    if (cpm_bdos(BDOS_WRITE, &spool_fcb) == 0) {
      spool_recs++;
    }
    spool_pos = 0;
  }

  cpm_bdos(BDOS_CLOSE, &spool_fcb);
  cpm_fcb_init(&spool_fcb, cfg_spool_file);
  cpm_bdos(BDOS_OPEN, &spool_fcb);
}

/* Read a 128-byte random record from the spool file into spool_dma */
static int spool_read_rec(rec) unsigned int rec;
{
  spool_fcb.r0 = (unsigned char)(rec & 0xFF);
  spool_fcb.r1 = (unsigned char)((rec >> 8) & 0xFF);
  spool_fcb.r2 = 0;
  cpm_bdos(BDOS_DMA, spool_dma);
  return (cpm_bdos(BDOS_RRAN, &spool_fcb) == 0);
}

/* Load a window of lines starting at 16-line checkpoint `new_start` */
static int spool_load_from(new_start) int new_start;
{
  int ck;
  unsigned int rec;
  int off;
  int pos;
  int len;
  unsigned char ch;

  if (!spool_active)
    return 0;

  new_start &= ~15;
  if (new_start < 0)
    new_start = 0;
  ck = new_start >> 4;
  if (ck >= MAX_CKPTS)
    return 0;

  rec = ckpt_rec[ck];
  off = (int)ckpt_off[ck];
  if (!spool_read_rec(rec))
    return 0;

  win_start = new_start;
  win_count = 0;
  text_pool_len = 0;

  while (win_count < WIN_LINES && (win_start + win_count) < total_lines) {
    pos = 0;
    for (;;) {
      if (off >= 128) {
        rec++;
        if (!spool_read_rec(rec))
          break;
        off = 0;
      }
      ch = spool_dma[off++];
      if (ch == '\0')
        break;
      if (pos < 158)
        io_line_buf[pos++] = (char)ch;
    }
    io_line_buf[pos] = '\0';

    len = pos + 1;
    if (text_pool_len + (unsigned int)len >= cfg_ram_limit)
      break;

    strcpy(&text_pool[text_pool_len], io_line_buf);
    win_lines[win_count++] = &text_pool[text_pool_len];
    text_pool_len += (unsigned int)len;
  }

  return 1;
}

/* Ensure visible screen lines [doc_top .. doc_top + screen_lines - 1] are in RAM */
int ensure_window_loaded(doc_top, screen_lines) int doc_top;
int screen_lines;
{
  int last_needed;
  int target_start;

  if (!spool_active || total_lines <= 0)
    return 0;

  last_needed = doc_top + screen_lines - 1;
  if (last_needed >= total_lines)
    last_needed = total_lines - 1;

  if (doc_top >= win_start && last_needed < win_start + win_count)
    return 0;

  /* Center window with look-behind margin so scrolling up/down stays cached */
  target_start = (doc_top - 48) & ~15;
  if (target_start < 0)
    target_start = 0;

  spool_load_from(target_start);

  /* If look-behind filled RAM before covering bottom of screen, align closer */
  if (last_needed >= win_start + win_count && target_start < (doc_top & ~15)) {
    spool_load_from(doc_top & ~15);
  }
  return 1;
}

/* Parse link index: looks for "[ n]" or "[n]" or "=>" */
static int parse_link_num(s) char *s;
{
  int n = 0;
  while (*s == ' ')
    s++;
  if (*s == '[') {
    s++;
    while (*s == ' ')
      s++;
    if (*s < '0' || *s > '9')
      return 0;
    while (*s >= '0' && *s <= '9') {
      n = n * 10 + (*s - '0');
      s++;
    }
    if (*s == ']')
      return n;
  } else if (s[0] == '=' && s[1] == '>') {
    return total_links + 1;
  }
  return 0;
}

/* Clear document buffer and remove any previous spool file */
clear_doc() {
  spool_cleanup();
  text_pool_len = 0;
  win_start = 0;
  win_count = 0;
  total_lines = 0;
  doc_truncated = 0;
  total_links = 0;
  cur_link = -1;
}

/* Add a line of text to RAM window and/or disk spool */
int add_doc_line(line) char *line;
{
  int len;
  int lid;
  int i;

  if (total_lines >= MAX_DOC_LINES) {
    doc_truncated = 1;
    return 0;
  }

  len = strlen(line);

  if (!spool_active) {
    /* Keep in RAM window if space allows */
    if (win_count < WIN_LINES &&
        (text_pool_len + (unsigned int)(len + 1)) < cfg_ram_limit) {
      strcpy(&text_pool[text_pool_len], line);
      win_lines[win_count] = &text_pool[text_pool_len];
      text_pool_len += (unsigned int)(len + 1);
      win_count++;

      lid = parse_link_num(line);
      if (lid > 0 && total_links < MAX_LINKS) {
        links[total_links].line = total_lines;
        links[total_links].id = lid;
        total_links++;
      }
      total_lines++;
      return 1;
    }

    /* RAM window full: lazily create spool file and flush existing RAM lines */
    if (cfg_max_spool_kb <= 0 || spool_full) {
      doc_truncated = 1;
      return 0;
    }

    if (!spool_create()) {
      doc_truncated = 1;
      return 0;
    }

    for (i = 0; i < win_count; i++) {
      if (!spool_write_raw(i, win_lines[i])) {
        doc_truncated = 1;
        return 0;
      }
    }
  }

  /* Spooling is active: append new line to disk spool */
  if (!spool_write_raw(total_lines, line)) {
    doc_truncated = 1;
    return 0;
  }

  lid = parse_link_num(line);
  if (lid > 0 && total_links < MAX_LINKS) {
    links[total_links].line = total_lines;
    links[total_links].id = lid;
    total_links++;
  }

  total_lines++;
  return 1;
}

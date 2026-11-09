/*
 * SPOOL.H - Lazy CP/M Disk Spooler, Sliding RAM Window & Config Loader
 *
 * Part of ZNetSuite (ZGMI).
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#ifndef _SPOOL_H
#define _SPOOL_H

#define WIN_LINES 400
#define MAX_DOC_LINES 4096
#define MAX_CKPTS (MAX_DOC_LINES / 16)
#define MAX_LINKS 160

/* Configuration settings (loaded from ZGMI.CFG and CLI flags) */
extern unsigned char target_unit;
extern int cfg_max_spool_kb;
extern int cfg_ram_kb;
extern unsigned int cfg_ram_limit;
extern int cfg_hw_scroll;
extern char cfg_spool_file[20];
extern char cfg_home_url[128];

/* Sliding RAM window buffer (dynamically sized in TPA from _Hbss to BDOS) */
extern char *text_pool;
extern unsigned int text_pool_len;
extern int win_start;
extern int win_count;
extern char *win_lines[WIN_LINES];

/* Document-wide line and link tracking */
extern int total_lines;
extern int doc_truncated;
extern int spool_active;
extern int spool_full;

struct link_entry {
  int line;
  int id;
};
extern struct link_entry links[MAX_LINKS];
extern int total_links;
extern int cur_link;

/* Memory & Config initialization */
extern spool_init_ram();
extern int load_config_file(/* char *fname */);

/* Document & Spooler lifecycle */
extern clear_doc();
extern spool_cleanup();
extern int add_doc_line(/* char *line */);
extern spool_finish_write();
extern int ensure_window_loaded(/* int doc_top, int screen_lines */);

/* UI & Screen Rendering (ui.c) */
extern int doc_top;
extern char current_url[128];
extern int header_dirty;
extern int prev_doc_top;
extern int prev_cur_link;

extern show_status(/* char *msg */);
extern ensure_link_visible();
extern select_next_link();
extern select_prev_link();
extern normalize_url(/* char *src, char *dst */);
extern render_header();
extern render_doc_row(/* int srow, int line_idx */);
extern render_status();
extern update_screen(/* int force_full */);
extern int prompt_edit_url(/* char *out_url */);

#endif /* _SPOOL_H */

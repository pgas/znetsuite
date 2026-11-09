/*
 * UI.C - Differential VT100/ANSI Screen Renderer & Inline URL Editor
 *
 * Part of ZNetSuite (ZGMI).
 * License: GNU Affero General Public License v3 (AGPL-3.0-or-later)
 */

#include "cio.h"
#include "fmt.h"
#include "spool.h"
#include "term.h"
#include <string.h>

/* View & differential UI state */
int doc_top;
char current_url[128];
int header_dirty = 1;
int prev_doc_top = -1;
int prev_cur_link = -1;

static char edit_buf[128];

/* Display loading or prompt status on bottom row (clamped to term_cols - 1) */
show_status(msg) char *msg;
{
  int col = 1;
  int max_col = term_cols - 1;

  term_goto(term_rows, 1);
  term_attr(TERM_ATTR_REVERSE);
  term_putc(' ');
  while (*msg && col < max_col - 1) {
    term_putc((unsigned char)*msg++);
    col++;
  }
  term_putc(' ');
  term_reset();
  term_clear_eol();
}

/* Ensure selected link is visible within screen lines */
ensure_link_visible() {
  int screen_lines = term_rows - 2;
  if (cur_link >= 0 && cur_link < total_links) {
    int l_line = links[cur_link].line;
    if (l_line < doc_top) {
      doc_top = l_line;
    } else if (l_line >= doc_top + screen_lines) {
      doc_top = l_line - screen_lines + 1;
    }
  }
}

/* Select next link, snapping to current viewport if cur_link was off-screen */
select_next_link() {
  int screen_lines = term_rows - 2;
  int i;

  if (total_links <= 0)
    return;

  if (cur_link < 0 || links[cur_link].line < doc_top ||
      links[cur_link].line >= doc_top + screen_lines) {
    for (i = 0; i < total_links; i++) {
      if (links[i].line >= doc_top) {
        cur_link = i;
        ensure_link_visible();
        return;
      }
    }
  }
  cur_link = (cur_link + 1) % total_links;
  ensure_link_visible();
}

/* Select prev link, snapping to current viewport if cur_link was off-screen */
select_prev_link() {
  int screen_lines = term_rows - 2;
  int i;

  if (total_links <= 0)
    return;

  if (cur_link < 0 || links[cur_link].line < doc_top ||
      links[cur_link].line >= doc_top + screen_lines) {
    for (i = total_links - 1; i >= 0; i--) {
      if (links[i].line < doc_top + screen_lines) {
        cur_link = i;
        ensure_link_visible();
        return;
      }
    }
  }
  cur_link = (cur_link - 1 + total_links) % total_links;
  ensure_link_visible();
}

/* Normalize URL into dst: prepend gemini:// and lowercase scheme/host */
normalize_url(src, dst) char *src;
char *dst;
{
  char *p;

  src = fmt_trim(src);
  if (strncasecmp(src, "GEMINI://", 9) != 0) {
    strcpy(dst, "gemini://");
    strncat(dst, src, 117);
    dst[127] = '\0';
  } else {
    strcpy(dst, "gemini://");
    strncat(dst, src + 9, 117);
    dst[127] = '\0';
  }

  p = dst + 9;
  while (*p && *p != '/') {
    if (*p >= 'A' && *p <= 'Z') {
      *p = *p + ('a' - 'A');
    }
    p++;
  }
}

/* Render top header bar only when URL changes or on full redraw */
render_header() {
  int col = 8;
  int max_col = term_cols - 1;
  char *p = current_url;

  term_home();
  term_attr(TERM_ATTR_REVERSE);
  term_puts(" ZGMI | ");
  while (*p && col < max_col) {
    term_putc((unsigned char)*p++);
    col++;
  }
  term_putc(' ');
  term_reset();
  term_clear_eol();
  header_dirty = 0;
}

/* Render a single document line at 1-based terminal row `srow` */
render_doc_row(srow, line_idx) int srow;
int line_idx;
{
  int line_width = term_cols - 1;
  int is_sel;
  int win_idx;
  int col;
  char *s;

  term_goto(srow, 1);

  if (line_idx >= 0 && line_idx < total_lines) {
    is_sel = (cur_link >= 0 && cur_link < total_links &&
              links[cur_link].line == line_idx);
    win_idx = line_idx - win_start;
    s = "";
    col = 0;

    if (win_idx >= 0 && win_idx < win_count) {
      s = win_lines[win_idx];
    }

    if (is_sel) {
      term_attr(TERM_ATTR_REVERSE);
    }

    while (*s && col < line_width) {
      term_putc((unsigned char)*s++);
      col++;
    }

    if (is_sel) {
      term_putc(' ');
      term_reset();
    }
    term_clear_eol();
  } else {
    term_putc('~');
    term_clear_eol();
  }
}

/* Render bottom status bar (strictly bounded to term_cols - 1) */
render_status() {
  char status_line[96];
  char num_buf[16];
  int col = 0;
  int max_col = term_cols - 1;
  char *p;

  status_line[0] = '\0';
  if (total_links > 0 && cur_link >= 0) {
    strcat(status_line, " [");
    fmt_int(num_buf, cur_link + 1);
    strcat(status_line, num_buf);
    strcat(status_line, "/");
    fmt_int(num_buf, total_links);
    strcat(status_line, num_buf);
    strcat(status_line, "] ");
  } else {
    strcat(status_line, " ");
  }

  strcat(status_line, "L:");
  fmt_int(num_buf, doc_top + 1);
  strcat(status_line, num_buf);
  strcat(status_line, "/");
  fmt_int(num_buf, total_lines);
  strcat(status_line, num_buf);

  if (doc_truncated) {
    strcat(status_line, " [TRUNC]");
  } else if (spool_active) {
    strcat(status_line, " [SPL]");
  }

  strcat(status_line, " | j/k:scrl Spc/d/u:pg Tab:lnk e:url b:bk q:quit ");

  term_goto(term_rows, 1);
  term_attr(TERM_ATTR_REVERSE);
  p = status_line;
  while (*p && col < max_col) {
    term_putc((unsigned char)*p++);
    col++;
  }
  term_reset();
  term_clear_eol();
}

/* Differential screen update: minimizes serial traffic to prevent flicker */
update_screen(force_full) int force_full;
{
  int r;
  int screen_lines = term_rows - 2;
  int old_top = prev_doc_top;
  int old_link = prev_cur_link;
  int oline;
  int nline;

  ensure_window_loaded(doc_top, screen_lines);

  /* 1. Full redraw or multi-line jump */
  if (force_full || old_top < 0 ||
      (doc_top != old_top &&
       (!cfg_hw_scroll ||
        (doc_top != old_top + 1 && doc_top != old_top - 1)))) {
    if (force_full || header_dirty) {
      render_header();
    }
    for (r = 0; r < screen_lines; r++) {
      render_doc_row(r + 2, doc_top + r);
    }
    render_status();
    prev_doc_top = doc_top;
    prev_cur_link = cur_link;
    return;
  }

  /* 2. Same scroll position (e.g. Tab / Shift-Tab link selection change) */
  if (doc_top == old_top) {
    if (old_link != cur_link) {
      if (old_link >= 0 && old_link < total_links) {
        oline = links[old_link].line;
        if (oline >= doc_top && oline < doc_top + screen_lines) {
          render_doc_row(2 + (oline - doc_top), oline);
        }
      }
      if (cur_link >= 0 && cur_link < total_links) {
        nline = links[cur_link].line;
        if (nline >= doc_top && nline < doc_top + screen_lines) {
          render_doc_row(2 + (nline - doc_top), nline);
        }
      }
    }
    render_status();
    prev_cur_link = cur_link;
    return;
  }

  /* 3. Hardware 1-line scroll down (doc_top == old_top + 1) */
  if (doc_top == old_top + 1) {
    if (old_link != cur_link && old_link >= 0 && old_link < total_links) {
      oline = links[old_link].line;
      if (oline >= doc_top && oline < old_top + screen_lines) {
        render_doc_row(2 + (oline - old_top), oline);
      }
    }
    term_scroll_up(2, term_rows - 1);
    render_doc_row(term_rows - 1, doc_top + screen_lines - 1);
    if (old_link != cur_link && cur_link >= 0 && cur_link < total_links) {
      nline = links[cur_link].line;
      if (nline >= doc_top && nline < doc_top + screen_lines - 1) {
        render_doc_row(2 + (nline - doc_top), nline);
      }
    }
    render_status();
    prev_doc_top = doc_top;
    prev_cur_link = cur_link;
    return;
  }

  /* 4. Hardware 1-line scroll up (doc_top == old_top - 1) */
  if (doc_top == old_top - 1) {
    if (old_link != cur_link && old_link >= 0 && old_link < total_links) {
      oline = links[old_link].line;
      if (oline >= old_top && oline < doc_top + screen_lines) {
        render_doc_row(2 + (oline - old_top), oline);
      }
    }
    term_scroll_down(2, term_rows - 1);
    render_doc_row(2, doc_top);
    if (old_link != cur_link && cur_link >= 0 && cur_link < total_links) {
      nline = links[cur_link].line;
      if (nline > doc_top && nline < doc_top + screen_lines) {
        render_doc_row(2 + (nline - doc_top), nline);
      }
    }
    render_status();
    prev_doc_top = doc_top;
    prev_cur_link = cur_link;
    return;
  }
}

/* Draw the URL editor bar on the line above the helpbar (term_rows - 1) */
static draw_url_edit(buf, pos, max_vis) char *buf;
int pos;
int max_vis;
{
  int edit_row = term_rows - 1;
  int start = 0;
  int i = 0;

  if (pos >= max_vis) {
    start = pos - max_vis + 1;
  }

  term_goto(edit_row, 1);
  term_attr(TERM_ATTR_REVERSE);
  term_puts(" URL: ");
  term_reset();
  term_putc(' ');

  while (buf[start + i] && i < max_vis) {
    term_putc((unsigned char)buf[start + i]);
    i++;
  }
  term_clear_eol();
  term_goto(edit_row, 8 + (pos - start));
}

/* Interactive inline URL editor on line above helpbar (triggered by 'e').
 * Returns 1 and writes normalized URL to out_url if user pressed Enter,
 * or returns 0 if user cancelled with Esc / Ctrl-C.
 */
int prompt_edit_url(out_url) char *out_url;
{
  int edit_row = term_rows - 1;
  int len;
  int pos;
  int max_vis;
  int key;
  int i;
  int old_pos;

  strcpy(edit_buf, current_url);
  len = strlen(edit_buf);
  pos = len;
  max_vis = term_cols - 9;
  if (max_vis < 20)
    max_vis = 20;

  show_status("Enter:open | Esc:cancel | BS/Del:erase | ^U:clear | ^W:del-part");
  term_cursor_show();
  draw_url_edit(edit_buf, pos, max_vis);

  for (;;) {
    key = term_getkey();
    if (key == 0)
      continue;

    if (key == KEY_ESC || key == KEY_CTRL_C) {
      term_cursor_hide();
      render_doc_row(edit_row, doc_top + (edit_row - 2));
      render_status();
      return 0;
    }

    if (key == KEY_CR || key == KEY_LF) {
      term_cursor_hide();
      fmt_trim(edit_buf);
      if (!edit_buf[0]) {
        render_doc_row(edit_row, doc_top + (edit_row - 2));
        render_status();
        return 0;
      }
      normalize_url(edit_buf, out_url);
      return 1;
    }

    if (key == 0x08 || key == KEY_BS) {
      if (pos > 0) {
        for (i = pos - 1; i < len; i++) {
          edit_buf[i] = edit_buf[i + 1];
        }
        pos--;
        len--;
        draw_url_edit(edit_buf, pos, max_vis);
      }
    } else if (key == KEY_DEL || key == KEY_CTRL_D) {
      if (pos < len) {
        for (i = pos; i < len; i++) {
          edit_buf[i] = edit_buf[i + 1];
        }
        len--;
        draw_url_edit(edit_buf, pos, max_vis);
      }
    } else if (key == KEY_CTRL_U) {
      edit_buf[0] = '\0';
      len = 0;
      pos = 0;
      draw_url_edit(edit_buf, pos, max_vis);
    } else if (key == 0x17) { /* Ctrl-W: delete last path segment */
      if (pos > 0) {
        old_pos = pos;
        if (pos > 0 && edit_buf[pos - 1] == '/')
          pos--;
        while (pos > 0 && edit_buf[pos - 1] != '/')
          pos--;
        for (i = 0; i <= len - old_pos; i++) {
          edit_buf[pos + i] = edit_buf[old_pos + i];
        }
        len -= (old_pos - pos);
        draw_url_edit(edit_buf, pos, max_vis);
      }
    } else if (key == KEY_LEFT || key == KEY_CTRL_B) {
      if (pos > 0) {
        pos--;
        draw_url_edit(edit_buf, pos, max_vis);
      }
    } else if (key == KEY_RIGHT || key == KEY_CTRL_F) {
      if (pos < len) {
        pos++;
        draw_url_edit(edit_buf, pos, max_vis);
      }
    } else if (key == KEY_HOME || key == KEY_CTRL_A) {
      pos = 0;
      draw_url_edit(edit_buf, pos, max_vis);
    } else if (key == KEY_END || key == KEY_CTRL_E) {
      pos = len;
      draw_url_edit(edit_buf, pos, max_vis);
    } else if (key >= 32 && key <= 126 && len < 125) {
      for (i = len; i >= pos; i--) {
        edit_buf[i + 1] = edit_buf[i];
      }
      edit_buf[pos] = (char)key;
      pos++;
      len++;
      draw_url_edit(edit_buf, pos, max_vis);
    }
  }
}


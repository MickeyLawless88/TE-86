/*  te_edit.c

    te-86 Text Editor - Line editor.

    TE (CP/M)  - Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    TE-86 (DOS) - Copyright (c) 2024-2026 Mickey W. Lawless

    Licensed under the GNU General Public License v2 or later.

    Changes (TE-86 DOS port):
    -----------------------------------------------------------------
    Features added:
      - K_TAB handler branches on cf_hard_tabs: when on, inserts a
        literal ASCII 0x09 tab character into the line buffer;
        when off, expands to spaces up to the next tab stop
        (original behaviour preserved)

    Bugs fixed:
      - Cursor column positioning updated to use cf_gutter macro
        so the cursor tracks correctly when line numbers are hidden
      - Horizontal scrolling: introduced box_hsc (horizontal scroll
        offset) so lines wider than the screen can be viewed and
        edited. The cursor is placed at (box_shc - box_hsc) + cf_gutter
        on screen, and box_hsc is adjusted whenever box_shc moves
        outside the visible window. The line is rendered from
        ln_dat + box_hsc so the visible portion is always correct.
*/

#include "te.h"

/* ---------------------------------------------------------------
   AdjHsc  --  recalculate box_hsc so box_shc is on-screen.
               vis_cols = usable text columns (cf_cols - cf_gutter).
               Keeps a small margin so the cursor doesn't sit at
               the extreme right edge after scrolling right.
   --------------------------------------------------------------- */
static void AdjHsc(void)
{
    int vis = (int)cf_cols - cf_gutter;
    if (vis < 1) vis = 1;

    /* Scroll right: cursor past right edge */
    while (box_shc - box_hsc >= vis)
        box_hsc += vis / 2 + 1;

    /* Scroll left: cursor before left edge */
    while (box_shc < box_hsc)
        box_hsc -= vis / 2 + 1;

    if (box_hsc < 0) box_hsc = 0;
}

/* ---------------------------------------------------------------
   BfEdit  --  edit the current line.
               Returns the key that terminated editing.
   --------------------------------------------------------------- */
int BfEdit(void)
{
    int i, ch, len, run, upd_lin, upd_col, upd_now, upd_cur, spc, old_len;

    editln = 1;

    strcpy(ln_dat, lp_arr[lp_cur] ? lp_arr[lp_cur] : "");
    len = old_len = strlen(ln_dat);
    run = upd_col = upd_now = upd_cur = 1;
    upd_lin = spc = 0;

    /* Clamp box_shc to line length, then re-sync scroll offset */
    if (box_shc > len)
        box_shc = len;
    AdjHsc();
    
    /* Force update of length display when entering a line */
    CrtLocate(PS_ROW, PS_COL_NOW);
    putint("%02d", len);

    while (run) {
        /* Redraw the current line by saving the working buffer back to
           lp_arr and calling Refresh for just this one screen row.
           Refresh handles line numbers, syntax highlighting, horizontal
           scroll, and block highlighting consistently with the rest of
           the editor -- no separate partial-draw logic needed here.    */
        if (upd_lin) {
            upd_lin = 0;
            spc     = 0;
            /* Save working buffer into lp_arr (handles realloc if line
               grew) so Refresh reads the current in-progress edit.    */
            ModifyLine(lp_cur, ln_dat);
            Refresh(box_shr, lp_cur);
        }

        /* Print line length in status bar */
        if (upd_now) {
            upd_now = 0;
            CrtLocate(PS_ROW, PS_COL_NOW);
            putint("%02d", len);
        }

        /* Print column number (1-based logical column) */
        if (upd_col) {
            upd_col = 0;
            CrtLocate(PS_ROW, PS_COL_CUR);
            putint("%02d", box_shc + 1);
        }

        /* Reposition cursor on screen */
        if (upd_cur) {
            upd_cur = 0;
            AdjHsc();
            CrtLocate(BOX_ROW + box_shr, (box_shc - box_hsc) + cf_gutter);
        }

        /* Get next character */
#if OPT_MACRO
        while ((ch = ForceGetCh()) == 0)
            ;
#else
        ch = ForceGetCh();
#endif

#if OPT_BLOCK
        /* Unselect block on any edit action */
        if (blk_start != -1 || blk_end != -1) {
            if (ch < 1000) {
                upd_cur = 1;
            }
            else {
                switch (ch) {
                    case K_CR:
                    case K_TAB:
                    case K_LDEL:
                    case K_RDEL:
                    case K_PASTE:
                    case K_MACRO:
                        upd_cur = 1;
                        break;
                }
            }

            if (upd_cur) {
                LoopBlkUnset();
                AdjHsc();
                CrtLocate(BOX_ROW + box_shr, (box_shc - box_hsc) + cf_gutter);
                upd_cur = 0;
            }
        }
#endif

        if (ch < 1000) {
            /* Printable / insertable character */
            if (len < ln_max) {
                for (i = len; i > box_shc; --i)
                    ln_dat[i] = ln_dat[i - 1];

                ln_dat[box_shc++] = (char)ch;
                ln_dat[++len] = '\0';

                ++upd_lin; ++upd_now; ++upd_col;

                if (cf_clang) {
#if OPT_MACRO
                    if (!MacroRunning()) {
#endif
                        switch (ch) {
                            case '[':  ForceChLeft(len, ']');  break;
                            case '{':  ForceChLeft(len, '}');  break;
                            case '(':  ForceChLeft(len, ')');  break;
                            case '"':  ForceChLeft(len, '"');  break;
                            case '\'': ForceChLeft(len, '\''); break;
                            case '*':
                                if (box_shc > 1 &&
                                    ln_dat[box_shc - 2] == '/' &&
                                    len + 1 < ln_max)
                                {
                                    ForceStr("*/");
                                    ForceCh(K_LEFT);
                                    ForceCh(K_LEFT);
                                }
                                break;
                        }
#if OPT_MACRO
                    }
#endif
                }
            }
            ++upd_cur;
        }
        else {
            switch (ch) {
                case K_LEFT:
                    if (box_shc) {
                        --box_shc; ++upd_col;
                    }
                    else if (lp_cur) {
                        box_shc = 9999;
                        ch = K_UP;
                        run = 0;
                    }
                    ++upd_cur;
                    break;

                case K_RIGHT:
                    if (box_shc < len) {
                        ++box_shc; ++upd_col;
                    }
                    else if (lp_cur < lp_now - 1) {
                        ch = K_DOWN;
                        box_shc = box_hsc = run = 0;
                    }
                    ++upd_cur;
                    break;

                case K_LDEL:
                    if (box_shc) {
                        strcpy(ln_dat + box_shc - 1, ln_dat + box_shc);
                        --box_shc; --len;
                        ++upd_now; ++upd_lin; ++upd_col;
                    }
                    else if (lp_cur) {
                        run = 0;
                    }
                    ++upd_cur;
                    break;

                case K_RDEL:
                    if (box_shc < len) {
                        strcpy(ln_dat + box_shc, ln_dat + box_shc + 1);
                        --len; ++upd_now; ++upd_lin;
                    }
                    else if (lp_cur < lp_now - 1) {
                        run = 0;
                    }
                    ++upd_cur;
                    break;

                case K_UP:
                    if (lp_cur) run = 0;
                    ++upd_cur;
                    break;

                case K_DOWN:
                    if (lp_cur < lp_now - 1) run = 0;
                    ++upd_cur;
                    break;

#if OPT_BLOCK
                case K_BLK_START:
                case K_BLK_END:
                case K_BLK_UNSET:
#endif
#if OPT_GOTO
                case K_GOTO:
#endif
#if OPT_MACRO
                case K_MACRO:
#endif
                case K_COPY:
                case K_CUT:
                case K_PASTE:
                case K_DELETE:
                case K_CLRCLP:
                case K_ESC:
                case K_CR:
                    run = 0;
                    break;

                case K_PGUP:
                case K_TOP:
                    if (lp_cur || box_shc) run = 0;
                    ++upd_cur;
                    break;

                case K_PGDOWN:
                case K_BOTTOM:
                    if (lp_cur < lp_now - 1 || box_shc != len) run = 0;
                    ++upd_cur;
                    break;

                case K_BEGIN:
                    if (box_shc || box_hsc) {
                        box_shc = box_hsc = 0;
                        ++upd_col;
                        ++upd_lin;
                    }
                    ++upd_cur;
                    break;

                case K_END:
                    if (box_shc != len) { box_shc = len; ++upd_col; }
                    ++upd_cur;
                    break;

                case K_TAB:
                    if (cf_hard_tabs) {
                        /* Insert a literal tab character */
                        if (ForceCh('\t')) break;
                    }
                    else {
                        /* Expand to spaces up to next tab stop */
                        i = cf_tab_cols - box_shc % cf_tab_cols;
                        while (i--)
                            if (ForceCh(' ')) break;
                    }
                    break;

#if OPT_LWORD
                case K_LWORD:
                    if (box_shc) {
                        if (ln_dat[box_shc] != ' ' && ln_dat[box_shc - 1] == ' ')
                            --box_shc;
                        while (box_shc && ln_dat[box_shc] == ' ')
                            --box_shc;
                        while (box_shc && ln_dat[box_shc] != ' ') {
                            if (ln_dat[--box_shc] == ' ') {
                                ++box_shc;
                                break;
                            }
                        }
                        ++upd_col;
                    }
                    ++upd_cur;
                    break;
#endif

#if OPT_RWORD
                case K_RWORD:
                    while (ln_dat[box_shc] && ln_dat[box_shc] != ' ')
                        ++box_shc;
                    while (ln_dat[box_shc] == ' ')
                        ++box_shc;
                    ++upd_col; ++upd_cur;
                    break;
#endif

#if OPT_FIND
                case K_FIND:
                    run = 0;
                    break;

                case K_NEXT:
                    if (find_str[0]) run = 0;
                    break;
#endif
            }
        }
    }

    /* Save changes back to the line array.
       If upd_lin fired during editing, ModifyLine was already called
       and lp_arr[lp_cur] already matches ln_dat.  We still call it
       here to cover the case where the user navigated without editing
       (upd_lin never fired).  lp_chg is set if anything changed.    */
    if (len != old_len || memcmp(lp_arr[lp_cur], ln_dat, len)) {
        ModifyLine(lp_cur, ln_dat);
        lp_chg = 1;
    }

    editln = 0;
    return ch;
}

/* ---------------------------------------------------------------
   Forced input buffer
   --------------------------------------------------------------- */

int ForceCh(int ch)
{
    if (fe_now < FORCED_MAX) {
        ++fe_now;
        if (fe_set == FORCED_MAX) fe_set = 0;
        fe_dat[fe_set++] = ch;
        return 0;
    }
    return -1;
}

int ForceStr(char *s)
{
    while (*s)
        if (ForceCh(*s++)) return -1;
    return 0;
}

int ForceGetCh(void)
{
    if (fe_now) {
        --fe_now;
        if (fe_get == FORCED_MAX) fe_get = 0;
        fe_forced = 1;
        return fe_dat[fe_get++];
    }

#if OPT_MACRO
    if (MacroRunning()) {
        MacroGet();
        if (fe_now)
            return ForceGetCh();
    }
#endif

    fe_forced = 0;
    return getchr();
}

void ForceChLeft(int len, int ch)
{
    if (!fe_forced && len < ln_max) {
        ForceCh(ch);
        ForceCh(K_LEFT);
    }
}

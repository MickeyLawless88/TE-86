/*  te_edit.c

    te-86 Text Editor - Edit line.

    Author: Mickey W. Lawless
    Date:   03/01/26

    Original CP/M version: Copyright (c) 2015-2021 Mickey W. Lawless
    MS-DOS Turbo C port: 2024
*/

#include "te.h"

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
    
    /* Force update of length display when entering a line */
    CrtLocate(PS_ROW, PS_COL_NOW);
    putint("%02d", len);

    if (box_shc > len)
        box_shc = len;

    while (run) {
        /* Print the current line */
        if (upd_lin) {
            upd_lin = 0;
            putstr(ln_dat + box_shc);
            if (spc) { putchr(' '); spc = 0; }
        }

        /* Print line length in status bar */
        if (upd_now) {
            upd_now = 0;
            CrtLocate(PS_ROW, PS_COL_NOW);
            putint("%02d", len);
        }

        /* Print column number */
        if (upd_col) {
            upd_col = 0;
            CrtLocate(PS_ROW, PS_COL_CUR);
            putint("%02d", box_shc + 1);
        }

        /* Reposition cursor */
        if (upd_cur) {
            upd_cur = 0;
            CrtLocate(BOX_ROW + box_shr, box_shc + cf_gutter);
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
                CrtLocate(BOX_ROW + box_shr, box_shc + cf_gutter);
                upd_cur = 0;
            }
        }
#endif

        if (ch < 1000) {
            /* Printable / insertable character */
            if (len < ln_max) {
                putchr(ch);

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
                        box_shc = run = 0;
                    }
                    ++upd_cur;
                    break;

                case K_LDEL:
                    if (box_shc) {
                        strcpy(ln_dat + box_shc - 1, ln_dat + box_shc);
                        --box_shc; --len;
                        ++upd_now; ++upd_lin; ++spc; ++upd_col;
                        putchr('\b');
                    }
                    else if (lp_cur) {
                        run = 0;
                    }
                    ++upd_cur;
                    break;

                case K_RDEL:
                    if (box_shc < len) {
                        strcpy(ln_dat + box_shc, ln_dat + box_shc + 1);
                        --len; ++upd_now; ++upd_lin; ++spc;
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
                    if (box_shc) { box_shc = 0; ++upd_col; }
                    ++upd_cur;
                    break;

                case K_END:
                    if (box_shc != len) { box_shc = len; ++upd_col; }
                    ++upd_cur;
                    break;

                case K_TAB:
                    i = cf_tab_cols - box_shc % cf_tab_cols;
                    while (i--)
                        if (ForceCh(' ')) break;
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

    /* Save changes back to the line array */
    if (len == old_len) {
        if (memcmp(lp_arr[lp_cur], ln_dat, len)) {
            strcpy(lp_arr[lp_cur], ln_dat);
            lp_chg = 1;
        }
    }
    else {
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

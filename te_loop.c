/*  te_loop.c

    te-86 Text Editor - Main editor loop.

    Author: Mickey W. Lawless
    Date:   03/01/26

    In the CP/M version this lived in te.c together with main().
    Separated here for clarity.

    Original CP/M version: Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    MS-DOS Turbo C port: 2024
*/

#include "te.h"

/* ---------------------------------------------------------------
   Internal helpers
   --------------------------------------------------------------- */

#if OPT_FIND
/* Find next occurrence of find_str starting from (line, col).
   Returns non-zero if found, updating lp_cur / box_shc.       */
static int DoFind(int line, int col)
{
    int i;
    char *p;

    for (i = line; i < lp_now; ++i) {
        p = lp_arr[i] + (i == line ? col : 0);
        if ((p = strstr(p, find_str))) {
            lp_cur  = i;
            box_shc = (int)(p - lp_arr[i]);
            return 1;
        }
    }
    return 0;
}
#endif

#if OPT_GOTO
static void DoGoto(void)
{
    char buf[8];
    int  n;

    buf[0] = '\0';
    if (SysLineStr("Go to line", buf, 6)) {
        n = atoi(buf) - 1;
        if (n >= 0 && n < lp_now) {
            lp_cur  = n;
            box_shc = 0;
        }
        else {
            ErrLine("Line out of range");
        }
    }
}
#endif

/* ---------------------------------------------------------------
   LoopBlkUnset  --  clear block selection and redraw
   --------------------------------------------------------------- */
#if OPT_BLOCK
void LoopBlkUnset(void)
{
    if (blk_count) {
        blk_start = blk_end = -1;
        blk_count = 0;
        RefreshAll();
    }
    else {
        blk_start = blk_end = -1;
    }
}
#endif

/* ---------------------------------------------------------------
   Loop  --  the main editing loop
   --------------------------------------------------------------- */
void Loop(void)
{
    int ch, run;

    /* Clipboard: a single line stored as a malloc'd string */
    char *clipboard = NULL;

    run = 1;

    /* Initial draw */
    RefreshAll();
    SysLineEdit();

    while (run) {
        /* Show cursor line / column in status bar */
        CrtLocate(PS_ROW, PS_LIN_CUR);
        putint("%04d", lp_cur + 1);

        CrtLocate(PS_ROW, PS_LIN_NOW);
        putint("%04d", lp_now);

        /* Position cursor in the editor box */
        CrtLocate(BOX_ROW + box_shr, box_shc + cf_num);

        /* Edit the current line */
        ch = BfEdit();

        switch (ch) {
            /* ---- Navigation ---- */
            case K_UP:
                if (lp_cur > 0) {
                    --lp_cur;
                    if (box_shr > 0) {
                        --box_shr;
                    }
                    else {
                        /* Scroll up */
                        Refresh(0, lp_cur - box_shr);
                    }
                    if (box_shc == 9999)
                        box_shc = strlen(lp_arr[lp_cur]);
                }
                break;

            case K_DOWN:
                if (lp_cur < lp_now - 1) {
                    ++lp_cur;
                    if (box_shr < box_rows - 1) {
                        ++box_shr;
                    }
                    else {
                        Refresh(0, lp_cur - box_shr);
                    }
                }
                break;

            case K_PGUP:
                if (lp_cur > 0) {
                    lp_cur -= box_rows;
                    if (lp_cur < 0) lp_cur = 0;
                    box_shr = 0;
                    box_shc = 0;
                    RefreshAll();
                }
                break;

            case K_PGDOWN:
                if (lp_cur < lp_now - 1) {
                    lp_cur += box_rows;
                    if (lp_cur >= lp_now) lp_cur = lp_now - 1;
                    box_shr = 0;
                    box_shc = 0;
                    RefreshAll();
                }
                break;

            case K_TOP:
                lp_cur = box_shr = box_shc = 0;
                RefreshAll();
                break;

            case K_BOTTOM:
                lp_cur  = lp_now - 1;
                box_shr = (lp_cur < box_rows) ? lp_cur : box_rows - 1;
                box_shc = strlen(lp_arr[lp_cur]);
                RefreshAll();
                break;

            /* ---- CR: split line ---- */
            case K_CR: {
                int indent = 0;
                int llen   = 0;
                int has_text = 0;
                char ibuf[256];
                char *src = lp_arr[lp_cur];

                /* Check if line has any non-whitespace content */
                {
                    char *p = src;
                    while (*p) {
                        if (*p != ' ' && *p != '\t') {
                            has_text = 1;
                            break;
                        }
                        ++p;
                    }
                }

                /* Auto-indent only if the line has actual text */
                if (cf_indent && has_text) {
                    while (*src == ' ' && llen < ln_max) {
                        ibuf[llen++] = ' ';
                        ++src;
                    }
                }

                /* Auto-list */
                if (cf_list && llen == 0 && has_text) {
                    src = lp_arr[lp_cur];
                    while (*src == ' ') ++src;
                    if (*src && strchr(cf_list_chr, *src)) {
                        ibuf[llen++] = *src++;
                        if (*src == ' ') ibuf[llen++] = ' ';
                    }
                }

                ibuf[llen] = '\0';
                indent = llen;

                if (SplitLine(lp_cur, box_shc)) {
                    /* Prepend indent to new line */
                    if (indent) {
                        char tmp[256];
                        strcpy(tmp, ibuf);
                        strcat(tmp, lp_arr[lp_cur + 1]);
                        ModifyLine(lp_cur + 1, tmp);
                    }

                    ++lp_cur;
                    box_shc = indent;

                    if (box_shr < box_rows - 1) {
                        ++box_shr;
                        Refresh(box_shr, lp_cur);
                    }
                    else {
                        Refresh(0, lp_cur - box_shr);
                    }

                    lp_chg = 1;
                }
                break;
            }

            /* ---- Backspace at start of line: join with previous ---- */
            case K_LDEL:
                if (lp_cur > 0) {
                    int prev_len = strlen(lp_arr[lp_cur - 1]);
                    if (JoinLines(lp_cur - 1)) {
                        --lp_cur;
                        box_shc = prev_len;

                        if (box_shr > 0) {
                            --box_shr;
                        }
                        Refresh(box_shr, lp_cur - box_shr + box_shr);
                        /* simpler: full redraw */
                        RefreshAll();
                        lp_chg = 1;
                    }
                }
                break;

            /* ---- Delete at end of line: join with next ---- */
            case K_RDEL:
                if (lp_cur < lp_now - 1) {
                    if (JoinLines(lp_cur)) {
                        RefreshAll();
                        lp_chg = 1;
                    }
                }
                break;

            /* ---- Cut ---- */
            case K_CUT:
                if (clipboard) { free(clipboard); clipboard = NULL; }
#if OPT_BLOCK
                if (blk_count) {
                    /* Cut whole block -- store first selected line,
                       delete all selected lines */
                    int first = blk_start;
                    int cnt   = blk_end - blk_start + 1;
                    clipboard = (char *)malloc(strlen(lp_arr[first]) + 1);
                    if (clipboard)
                        strcpy(clipboard, lp_arr[first]);
                    while (cnt-- && lp_now > 1)
                        DeleteLine(first);
                    if (lp_cur >= lp_now) lp_cur = lp_now - 1;
                    LoopBlkUnset();
                }
                else
#endif
                {
                    clipboard = (char *)malloc(strlen(lp_arr[lp_cur]) + 1);
                    if (clipboard)
                        strcpy(clipboard, lp_arr[lp_cur]);
                    if (lp_now > 1)
                        DeleteLine(lp_cur);
                    else
                        ClearLine(lp_cur);
                    if (lp_cur >= lp_now) lp_cur = lp_now - 1;
                }
                box_shc = 0;
                RefreshAll();
                lp_chg = 1;
                break;

            /* ---- Copy ---- */
            case K_COPY:
                if (clipboard) { free(clipboard); clipboard = NULL; }
#if OPT_BLOCK
                if (blk_count) {
                    clipboard = (char *)malloc(strlen(lp_arr[blk_start]) + 1);
                    if (clipboard)
                        strcpy(clipboard, lp_arr[blk_start]);
                    LoopBlkUnset();
                }
                else
#endif
                {
                    clipboard = (char *)malloc(strlen(lp_arr[lp_cur]) + 1);
                    if (clipboard)
                        strcpy(clipboard, lp_arr[lp_cur]);
                }
                break;

            /* ---- Paste ---- */
            case K_PASTE:
                if (clipboard) {
                    if (InsertLine(lp_cur, clipboard)) {
                        if (box_shr < box_rows - 1) ++box_shr;
                        RefreshAll();
                        lp_chg = 1;
                    }
                }
                break;

            /* ---- Delete line ---- */
            case K_DELETE:
#if OPT_BLOCK
                if (blk_count) {
                    int first = blk_start;
                    int cnt   = blk_end - blk_start + 1;
                    while (cnt-- && lp_now > 1)
                        DeleteLine(first);
                    if (lp_cur >= lp_now) lp_cur = lp_now - 1;
                    LoopBlkUnset();
                }
                else
#endif
                {
                    if (lp_now > 1)
                        DeleteLine(lp_cur);
                    else
                        ClearLine(lp_cur);
                    if (lp_cur >= lp_now) lp_cur = lp_now - 1;
                }
                box_shc = 0;
                RefreshAll();
                lp_chg = 1;
                break;

            /* ---- Clear clipboard ---- */
            case K_CLRCLP:
                if (clipboard) { free(clipboard); clipboard = NULL; }
                break;

#if OPT_BLOCK
            /* ---- Block start ---- */
            case K_BLK_START:
                blk_start = lp_cur;
                if (blk_end < blk_start) blk_end = blk_start;
                blk_count = blk_end - blk_start + 1;
                RefreshAll();
                break;

            /* ---- Block end ---- */
            case K_BLK_END:
                blk_end = lp_cur;
                if (blk_start < 0) blk_start = blk_end;
                if (blk_end < blk_start) { int t = blk_start; blk_start = blk_end; blk_end = t; }
                blk_count = blk_end - blk_start + 1;
                RefreshAll();
                break;

            /* ---- Block unset ---- */
            case K_BLK_UNSET:
                LoopBlkUnset();
                break;
#endif

#if OPT_FIND
            /* ---- Find ---- */
            case K_FIND: {
                char buf[FIND_MAX];
                buf[0] = '\0';
                if (SysLineStr("Find", buf, FIND_MAX - 1)) {
                    strcpy(find_str, buf);
                    if (!DoFind(lp_cur, box_shc + 1)) {
                        if (!DoFind(0, 0))
                            ErrLine("Not found");
                    }
                    RefreshAll();
                }
                break;
            }

            /* ---- Find next ---- */
            case K_NEXT:
                if (find_str[0]) {
                    if (!DoFind(lp_cur, box_shc + 1)) {
                        if (!DoFind(0, 0))
                            ErrLine("Not found");
                    }
                    RefreshAll();
                }
                break;
#endif

#if OPT_GOTO
            /* ---- Go to line ---- */
            case K_GOTO:
                DoGoto();
                box_shc = 0;
                RefreshAll();
                break;
#endif

#if OPT_MACRO
            /* ---- Run macro ---- */
            case K_MACRO: {
                char fn[FILENAME_MAX_LEN];
                fn[0] = '\0';
                if (SysLineStr("Macro file", fn, FILENAME_MAX_LEN - 1)) {
                    /* Append default extension if none given */
                    if (!strchr(fn, '.'))
                        strcat(fn, MAC_FTYPE);
                    MacroRunFile(fn, 0);
                }
                break;
            }
#endif

            /* ---- ESC: menu ---- */
            case K_ESC:
                if (Menu()) {
                    run = 0;
                }
                else {
                    Layout();
                    ShowFilename();
                    RefreshAll();
                    SysLineEdit();
                }
                break;

            default:
                break;
        }

        /* Bounds check */
        if (lp_cur < 0)         lp_cur = 0;
        if (lp_cur >= lp_now)   lp_cur = lp_now - 1;
        if (box_shc < 0)        box_shc = 0;

        /* Keep box_shr consistent */
        {
            int first = lp_cur - box_shr;
            if (first < 0) {
                box_shr = lp_cur;
            }
            else if (box_shr >= box_rows) {
                box_shr = box_rows - 1;
            }
        }

        /* Clear the system line if something wrote to it */
        if (sysln) {
            sysln = 0;
            SysLineEdit();
        }
    }

    if (clipboard)
        free(clipboard);
}

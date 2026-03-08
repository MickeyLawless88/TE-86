/*  te_file.c

    te-86 Text Editor - File I/O.

    TE (CP/M)  - Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    TE-86 (DOS) - Copyright (c) 2024-2026 Mickey W. Lawless

    Licensed under the GNU General Public License v2 or later.

    Changes (TE-86 DOS port):
    -----------------------------------------------------------------
    Features added:
      - Hard tab support: when cf_hard_tabs is on, ASCII 0x09 bytes
        are preserved in the line buffer rather than converted to
        spaces; WriteFile round-trips them unchanged

    Bugs fixed:
      - "Line too long" no longer aborts the file load.  Lines wider
        than LN_MAX_HARD (255) are now silently truncated: the excess
        characters are drained from the file with fgetc() so the next
        fgets() resumes at the correct position, and a one-time
        warning "Long line(s) truncated" is shown instead of halting
      - ReadFile used a local trunc counter (separate from rare) so
        the "Tabs changed to spaces" and "Illegal characters" messages
        are reported independently and correctly
*/

#include "te.h"

/* ---------------------------------------------------------------
   ResetLines  --  free all line buffers and reset state
   --------------------------------------------------------------- */
void ResetLines(void)
{
    FreeArray(lp_arr, cf_mx_lines, 0);

    lp_cur = lp_now = lp_chg = box_shr = box_shc = 0;

#if OPT_BLOCK
    blk_start = blk_end = -1;
    blk_count = 0;
#endif
}

/* ---------------------------------------------------------------
   NewFile  --  start a fresh, empty document
   --------------------------------------------------------------- */
void NewFile(void)
{
    ResetLines();
    file_name[0] = '\0';
    InsertLine(0, NULL);
}

/* ---------------------------------------------------------------
   ReadFile  --  load a text file into the editor buffer.
                 Returns non-zero on error.
   --------------------------------------------------------------- */
int ReadFile(char *fn)
{
    FILE *fp;
    int ch, code, len, i, tabs, rare, trunc;
    unsigned char *p;

    ResetLines();
    code = tabs = rare = trunc = 0;

    SysLine("Reading file... ");

    if (!(fp = fopen(fn, "r"))) {
        ErrLineOpen();
        return -1;
    }

    for (i = 0; i < 32000; ++i) {
        if (!fgets(ln_dat, ln_max + 2, fp))
            break;

        if (lp_now == cf_mx_lines) {
            ErrLineTooMany();
            ++code;
            break;
        }

        len = strlen(ln_dat);

        if (ln_dat[len - 1] == '\n') {
            /* Normal line - strip the newline */
            ln_dat[--len] = '\0';
        }
        else if (len >= ln_max) {
            /* Line is longer than our buffer - truncate it here and
               drain the rest of the line from the file so the next
               fgets starts at the correct position.  Warn once.     */
            if (!trunc)
                ErrLine("Long line(s) truncated");
            ++trunc;
            {
                int drain;
                do {
                    drain = fgetc(fp);
                } while (drain != '\n' && drain != EOF);
            }
            /* ln_dat[len] is already NUL from fgets - keep as-is */
        }
        /* else: last line of file with no trailing newline - keep as-is */

        if (!(lp_arr[lp_now] = AllocMem(len + 1))) {
            ++code;
            break;
        }

        p = (unsigned char *)strcpy(lp_arr[lp_now++], ln_dat);

        /* Replace control characters; tabs are kept or expanded
           depending on the cf_hard_tabs configuration setting.    */
        while ((ch = *p)) {
            if (ch < ' ') {
                if (ch == '\t') {
                    if (!cf_hard_tabs) {
                        *p = ' ';
                        ++tabs;
                    }
                    /* else: keep the literal \t in the buffer */
                }
                else {
                    *p = '?';
                    ++rare;
                }
            }
            ++p;
        }
    }

    fclose(fp);

    if (code)
        return -1;

    if (!lp_now)
        InsertLine(0, NULL);

    if (tabs)
        ErrLine("Tabs changed to spaces");

    if (rare)
        ErrLine("Illegal characters changed to '?'");

    return 0;
}

/* ---------------------------------------------------------------
   BackupFile  --  rename existing file to te.bkp
   --------------------------------------------------------------- */
void BackupFile(char *fn)
{
    FILE *fp;
    char *bkp = "te-86.bkp";

    if ((fp = fopen(fn, "r"))) {
        fclose(fp);
        remove(bkp);
        rename(fn, bkp);
    }
}

/* ---------------------------------------------------------------
   WriteFile  --  save buffer to file.
                  Returns non-zero on error.
   --------------------------------------------------------------- */
int WriteFile(char *fn)
{
    FILE *fp;
    int i;
    char *p;

    SysLine("Writing file... ");

    BackupFile(fn);

    fp = fopen(fn, "w");
    if (!fp) {
        ErrLineOpen();
        return -1;
    }

    for (i = 0; i < lp_now; ++i) {
        p = lp_arr[i];
        if (p && *p) {
            fputs(p, fp);
        }
        fputc('\n', fp);
    }

    if (ferror(fp)) {
        fclose(fp);
        remove(fn);
        ErrLine("Write error");
        return -1;
    }

    if (fclose(fp) == EOF) {
        remove(fn);
        ErrLine("Close error");
        return -1;
    }

    SysLine("File saved.");
    return (lp_chg = 0);
}

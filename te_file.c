/*  te_file.c

    te-86 Text Editor - File I/O.

    Author: Mickey W. Lawless
    Date:   03/01/26

    Original CP/M version: Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    MS-DOS Turbo C port: 2024
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
    int ch, code, len, i, tabs, rare;
    unsigned char *p;

    ResetLines();
    code = tabs = rare = 0;

    SysLine("Reading file... ");

    if (!(fp = fopen(fn, "r"))) {
        ErrLineOpen();
        return -1;
    }

    for (i = 0; i < 32000; ++i) {
        if (!fgets(ln_dat, ln_max + 2, fp))  /* ln_max + CR + NUL */
            break;

        if (lp_now == cf_mx_lines) {
            ErrLineTooMany();
            ++code;
            break;
        }

        len = strlen(ln_dat);

        if (ln_dat[len - 1] == '\n') {
            ln_dat[--len] = '\0';
        }
        else if (len > ln_max) {
            ErrLineLong();
            ++code;
            break;
        }

        if (!(lp_arr[lp_now] = AllocMem(len + 1))) {
            ++code;
            break;
        }

        p = (unsigned char *)strcpy(lp_arr[lp_now++], ln_dat);

        /* Replace tabs and control characters */
        while ((ch = *p)) {
            if (ch < ' ') {
                if (ch == '\t') {
                    *p = ' ';
                    ++tabs;
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

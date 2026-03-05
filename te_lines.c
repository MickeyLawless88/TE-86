/*  te_lines.c

    te-86 Text Editor - Operations with text lines.

    Author: Mickey W. Lawless
    Date:   03/01/26

    Original CP/M version: Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    MS-DOS Turbo C port: 2024
*/

#include "te.h"

/* ---------------------------------------------------------------
   GetFirstLine  --  line index of the first visible row
   --------------------------------------------------------------- */
int GetFirstLine(void)
{
    return lp_cur - box_shr;
}

/* ---------------------------------------------------------------
   GetLastLine  --  line index of the last visible row
   --------------------------------------------------------------- */
int GetLastLine(void)
{
    int last = GetFirstLine() + box_rows - 1;
    return (last >= lp_now - 1) ? lp_now - 1 : last;
}

/* ---------------------------------------------------------------
   SetLine  --  allocate and store text for a line.
                If insert != 0, shift lines down first.
                Returns non-zero on success.
   --------------------------------------------------------------- */
int SetLine(int line, char *text, int insert)
{
    char *p;
    int i;

    if (insert && lp_now >= cf_mx_lines) {
        ErrLineTooMany();
        return 0;
    }

    if (!text)
        text = "";

    if ((p = AllocMem((unsigned int)(strlen(text) + 1)))) {
        if (insert) {
            for (i = lp_now; i > line; --i)
                lp_arr[i] = lp_arr[i - 1];
            ++lp_now;
        }
        else {
            if (lp_arr[line])
                free(lp_arr[line]);
        }

        lp_arr[line] = strcpy(p, text);
        return 1;
    }

    return 0;
}

int ModifyLine(int line, char *text)
{
    return SetLine(line, text, 0);
}

int ClearLine(int line)
{
    return ModifyLine(line, NULL);
}

int InsertLine(int line, char *text)
{
    return SetLine(line, text, 1);
}

int AppendLine(int line, char *text)
{
    return InsertLine(line + 1, text);
}

/* ---------------------------------------------------------------
   SplitLine  --  split a line at position pos into two lines
   --------------------------------------------------------------- */
int SplitLine(int line, int pos)
{
    char *p, *p2;

    if ((p = AllocMem((unsigned int)(pos + 1)))) {
        if (AppendLine(line, lp_arr[line] + pos)) {
            p2 = lp_arr[line];
            p2[pos] = '\0';
            strcpy(p, p2);
            free(p2);
            lp_arr[line] = p;
            return 1;
        }
        free(p);
    }
    return 0;
}

/* ---------------------------------------------------------------
   DeleteLine  --  remove a line and shift the rest up
   --------------------------------------------------------------- */
int DeleteLine(int line)
{
    int i;

    free(lp_arr[line]);
    --lp_now;

    for (i = line; i < lp_now; ++i)
        lp_arr[i] = lp_arr[i + 1];

    lp_arr[lp_now] = NULL;
    return 1;
}

/* ---------------------------------------------------------------
   JoinLines  --  concatenate line+1 onto line and delete line+1
   --------------------------------------------------------------- */
int JoinLines(int line)
{
    char *p, *p1, *p2;
    int s1, s2;

    p1 = lp_arr[line];
    p2 = lp_arr[line + 1];
    s1 = strlen(p1);
    s2 = strlen(p2);

    if (s1 + s2 <= ln_max) {
        if ((p = AllocMem((unsigned int)(s1 + s2 + 1)))) {
            lp_arr[line] = strcat(strcpy(p, p1), p2);
            free(p1);
            DeleteLine(line + 1);
            return 1;
        }
    }
    return 0;
}

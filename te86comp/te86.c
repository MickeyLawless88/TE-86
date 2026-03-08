/*  te86.c

    te-86 Text Editor - Combined source file.

    Author: Mickey W. Lawless
    Date:   03/01/26

    This is a consolidated version with all source modules combined.
    Original CP/M version: Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    MS-DOS Turbo C port: 2024

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <dos.h>
#include <time.h>

#include "te.h"
#include "te_keys.h"

/* ============================================================================
   GLOBAL VARIABLE DEFINITIONS
   ============================================================================ */

/* Editor state */
char   **lp_arr    = NULL;    /* line pointer array                  */
int      lp_cur    = 0;       /* current line index                  */
int      lp_now    = 0;       /* total lines                         */
int      lp_chg    = 0;       /* unsaved changes flag                */
int      ln_max    = 128;     /* max editable line length (chars)    */
char    *ln_dat    = NULL;    /* working line buffer                 */
int      box_rows  = 0;       /* editor box height in rows           */
int      box_shr   = 0;       /* editor box: cursor row within box   */
int      box_shc   = 0;       /* editor box: cursor col              */
int      sysln     = 0;       /* system line needs refresh           */
int      editln    = 0;       /* non-zero while inside BfEdit()      */
char     file_name[FILENAME_MAX_LEN] = "";

/* Forced input ring buffer */
int      fe_dat[FORCED_MAX];
int      fe_now    = 0;       /* bytes currently in buffer           */
int      fe_get    = 0;       /* read index                          */
int      fe_set    = 0;       /* write index                         */
int      fe_forced = 0;       /* last char came from forced buffer   */

/* Find string */
#if OPT_FIND
char     find_str[FIND_MAX]  = "";
#endif

/* Block selection */
#if OPT_BLOCK
int      blk_start = -1;
int      blk_end   = -1;
int      blk_count = 0;
#endif

/* Macro state */
#if OPT_MACRO
FILE    *mac_fp    = NULL;
int      mac_raw   = 0;
char     mac_sym[MAC_SYM_SIZ] = "";
unsigned char mac_indent = 1;
unsigned char mac_list   = 1;
#endif

/* Help items table  (key-function codes, -1 = end-of-table) */
int help_items[] = {
    K_UP,        K_DOWN,      K_LEFT,
    K_RIGHT,     K_BEGIN,     K_END,
    K_TOP,       K_BOTTOM,    K_PGUP,
    K_PGDOWN,    K_TAB,       K_CR,
    K_ESC,       K_RDEL,      K_LDEL,
    K_CUT,       K_COPY,      K_PASTE,
    K_DELETE,    K_CLRCLP,    K_FIND,
    K_NEXT,      K_GOTO,      K_LWORD,
    K_RWORD,     K_BLK_START, K_BLK_END,
    K_BLK_UNSET, K_MACRO,     -1
};

/* ============================================================================
   CONFIGURATION (te_conf.c)
   ============================================================================ */

/*  te_conf.c

    te-86 Text Editor - Configuration block.

    Author: Mickey W. Lawless
    Date:   03/01/26

    Configuration is stored as a single byte array to guarantee
    contiguous memory layout for TECF patching.
*/

unsigned char cf_data[138] = {
	'T','E','_','C','O','N','F', 0, 0, 0,
	80,24,4,0,4,8,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,80,4,4,0,0,
	0,'-','*','>',0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,'-',
	':','|','-',0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,1,
	1,1,1,1,1,1,1,1,1,1,
	1,1,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0
};

unsigned char CfGetRows(void)
{
	return cf_data[44];
}

unsigned char CfGetCols(void)
{
	return cf_data[45];
}

unsigned char CfGetTabCols(void)
{
	return cf_data[46];
}

unsigned char CfGetNum(void)
{
	return cf_data[47];
}

unsigned char CfGetClang(void)
{
	return cf_data[48];
}

unsigned char CfGetIndent(void)
{
	return cf_data[49];
}

unsigned char CfGetList(void)
{
	return cf_data[50];
}

char *CfGetListChr(void)
{
	return (char *)&cf_data[51];
}

unsigned char CfGetRulChr(void)
{
	return cf_data[59];
}

unsigned char CfGetRulTab(void)
{
	return cf_data[60];
}

unsigned char CfGetVertChr(void)
{
	return cf_data[61];
}

unsigned char CfGetHorzChr(void)
{
	return cf_data[62];
}

unsigned char CfGetLnumChr(void)
{
	return cf_data[63];
}

unsigned char *CfGetKeys(void)
{
	return &cf_data[80];
}

unsigned char *CfGetKeysEx(void)
{
	return &cf_data[109];
}

void CfSetRows(unsigned char v)
{
    cf_data[44] = v;
}

void CfSetCols(unsigned char v)
{
    cf_data[45] = v;
}

void CfSetIndent(unsigned char v)
{
    cf_data[49] = v;
}

void CfSetList(unsigned char v)
{
    cf_data[50] = v;
}

void CfSetTabCols(unsigned char v)
{
    cf_data[46] = v;
}

void CfSetNum(unsigned char v)
{
    cf_data[47] = v;
}

void CfSetClang(unsigned char v)
{
    cf_data[48] = v;
}

void CfSetRulChr(unsigned char v)
{
    cf_data[59] = v;
}

void CfSetRulTab(unsigned char v)
{
    cf_data[60] = v;
}

void CfSetVertChr(unsigned char v)
{
    cf_data[61] = v;
}

void CfSetHorzChr(unsigned char v)
{
    cf_data[62] = v;
}

void CfSetLnumChr(unsigned char v)
{
    cf_data[63] = v;
}

void CfSetListChr(char *s)
{
    int i;
    for (i = 0; i < 7 && s[i]; ++i)
        cf_data[51 + i] = s[i];
    cf_data[51 + i] = '\0';
}

int CfSave(void)
{
    FILE *fp;
    
    fp = fopen("TE-86.CFG", "wb");
    if (!fp)
        return -1;
    
    if (fwrite(cf_data, 1, 138, fp) != 138) {
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    return 0;
}

int CfLoad(void)
{
    FILE *fp;
    unsigned char buf[138];
    
    fp = fopen("TE-86.CFG", "rb");
    if (!fp)
        return -1;
    
    if (fread(buf, 1, 138, fp) != 138) {
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    
    if (buf[0] != 'T' || buf[1] != 'E' || buf[2] != '_' ||
        buf[3] != 'C' || buf[4] != 'O' || buf[5] != 'N' || buf[6] != 'F') {
        return -1;
    }
    
    memcpy(cf_data + 10, buf + 10, 128);
    
    return 0;
}

/* ============================================================================
   KEYS (te_keys.c)
   ============================================================================ */

char *GetKeyName(int key)
{
    switch (key) {
        case K_CR:  return (char *)&cf_data[64];
        case K_ESC: return (char *)&cf_data[72];
    }
    return "?";
}

char *GetKeyWhat(int key)
{
    switch (key) {
        case K_UP:        return "Up";
        case K_DOWN:      return "Down";
        case K_LEFT:      return "Left";
        case K_RIGHT:     return "Right";
        case K_BEGIN:     return "Begin";
        case K_END:       return "End";
        case K_TOP:       return "Top";
        case K_BOTTOM:    return "Bottom";
        case K_PGUP:      return "PgUp";
        case K_PGDOWN:    return "PgDown";
        case K_TAB:       return "Indent";
        case K_CR:        return "NewLine";
        case K_ESC:       return "Escape";
        case K_RDEL:      return "DelRight";
        case K_LDEL:      return "DelLeft";
        case K_CUT:       return "Cut";
        case K_COPY:      return "Copy";
        case K_PASTE:     return "Paste";
        case K_DELETE:    return "Delete";
        case K_CLRCLP:    return "ClearClip";
#if OPT_FIND
        case K_FIND:      return "Find";
        case K_NEXT:      return "FindNext";
#endif
#if OPT_GOTO
        case K_GOTO:      return "GoLine";
#endif
#if OPT_LWORD
        case K_LWORD:     return "WordLeft";
#endif
#if OPT_RWORD
        case K_RWORD:     return "WordRight";
#endif
#if OPT_BLOCK
        case K_BLK_START: return "BlockStart";
        case K_BLK_END:   return "BlockEnd";
        case K_BLK_UNSET: return "BlockUnset";
#endif
#if OPT_MACRO
        case K_MACRO:     return "Macro";
#endif
    }
    return "?";
}

int GetKey(void)
{
    int c, x, i;
    unsigned char *keys = CfGetKeys();
    unsigned char *keys_ex = CfGetKeysEx();

    c = CrtIn();

    if (c > 31 && c != 127) {
        return c;
    }

    if (c == 27)  return K_ESC;
    if (c == 13)  return K_CR;
    if (c == 9)   return K_TAB;
    if (c == 8)   return K_LDEL;

    if (c == 0 || c == 0xE0) {
        x = CrtIn();
        for (i = 0; i < KEYS_MAX; ++i) {
            if (keys[i] == 0 && keys_ex[i] == x) {
                return i + 1000;
            }
        }
        return '?';
    }

    for (i = 0; i < KEYS_MAX; ++i) {
        if (keys[i] && c == keys[i] && keys_ex[i] == 0) {
            return i + 1000;
        }
    }

    return '?';
}

/* ============================================================================
   CRT ABSTRACTION (te_crt.c)
   ============================================================================ */

static int crt_reverse = 0;
#define ATTR_NORMAL  0x07
#define ATTR_REVERSE 0x70

void CrtInit(void)
{
    textmode(C80);
    textattr(ATTR_NORMAL);
    clrscr();
    _setcursortype(_NORMALCURSOR);
    crt_reverse = 0;
}

void CrtDone(void)
{
    textattr(ATTR_NORMAL);
    clrscr();
    _setcursortype(_NORMALCURSOR);
    gotoxy(1, 1);
}

void CrtClear(void)
{
    textattr(crt_reverse ? ATTR_REVERSE : ATTR_NORMAL);
    clrscr();
}

void CrtClearLine(int row)
{
    gotoxy(1, row + 1);
    textattr(ATTR_NORMAL);
    clreol();
}

void CrtClearEol(void)
{
    textattr(ATTR_NORMAL);
    clreol();
}

void CrtLocate(int row, int col)
{
    gotoxy(col + 1, row + 1);
}

void CrtOut(int ch)
{
    int x, y;
    switch (ch) {
        case '\b':
            x = wherex();
            if (x > 1) {
                gotoxy(x - 1, wherey());
                putch(' ');
                gotoxy(x - 1, wherey());
            }
            break;
        case '\n':
            y = wherey();
            gotoxy(1, y + 1);
            break;
        case '\r':
            gotoxy(1, wherey());
            break;
        default:
            putch(ch);
            break;
    }
}

int CrtIn(void)
{
    static int pending = -1;
    int ch;

    if (pending >= 0) {
        ch = pending;
        pending = -1;
        return ch;
    }

    ch = getch();

    if (ch == 0 || ch == 0xE0) {
        pending = getch();
        return 0;
    }

    return ch;
}

void CrtReverse(int on)
{
    crt_reverse = on;
    textattr(on ? ATTR_REVERSE : ATTR_NORMAL);
}

void CrtGetSize(int *rows, int *cols)
{
    struct text_info ti;
    gettextinfo(&ti);
    *rows = ti.screenheight;
    *cols = ti.screenwidth;
}

/* ============================================================================
   ERROR MESSAGES (te_err.c)
   ============================================================================ */

void ErrLine(char *s)
{
    SysLineCont(s);
    if (editln) {
        SysLineEdit();
        sysln = 0;
        CrtLocate(BOX_ROW + box_shr, box_shc + CfGetNum());
    }
}

void ErrLineMem(void)    { ErrLine("Not enough memory"); }
void ErrLineLong(void)   { ErrLine("Line too long");     }
void ErrLineOpen(void)   { ErrLine("Can't open");        }
void ErrLineTooMany(void){ ErrLine("Too many lines");    }

/* ============================================================================
   MISCELLANEOUS (te_misc.c)
   ============================================================================ */

char *AllocMem(unsigned int bytes)
{
    char *p;

    if (!(p = (char *)malloc(bytes)))
        ErrLineMem();

    return p;
}

void *FreeArray(char **arr, int count, int flag)
{
    int i;

    for (i = 0; i < count; ++i) {
        if (arr[i]) {
            free(arr[i]);
            arr[i] = NULL;
        }
    }

    if (flag)
        free(arr);

    return NULL;
}

#if OPT_MACRO
int MatchStr(char *s1, char *s2)
{
    return !strcmp(s1, s2);
}
#endif

/* ============================================================================
   LINES (te_lines.c)
   ============================================================================ */

int GetFirstLine(void)
{
    return lp_cur - box_shr;
}

int GetLastLine(void)
{
    int last = GetFirstLine() + box_rows - 1;
    return (last >= lp_now - 1) ? lp_now - 1 : last;
}

int SetLine(int line, char *text, int insert)
{
    char *p;
    int i;

    if (insert && lp_now >= *((int*)&cf_data[10])) {
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

/* ============================================================================
   FILE I/O (te_file.c)
   ============================================================================ */

void ResetLines(void)
{
    FreeArray(lp_arr, *((int*)&cf_data[10]), 0);

    lp_cur = lp_now = lp_chg = box_shr = box_shc = 0;

#if OPT_BLOCK
    blk_start = blk_end = -1;
    blk_count = 0;
#endif
}

void NewFile(void)
{
    ResetLines();
    file_name[0] = '\0';
    InsertLine(0, NULL);
}

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
        if (!fgets(ln_dat, ln_max + 2, fp))
            break;

        if (lp_now == *((int*)&cf_data[10])) {
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

/* ============================================================================
   EDIT LINE (te_edit.c)
   ============================================================================ */

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

int BfEdit(void)
{
    int i, ch, len, run, upd_lin, upd_col, upd_now, upd_cur, spc, old_len;

    editln = 1;

    strcpy(ln_dat, lp_arr[lp_cur] ? lp_arr[lp_cur] : "");
    len = old_len = strlen(ln_dat);
    run = upd_col = upd_now = upd_cur = 1;
    upd_lin = spc = 0;
    
    CrtLocate(PS_ROW, PS_COL_NOW);
    putint("%02d", len);

    if (box_shc > len)
        box_shc = len;

    while (run) {
        if (upd_lin) {
            upd_lin = 0;
            putstr(ln_dat + box_shc);
            if (spc) { putchr(' '); spc = 0; }
        }

        if (upd_now) {
            upd_now = 0;
            CrtLocate(PS_ROW, PS_COL_NOW);
            putint("%02d", len);
        }

        if (upd_col) {
            upd_col = 0;
            CrtLocate(PS_ROW, PS_COL_CUR);
            putint("%02d", box_shc + 1);
        }

        if (upd_cur) {
            upd_cur = 0;
            CrtLocate(BOX_ROW + box_shr, box_shc + CfGetNum());
        }

#if OPT_MACRO
        while ((ch = ForceGetCh()) == 0)
            ;
#else
        ch = ForceGetCh();
#endif

#if OPT_BLOCK
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
                CrtLocate(BOX_ROW + box_shr, box_shc + CfGetNum());
                upd_cur = 0;
            }
        }
#endif

        if (ch < 1000) {
            if (len < ln_max) {
                putchr(ch);

                for (i = len; i > box_shc; --i)
                    ln_dat[i] = ln_dat[i - 1];

                ln_dat[box_shc++] = (char)ch;
                ln_dat[++len] = '\0';

                ++upd_lin; ++upd_now; ++upd_col;

                if (CfGetClang()) {
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
                    i = CfGetTabCols() - box_shc % CfGetTabCols();
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

/* ============================================================================
   UI FUNCTIONS (te_ui.c - partial)
   ============================================================================ */

void putchrx(int ch, int n)
{
    while (n--)
        putchr(ch);
}

void putstr(char *s)
{
    while (*s)
        putchr(*s++);
}

void putln(char *s)
{
    putstr(s);
    putchr('\n');
}

void putint(char *fmt, int val)
{
    char r[16];
    sprintf(r, fmt, val);
    putstr(r);
}

void Layout(void)
{
    int i, k, w;

    CrtClear();

    CrtLocate(PS_ROW, PS_INF);
    putstr(PS_TXT);

    CrtLocate(PS_ROW, PS_LIN_MAX);
    putint("%04d", *((int*)&cf_data[10]));

    CrtLocate(PS_ROW, PS_COL_MAX);
    putint("%02d", 1 + ln_max);

#if CRT_LONG
    CrtLocate(BOX_ROW - 1, CfGetNum());

    w = CfGetCols() - CfGetNum();

    for (i = k = 0; i < w; ++i) {
        if (k++) {
            putchr(CfGetRulChr());
            if (k == CfGetTabCols())
                k = 0;
        }
        else {
            putchr(CfGetRulTab());
        }
    }

    CrtLocate(CfGetRows() - 2, 0);
    putchrx(CfGetHorzChr(), CfGetCols());
#endif
}

void ShowFilename(void)
{
    char *s;
    int maxlen;
    
    CrtLocate(PS_ROW, PS_FNAME);
    s = CurrentFile();
    
    maxlen = PS_INF - PS_FNAME - 1;
    if (maxlen < 1) maxlen = 1;
    
    if ((int)strlen(s) > maxlen) {
        while (maxlen-- > 0)
            putchr(*s++);
    }
    else {
        putstr(s);
        putchrx(' ', maxlen - (int)strlen(s));
    }
}

void SysLine(char *s)
{
    CrtClearLine(CfGetRows() - 1);
    if (s)
        putstr(s);
    sysln = 1;
}

void SysLineEdit(void)
{
    SysLine(GetKeyName(K_ESC));
    putstr(" = menu");
}

void SysLineCont(char *s)
{
    SysLineWait(s, NULL, "continue");
}

void SysLineBack(char *s)
{
    SysLineWait(s, NULL, "back");
}

int SysLineConf(char *s)
{
    return SysLineWait(s, "continue", "cancel");
}

int SysLineWait(char *s, char *cr, char *esc)
{
    int ch;

    SysLine(s);

    if (s)
        putstr(" (");

    if (cr) {
        putstr(GetKeyName(K_CR));
        putstr(" = ");
        putstr(cr);
        putstr(", ");
    }

    if (esc) {
        putstr(GetKeyName(K_ESC));
        putstr(" = ");
        putstr(esc);
    }

    if (s)
        putchr(')');

    putstr(": ");

    for (;;) {
        ch = getchr();
        if (cr  && ch == K_CR)  break;
        if (esc && ch == K_ESC) break;
    }

    SysLine(NULL);

    return (ch == K_CR);
}

int SysLineStr(char *what, char *buf, int maxlen)
{
    int ch;

    SysLine(what);
    putstr(" (");
    putstr(GetKeyName(K_ESC));
    putstr(" = cancel): ");

    ch = ReadLine(buf, maxlen);

    SysLine(NULL);

    if (ch == K_CR && *buf)
        return 1;

    return 0;
}

int SysLineFile(char *fn)
{
    return SysLineStr("Filename", fn, FILENAME_MAX_LEN - 1);
}

int SysLineChanges(void)
{
    return SysLineConf("Changes will be lost!");
}

int ReadLine(char *buf, int width)
{
    int len, ch;

    putstr(buf);
    len = strlen(buf);

    for (;;) {
        switch ((ch = getchr())) {
            case K_LDEL:
                if (len) {
                    putchr('\b');
                    putchr(' ');
                    putchr('\b');
                    --len;
                }
                break;
            case K_CR:
            case K_ESC:
                buf[len] = '\0';
                return ch;
            default:
                if (len < width && ch >= ' ')
                    putchr(buf[len++] = (char)ch);
                break;
        }
    }
}

char *CurrentFile(void)
{
    return (file_name[0] ? file_name : "-");
}

void ClearBox(void)
{
    int i;
    for (i = 0; i < box_rows; ++i)
        CrtClearLine(BOX_ROW + i);
}

void CenterText(int row, char *txt)
{
    CrtLocate(row, (CfGetCols() - (int)strlen(txt)) / 2);
    putstr(txt);
}

#if OPT_BLOCK
void RefreshBlock(int row, int sel)
{
    int i, line;

    line = GetFirstLine() + row;

    for (i = row; i < box_rows; ++i) {
        if (line >= blk_start) {
            if (line <= blk_end) {
#if CRT_CAN_REV
                CrtLocate(BOX_ROW + i, CfGetNum());
                CrtClearEol();
                if (sel) CrtReverse(1);
                putstr(lp_arr[line]);
                putchr(' ');
                if (sel) CrtReverse(0);
#else
                CrtLocate(BOX_ROW + i, CfGetCols() - 1);
                putchr(sel ? '*' : ' ');
#endif
            }
            else {
                break;
            }
        }
        ++line;
    }
}
#endif

void Refresh(int row, int line)
{
    int i;
    char format[8];

#if OPT_BLOCK
    int blk, sel;
    blk = (blk_count &&
           blk_start <= GetLastLine() &&
           blk_end   >= GetFirstLine());
    sel = 0;
#endif

    if (CfGetNum()) {
        strcpy(format, "%?d");
        format[1] = (char)('0' + CfGetNum() - 1);
    }

    for (i = row; i < box_rows; ++i) {
        CrtClearLine(BOX_ROW + i);

        if (line < lp_now) {
            if (CfGetNum()) {
                putint(format, line + 1);
                putchr(CfGetLnumChr());
            }

#if OPT_BLOCK
            if (blk) {
                if (line >= blk_start && line <= blk_end) {
#if CRT_CAN_REV
                    CrtReverse((sel = 1));
#else
                    sel = 1;
#endif
                }
            }
#endif

            putstr(lp_arr[line++]);

#if OPT_BLOCK
            if (sel) {
#if CRT_CAN_REV
                putchr(' ');
                CrtReverse((sel = 0));
#else
                sel = 0;
                CrtLocate(BOX_ROW + i, CfGetCols() - 1);
                putchr('*');
#endif
            }
#endif
        }
    }
}

void RefreshAll(void)
{
    Refresh(0, lp_cur - box_shr);
}

/* Forward declarations */
int MenuNew(void);
int MenuOpen(void);
int MenuSave(void);
int MenuSaveAs(void);
#if OPT_MACRO
int MenuInsert(void);
#endif
void MenuHelp(void);
void MenuHelpCh(int ch);
void MenuConfig(void);
void MenuAbout(void);
int MenuExit(void);

int Menu(void)
{
    int run, row, stay, menu, ask;

    run = stay = menu = ask = 1;

    while (run) {
        if (menu) {
            row = BOX_ROW + 1;
            ClearBox();
            CenterText(row++, "OPTIONS");
            row++;
#if CRT_LONG
            CenterText(row++, "New");
            CenterText(row++, "Open");
            CenterText(row++, "Save");
            CenterText(row++, "save As");
#if OPT_MACRO
            CenterText(row++, "Insert");
#endif
            CenterText(row++, "Config");
            CenterText(row++, "Help");
            CenterText(row++, "aBout te-86");
            CenterText(row,   "eXit te-86");
#else
#if OPT_MACRO
            CenterText(row++, "New   Open    Save    Save As");
            CenterText(row++, "Insert Config Help aBout eXit");
#else
            CenterText(row++, "New   Open    Save   Save As");
            CenterText(row++, "Config  Help  aBout  eXit   ");
#endif
#endif
            menu = 0;
        }

        if (ask) {
            SysLine("Option (");
            putstr(GetKeyName(K_ESC));
            putstr(" = back): ");
        }
        else {
            ask = 1;
        }

        switch (toupper(getchr())) {
            case 'N':    run = MenuNew();    break;
            case 'O':    run = MenuOpen();   break;
            case 'S':    run = MenuSave();   break;
            case 'A':    run = MenuSaveAs(); break;
#if OPT_MACRO
            case 'I':    run = MenuInsert(); break;
#endif
            case 'C':    MenuConfig(); menu = 1; break;
            case 'B':    MenuAbout(); menu = 1; break;
            case 'H':    MenuHelp();  menu = 1; break;
            case 'X':    run = stay = MenuExit(); break;
            case K_ESC:  run = 0; break;
            default:     ask = 0; break;
        }
    }

    ClearBox();
    SysLine(NULL);

    return !stay;
}

int MenuNew(void)
{
    if (lp_chg && !SysLineChanges())
        return 1;
    NewFile();
    return 0;
}

int MenuOpen(void)
{
    char fn[FILENAME_MAX_LEN];

    if (lp_chg && !SysLineChanges())
        return 1;

    fn[0] = '\0';

    if (SysLineFile(fn)) {
        if (ReadFile(fn))
            NewFile();
        else
            strcpy(file_name, fn);
        return 0;
    }
    return 1;
}

int MenuSave(void)
{
    if (!file_name[0])
        return MenuSaveAs();
    WriteFile(file_name);
    return 1;
}

int MenuSaveAs(void)
{
    char fn[FILENAME_MAX_LEN];

    strcpy(fn, file_name);

    if (SysLineFile(fn)) {
        if (!WriteFile(fn))
            strcpy(file_name, fn);
        return 0;
    }
    return 1;
}

#if OPT_MACRO
int MenuInsert(void)
{
    char fn[FILENAME_MAX_LEN];
    fn[0] = '\0';
    if (SysLineFile(fn))
        return MacroRunFile(fn, 1);
    return 1;
}
#endif

void MenuHelp(void)
{
    int i, k;
    char *s;

    ClearBox();
    CrtLocate(BOX_ROW + 1, 0);
    putln("HELP:\n");

#if CRT_LONG
    for (i = 0; help_items[i] != -1; ++i) {
        if ((k = help_items[i])) {
            if (*(s = GetKeyWhat(k)) == '?')
                k = 0;
        }

        if (k) {
            putstr(s);
            putchrx(' ', 11 - (int)strlen(s));

            k -= 1000;
            MenuHelpCh(CfGetKeys()[k]);
            MenuHelpCh(CfGetKeysEx()[k]);
        }
        else {
            putchrx(' ', 15);
        }

        if ((i + 1) % 3) {
            putchr(' ');
            putchr(CfGetVertChr());
            putchr(' ');
        }
        else {
            putchr('\n');
        }
    }
#else
    putstr("Sorry, no help is available.");
#endif

    SysLineBack(NULL);
}

void MenuHelpCh(int ch)
{
    if (ch) {
        if (ch < 32 || ch == 0x7F) {
            putchr('^');
            putchr(ch != 0x7F ? '@' + ch : '?');
        }
        else {
            putchr(ch);
            putchr(' ');
        }
    }
    else {
        putchr(' ');
        putchr(' ');
    }
}

void MenuConfig(void)
{
    int row, ch, val;
    char buf[16];

    for (;;) {
        row = BOX_ROW + 1;
        ClearBox();
        CenterText(row++, "CONFIGURATION");
        row++;
        
        CrtLocate(row++, 2);
        putstr("1. Tab width:       ");
        putint("%d", CfGetTabCols());
        
        CrtLocate(row++, 2);
        putstr("2. Line num width:  ");
        putint("%d", CfGetNum());
        
        CrtLocate(row++, 2);
        putstr("3. Auto-indent:     ");
        putstr(CfGetIndent() ? "ON " : "OFF");
        
        CrtLocate(row++, 2);
        putstr("4. Auto-list:       ");
        putstr(CfGetList() ? "ON " : "OFF");
        
        CrtLocate(row++, 2);
        putstr("5. List bullets:    ");
        putstr(CfGetListChr());
        
        CrtLocate(row++, 2);
        putstr("6. C-lang complete: ");
        putstr(CfGetClang() ? "ON " : "OFF");
        
        CrtLocate(row++, 2);
        putstr("7. Ruler char:      ");
        putchr(CfGetRulChr());
        
        CrtLocate(row++, 2);
        putstr("8. Ruler tab char:  ");
        putchr(CfGetRulTab());
        
        CrtLocate(row++, 2);
        putstr("9. Line num char:   ");
        putchr(CfGetLnumChr());
        
        CrtLocate(row++, 2);
        putstr("0. Horiz line char: ");
        putchr(CfGetHorzChr());
        
        row++;
        CrtLocate(row++, 2);
        putstr("S. Save and exit config");
        
        SysLine("Option (Esc = cancel): ");
        
        ch = toupper(getchr());
        
        switch (ch) {
            case '1':
                SysLine("Tab width (1-8): ");
                buf[0] = '\0';
                if (ReadLine(buf, 2) == K_CR && buf[0]) {
                    val = atoi(buf);
                    if (val >= 1 && val <= 8) {
                        CfSetTabCols((unsigned char)val);
                    }
                }
                break;
                
            case '2':
                SysLine("Line number width (0-6): ");
                buf[0] = '\0';
                if (ReadLine(buf, 2) == K_CR && buf[0]) {
                    val = atoi(buf);
                    if (val >= 0 && val <= 6) {
                        CfSetNum((unsigned char)val);
                    }
                }
                break;
                
            case '3':
                CfSetIndent(CfGetIndent() ? 0 : 1);
                break;
                
            case '4':
                CfSetList(CfGetList() ? 0 : 1);
                break;
                
            case '5':
                SysLine("List bullets (e.g. -*>): ");
                buf[0] = '\0';
                if (ReadLine(buf, 7) == K_CR && buf[0]) {
                    CfSetListChr(buf);
                }
                break;
                
            case '6':
                CfSetClang(CfGetClang() ? 0 : 1);
                break;
                
            case '7':
                SysLine("Ruler char: ");
                ch = getchr();
                if (ch > 31 && ch < 127) {
                    CfSetRulChr((unsigned char)ch);
                }
                break;
                
            case '8':
                SysLine("Ruler tab char: ");
                ch = getchr();
                if (ch > 31 && ch < 127) {
                    CfSetRulTab((unsigned char)ch);
                }
                break;
                
            case '9':
                SysLine("Line number separator char: ");
                ch = getchr();
                if (ch > 31 && ch < 127) {
                    CfSetLnumChr((unsigned char)ch);
                }
                break;
                
            case '0':
                SysLine("Horizontal line char: ");
                ch = getchr();
                if (ch > 31 && ch < 127) {
                    CfSetHorzChr((unsigned char)ch);
                }
                break;
                
            case 'S':
                if (CfSave() == 0) {
                    SysLine("Configuration saved to TE-86.CFG");
                    getchr();
                }
                else {
                    SysLine("Error saving config!");
                    getchr();
                }
                SysLine(NULL);
                return;
                
            case K_ESC:
                SysLine(NULL);
                return;
        }
    }
}

void MenuAbout(void)
{
    int row;

#if CRT_LONG
    row = BOX_ROW + 1;
    ClearBox();
    CenterText(row++, "te-86 Text Editor");
    row++;
    CenterText(row++, VERSION);
    row++;
    CenterText(row++, "Configured for");
    CenterText(row++, (char *)&cf_data[12]);
    row++;
    CenterText(row++, COPYRIGHT);
    row++;
    CenterText(row++, "http://www.floppysoftware.es");
    CenterText(row++, "https://cpm-connections.blogspot.com");
    CenterText(row,   "floppysoftware@gmail.com");
#else
    row = BOX_ROW;
    ClearBox();
    CenterText(row++, "te-86 Text Editor");
    CenterText(row++, VERSION);
    CenterText(row++, "Configured for");
    CenterText(row++, (char *)&cf_data[12]);
    CenterText(row++, COPYRIGHT);
    CenterText(row++, "www.floppysoftware.es");
#endif

    SysLineBack(NULL);
}

int MenuExit(void)
{
    if (lp_chg && !SysLineChanges())
        return 1;
    return 0;
}

/* ============================================================================
   MAIN LOOP (te_loop.c)
   ============================================================================ */

#if OPT_FIND
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

void Loop(void)
{
    int ch, run;
    char *clipboard = NULL;

    run = 1;

    RefreshAll();
    SysLineEdit();

    while (run) {
        CrtLocate(PS_ROW, PS_LIN_CUR);
        putint("%04d", lp_cur + 1);

        CrtLocate(PS_ROW, PS_LIN_NOW);
        putint("%04d", lp_now);

        CrtLocate(BOX_ROW + box_shr, box_shc + CfGetNum());

        ch = BfEdit();

        switch (ch) {
            case K_UP:
                if (lp_cur > 0) {
                    --lp_cur;
                    if (box_shr > 0) {
                        --box_shr;
                    }
                    else {
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

            case K_CR: {
                int indent = 0;
                int llen   = 0;
                int has_text = 0;
                char ibuf[256];
                char *src = lp_arr[lp_cur];

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

                if (CfGetIndent() && has_text) {
                    while (*src == ' ' && llen < ln_max) {
                        ibuf[llen++] = ' ';
                        ++src;
                    }
                }

                if (CfGetList() && llen == 0 && has_text) {
                    src = lp_arr[lp_cur];
                    while (*src == ' ') ++src;
                    if (*src && strchr(CfGetListChr(), *src)) {
                        ibuf[llen++] = *src++;
                        if (*src == ' ') ibuf[llen++] = ' ';
                    }
                }

                ibuf[llen] = '\0';
                indent = llen;

                if (SplitLine(lp_cur, box_shc)) {
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

            case K_LDEL:
                if (lp_cur > 0) {
                    int prev_len = strlen(lp_arr[lp_cur - 1]);
                    if (JoinLines(lp_cur - 1)) {
                        --lp_cur;
                        box_shc = prev_len;

                        if (box_shr > 0) {
                            --box_shr;
                        }
                        RefreshAll();
                        lp_chg = 1;
                    }
                }
                break;

            case K_RDEL:
                if (lp_cur < lp_now - 1) {
                    if (JoinLines(lp_cur)) {
                        RefreshAll();
                        lp_chg = 1;
                    }
                }
                break;

            case K_CUT:
                if (clipboard) { free(clipboard); clipboard = NULL; }
#if OPT_BLOCK
                if (blk_count) {
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

            case K_PASTE:
                if (clipboard) {
                    if (InsertLine(lp_cur, clipboard)) {
                        if (box_shr < box_rows - 1) ++box_shr;
                        RefreshAll();
                        lp_chg = 1;
                    }
                }
                break;

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

            case K_CLRCLP:
                if (clipboard) { free(clipboard); clipboard = NULL; }
                break;

#if OPT_BLOCK
            case K_BLK_START:
                blk_start = lp_cur;
                if (blk_end < blk_start) blk_end = blk_start;
                blk_count = blk_end - blk_start + 1;
                RefreshAll();
                break;

            case K_BLK_END:
                blk_end = lp_cur;
                if (blk_start < 0) blk_start = blk_end;
                if (blk_end < blk_start) { int t = blk_start; blk_start = blk_end; blk_end = t; }
                blk_count = blk_end - blk_start + 1;
                RefreshAll();
                break;

            case K_BLK_UNSET:
                LoopBlkUnset();
                break;
#endif

#if OPT_FIND
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
            case K_GOTO:
                DoGoto();
                box_shc = 0;
                RefreshAll();
                break;
#endif

#if OPT_MACRO
            case K_MACRO: {
                char fn[FILENAME_MAX_LEN];
                fn[0] = '\0';
                if (SysLineStr("Macro file", fn, FILENAME_MAX_LEN - 1)) {
                    if (!strchr(fn, '.'))
                        strcat(fn, MAC_FTYPE);
                    MacroRunFile(fn, 0);
                }
                break;
            }
#endif

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

        if (lp_cur < 0)         lp_cur = 0;
        if (lp_cur >= lp_now)   lp_cur = lp_now - 1;
        if (box_shc < 0)        box_shc = 0;

        {
            int first = lp_cur - box_shr;
            if (first < 0) {
                box_shr = lp_cur;
            }
            else if (box_shr >= box_rows) {
                box_shr = box_rows - 1;
            }
        }

        if (sysln) {
            sysln = 0;
            SysLineEdit();
        }
    }

    if (clipboard)
        free(clipboard);
}

/* ============================================================================
   MACROS (te_macro.c)
   ============================================================================ */

#if OPT_MACRO

int MacroRunFile(char *fname, int raw)
{
    if (!(mac_fp = fopen(fname, "r"))) {
        ErrLineOpen();
        return -1;
    }

    mac_raw = raw;

    mac_indent = CfGetIndent();
    mac_list   = CfGetList();
    CfSetIndent(0);
    CfSetList(0);

    return 0;
}

int MacroRunning(void)
{
    return mac_fp != NULL;
}

void MacroStop(void)
{
    if (mac_fp) {
        fclose(mac_fp);
        mac_fp = NULL;
    }

    CfSetIndent(mac_indent);
    CfSetList(mac_list);

    ForceCh('\0');
}

int MacroGetRaw(void)
{
    int ch;

    if (mac_fp) {
        if (mac_raw) {
            switch (ch = fgetc(mac_fp)) {
                case '\n': ch = K_CR; break;
                case '\t': ch = ' ';  break;
            }
        }
        else {
            while ((ch = fgetc(mac_fp)) == '\n')
                ;
        }

        if (ch != EOF) {
            if (ch < 32 || ch == 127)
                ch = '?';
            return ch;
        }

        MacroStop();
    }

    return '\0';
}

int MacroIsCmdChar(char ch)
{
    return isalpha((unsigned char)ch) || ch == '#' || ch == '+' || ch == '-';
}

int MatchSym(char *s)
{
    return MatchStr(mac_sym, s);
}

void MacroGet(void)
{
    int  i, n, ch;

    if (!(ch = MacroGetRaw()))
        return;

    if (mac_raw) {
        ForceCh(ch);
        return;
    }

    if (ch != MAC_START) {
        if (ch != MAC_ESCAPE) {
            ForceCh(ch);
        }
        else {
            if ((ch = MacroGetRaw())) {
                ForceCh(ch);
            }
            else {
                ErrLine("Bad escape sequence");
                MacroStop();
            }
        }
        return;
    }

    for (i = 0; MacroIsCmdChar((char)(ch = MacroGetRaw())) && i < MAC_SYM_MAX; ++i)
        mac_sym[i] = (char)tolower(ch);

    if (i) {
        mac_sym[i] = '\0';

        if (ch == MAC_SEP) {
            n = 0;
            while (isdigit(ch = MacroGetRaw()))
                n = n * 10 + ch - '0';
            if (n < 0 || n > FORCED_MAX)
                n = -1;
        }
        else {
            n = 1;
        }

        if (n >= 0) {
            if (ch == ' ' && MatchSym("#")) {
                while ((ch = MacroGetRaw())) {
                    if (ch == MAC_END) {
                        ForceCh('\0');
                        return;
                    }
                }
            }

            if (ch == MAC_END) {
                ch = 0;

                if      (MatchSym("up"))        ch = K_UP;
                else if (MatchSym("down"))      ch = K_DOWN;
                else if (MatchSym("left"))      ch = K_LEFT;
                else if (MatchSym("right"))     ch = K_RIGHT;
                else if (MatchSym("begin"))     ch = K_BEGIN;
                else if (MatchSym("end"))       ch = K_END;
                else if (MatchSym("top"))       ch = K_TOP;
                else if (MatchSym("bottom"))    ch = K_BOTTOM;
                else if (MatchSym("newline"))   ch = K_CR;
                else if (MatchSym("indent"))    ch = K_TAB;
                else if (MatchSym("delright"))  ch = K_RDEL;
                else if (MatchSym("delleft"))   ch = K_LDEL;
                else if (MatchSym("cut"))       ch = K_CUT;
                else if (MatchSym("copy"))      ch = K_COPY;
                else if (MatchSym("paste"))     ch = K_PASTE;
                else if (MatchSym("delete"))    ch = K_DELETE;
                else if (MatchSym("clearclip")) ch = K_CLRCLP;
#if OPT_BLOCK
                else if (MatchSym("blockstart"))ch = K_BLK_START;
                else if (MatchSym("blockend"))  ch = K_BLK_END;
#endif

                if (ch) {
                    while (n--)
                        if (ForceCh(ch)) break;
                    return;
                }

                if (MatchSym("filename")) {
                    while (n--)
                        ForceStr(CurrentFile());
                    return;
                }
                else if (MatchSym("autoindent")) {
                    CfSetIndent(n ? 1 : 0);
                    ForceCh('\0');
                    return;
                }
                else if (MatchSym("autolist")) {
                    CfSetList(n ? 1 : 0);
                    ForceCh('\0');
                    return;
                }
            }
        }
    }

    ErrLine("Bad symbol");
    MacroStop();
}

#endif

/* ============================================================================
   MAIN ENTRY POINT
   ============================================================================ */

int main(int argc, char *argv[])
{
    int rows, cols;

    CrtInit();
    CfLoad();

    CrtGetSize(&rows, &cols);
    if (CfGetRows() == 0 || CfGetRows() > (unsigned char)rows) 
        CfSetRows(CfGetRows() == 0 ? rows : (unsigned char)rows);
    if (CfGetCols() == 0 || CfGetCols() > (unsigned char)cols) 
        CfSetCols(CfGetCols() == 0 ? cols : (unsigned char)cols);

    lp_arr = (char **)calloc(*((int*)&cf_data[10]) + 1, sizeof(char *));
    if (!lp_arr) {
        CrtDone();
        fprintf(stderr, "te-86: not enough memory\n");
        return 1;
    }

    ln_max = CfGetCols() - CfGetNum() - 1;
    if (ln_max < 1) ln_max = 1;

    ln_dat = (char *)malloc(ln_max + 4);
    if (!ln_dat) {
        free(lp_arr);
        CrtDone();
        fprintf(stderr, "te-86: not enough memory\n");
        return 1;
    }

    box_rows = CfGetRows() - BOX_ROW - 2;
    if (box_rows < 1) box_rows = 1;

    Layout();

    if (argc > 1) {
        strncpy(file_name, argv[1], FILENAME_MAX_LEN - 1);
        file_name[FILENAME_MAX_LEN - 1] = '\0';

        if (ReadFile(file_name)) {
            NewFile();
            file_name[0] = '\0';
        }
    }
    else {
        NewFile();
    }

    ShowFilename();

    Loop();

    FreeArray(lp_arr, *((int*)&cf_data[10]), 0);
    free(lp_arr);
    free(ln_dat);

    CrtDone();

    return 0;
}

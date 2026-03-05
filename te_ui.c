/*  te_ui.c

    te-86 Text Editor - User interface.

    Author: Mickey W. Lawless
    Date:   03/01/26

    Original CP/M version: Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    MS-DOS Turbo C port: 2024
*/

#include "te.h"

/* ---------------------------------------------------------------
   putchrx  --  print ch n times
   --------------------------------------------------------------- */
void putchrx(int ch, int n)
{
    while (n--)
        putchr(ch);
}

/* ---------------------------------------------------------------
   putstr  --  print a NUL-terminated string
   --------------------------------------------------------------- */
void putstr(char *s)
{
    while (*s)
        putchr(*s++);
}

/* ---------------------------------------------------------------
   putln  --  print string + newline
   --------------------------------------------------------------- */
void putln(char *s)
{
    putstr(s);
    putchr('\n');
}

/* ---------------------------------------------------------------
   putint  --  format an integer and print it
   --------------------------------------------------------------- */
void putint(char *fmt, int val)
{
    char r[16];
    sprintf(r, fmt, val);
    putstr(r);
}

/* ---------------------------------------------------------------
   Layout  --  draw the static screen chrome
   --------------------------------------------------------------- */
void Layout(void)
{
    int i, k, w;

    CrtClear();

    /* Status bar template */
    CrtLocate(PS_ROW, PS_INF);
    putstr(PS_TXT);

    /* Max lines */
    CrtLocate(PS_ROW, PS_LIN_MAX);
    putint("%04d", cf_mx_lines);

    /* Max columns */
    CrtLocate(PS_ROW, PS_COL_MAX);
    putint("%02d", 1 + ln_max);

#if CRT_LONG
    /* Ruler */
    CrtLocate(BOX_ROW - 1, cf_num);

    w = cf_cols - cf_num;

    for (i = k = 0; i < w; ++i) {
        if (k++) {
            putchr(cf_rul_chr);
            if (k == cf_tab_cols)
                k = 0;
        }
        else {
            putchr(cf_rul_tab);
        }
    }

    /* System line separator */
    CrtLocate(cf_rows - 2, 0);
    putchrx(cf_horz_chr, cf_cols);
#endif
}

/* ---------------------------------------------------------------
   ShowFilename  --  display filename in status bar
   --------------------------------------------------------------- */
void ShowFilename(void)
{
    char *s;
    int maxlen;
    
    CrtLocate(PS_ROW, PS_FNAME);
    s = CurrentFile();
    
    /* Limit filename display to not overwrite status bar */
    maxlen = PS_INF - PS_FNAME - 1;
    if (maxlen < 1) maxlen = 1;
    
    if ((int)strlen(s) > maxlen) {
        /* Truncate long filename */
        while (maxlen-- > 0)
            putchr(*s++);
    }
    else {
        putstr(s);
        putchrx(' ', maxlen - (int)strlen(s));
    }
}

/* ---------------------------------------------------------------
   SysLine  --  display message on the system (bottom) line.
                Pass NULL to clear.
   --------------------------------------------------------------- */
void SysLine(char *s)
{
    CrtClearLine(cf_rows - 1);
    if (s)
        putstr(s);
    sysln = 1;
}

/* ---------------------------------------------------------------
   SysLineEdit  --  show "Esc = menu" hint while editing
   --------------------------------------------------------------- */
void SysLineEdit(void)
{
    SysLine(GetKeyName(K_ESC));
    putstr(" = menu");
}

/* ---------------------------------------------------------------
   SysLineWait  --  show message and wait for CR or ESC.
                    Returns non-zero if CR was pressed.
   --------------------------------------------------------------- */
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

/* ---------------------------------------------------------------
   SysLineCont  --  wait for ESC to continue
   --------------------------------------------------------------- */
void SysLineCont(char *s)
{
    SysLineWait(s, NULL, "continue");
}

/* ---------------------------------------------------------------
   SysLineBack  --  wait for ESC to go back
   --------------------------------------------------------------- */
void SysLineBack(char *s)
{
    SysLineWait(s, NULL, "back");
}

/* ---------------------------------------------------------------
   SysLineConf  --  ask for confirmation; returns NZ if confirmed
   --------------------------------------------------------------- */
int SysLineConf(char *s)
{
    return SysLineWait(s, "continue", "cancel");
}

/* ---------------------------------------------------------------
   SysLineStr  --  ask user to type a string; returns NZ if entered
   --------------------------------------------------------------- */
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

/* ---------------------------------------------------------------
   SysLineFile  --  ask for a filename
   --------------------------------------------------------------- */
int SysLineFile(char *fn)
{
    return SysLineStr("Filename", fn, FILENAME_MAX_LEN - 1);
}

/* ---------------------------------------------------------------
   SysLineChanges  --  warn about unsaved changes
   --------------------------------------------------------------- */
int SysLineChanges(void)
{
    return SysLineConf("Changes will be lost!");
}

/* ---------------------------------------------------------------
   ReadLine  --  simple line input; returns K_CR or K_ESC
   --------------------------------------------------------------- */
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

/* ---------------------------------------------------------------
   CurrentFile  --  return current filename or "-"
   --------------------------------------------------------------- */
char *CurrentFile(void)
{
    return (file_name[0] ? file_name : "-");
}

/* ---------------------------------------------------------------
   ClearBox  --  blank the editor area
   --------------------------------------------------------------- */
void ClearBox(void)
{
    int i;
    for (i = 0; i < box_rows; ++i)
        CrtClearLine(BOX_ROW + i);
}

/* ---------------------------------------------------------------
   CenterText  --  print text centred on a given row
   --------------------------------------------------------------- */
void CenterText(int row, char *txt)
{
    CrtLocate(row, (cf_cols - (int)strlen(txt)) / 2);
    putstr(txt);
}

/* ---------------------------------------------------------------
   RefreshBlock  --  redraw selected block region with highlighting
   --------------------------------------------------------------- */
#if OPT_BLOCK
void RefreshBlock(int row, int sel)
{
    int i, line;

    line = GetFirstLine() + row;

    for (i = row; i < box_rows; ++i) {
        if (line >= blk_start) {
            if (line <= blk_end) {
#if CRT_CAN_REV
                CrtLocate(BOX_ROW + i, cf_num);
                CrtClearEol();
                if (sel) CrtReverse(1);
                putstr(lp_arr[line]);
                putchr(' ');
                if (sel) CrtReverse(0);
#else
                CrtLocate(BOX_ROW + i, cf_cols - 1);
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

/* ---------------------------------------------------------------
   Refresh  --  redraw the editor box from row/line onwards
   --------------------------------------------------------------- */
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

    if (cf_num) {
        strcpy(format, "%?d");
        format[1] = (char)('0' + cf_num - 1);
    }

    for (i = row; i < box_rows; ++i) {
        CrtClearLine(BOX_ROW + i);

        if (line < lp_now) {
            if (cf_num) {
                putint(format, line + 1);
                putchr(cf_lnum_chr);
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
                CrtLocate(BOX_ROW + i, cf_cols - 1);
                putchr('*');
#endif
            }
#endif
        }
    }
}

/* ---------------------------------------------------------------
   RefreshAll  --  full redraw from current scroll position
   --------------------------------------------------------------- */
void RefreshAll(void)
{
    Refresh(0, lp_cur - box_shr);
}

/* ---------------------------------------------------------------
   Menu  --  show the main menu; returns non-zero to quit program
   --------------------------------------------------------------- */
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

/* ---------------------------------------------------------------
   Menu option handlers
   --------------------------------------------------------------- */
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

/* ---------------------------------------------------------------
   MenuHelp  --  show key binding reference
   --------------------------------------------------------------- */
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
            MenuHelpCh(cf_keys[k]);
            MenuHelpCh(cf_keys_ex[k]);
        }
        else {
            putchrx(' ', 15);
        }

        if ((i + 1) % 3) {
            putchr(' ');
            putchr(cf_vert_chr);
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

/* ---------------------------------------------------------------
   MenuConfig  --  configuration menu
   --------------------------------------------------------------- */
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
        putint("%d", cf_tab_cols);
        
        CrtLocate(row++, 2);
        putstr("2. Line num width:  ");
        putint("%d", cf_num);
        
        CrtLocate(row++, 2);
        putstr("3. Auto-indent:     ");
        putstr(cf_indent ? "ON " : "OFF");
        
        CrtLocate(row++, 2);
        putstr("4. Auto-list:       ");
        putstr(cf_list ? "ON " : "OFF");
        
        CrtLocate(row++, 2);
        putstr("5. List bullets:    ");
        putstr(cf_list_chr);
        
        CrtLocate(row++, 2);
        putstr("6. C-lang complete: ");
        putstr(cf_clang ? "ON " : "OFF");
        
        CrtLocate(row++, 2);
        putstr("7. Ruler char:      ");
        putchr(cf_rul_chr);
        
        CrtLocate(row++, 2);
        putstr("8. Ruler tab char:  ");
        putchr(cf_rul_tab);
        
        CrtLocate(row++, 2);
        putstr("9. Line num char:   ");
        putchr(cf_lnum_chr);
        
        CrtLocate(row++, 2);
        putstr("0. Horiz line char: ");
        putchr(cf_horz_chr);
        
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
                CfSetIndent(cf_indent ? 0 : 1);
                break;
                
            case '4':
                CfSetList(cf_list ? 0 : 1);
                break;
                
            case '5':
                SysLine("List bullets (e.g. -*>): ");
                buf[0] = '\0';
                if (ReadLine(buf, 7) == K_CR && buf[0]) {
                    CfSetListChr(buf);
                }
                break;
                
            case '6':
                CfSetClang(cf_clang ? 0 : 1);
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

/* ---------------------------------------------------------------
   MenuAbout  --  show about box
   --------------------------------------------------------------- */
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
    CenterText(row++, cf_name);
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
    CenterText(row++, cf_name);
    CenterText(row++, COPYRIGHT);
    CenterText(row++, "www.floppysoftware.es");
#endif

    SysLineBack(NULL);
}

/* ---------------------------------------------------------------
   MenuExit  --  confirm and exit
   --------------------------------------------------------------- */
int MenuExit(void)
{
    if (lp_chg)
        return !SysLineChanges();
    return 0;
}

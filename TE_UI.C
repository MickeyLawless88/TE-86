/*  te_ui.c

    te-86 Text Editor - User interface.

    TE (CP/M)  - Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    TE-86 (DOS) - Copyright (c) 2024-2026 Mickey W. Lawless

    Licensed under the GNU General Public License v2 or later.

    Changes (TE-86 DOS port):
    -----------------------------------------------------------------
    Features added:
      - FileBrowser(): semi-graphical file/directory picker invoked
        from MenuOpen; scrollable 3-column dialog with reverse-video
        highlight, dirs shown as [NAME], sorted dirs-first then alpha;
        arrow key and mouse navigation; Esc falls back to plain prompt
      - CtxMenu(): small copy/cut/paste popup drawn near the click or
        cursor position; keyboard (arrows/Enter/Esc) and mouse
        (hover to highlight, click to confirm, click outside to cancel)
        navigation; Ctrl+C/X/V shortcuts work inside the menu;
        selection stays highlighted while menu is open; triggered by
        right-click with active block selection (K_MOUSE_CTX) or
        Ctrl+R (K_CTX_MENU)
      - MenuOpen now calls FileBrowser first; ESC from browser falls
        back to the original typed-filename prompt
      - OPT_HILIGHT syntax highlighting (HlPutStr, HlDetect,
        HlIsKeyword, HlLineComment): colour or monochrome attributes
        applied per-token for 8 languages
      - hl_in_comment global replaces local in_comment so block
        comment state persists across lines; reset on file open/new
        and on highlight toggle in MenuConfig
      - HlPutStr initialises cur_attr from hl_in_comment at the top
        of each call so continuation lines of a block comment start
        in comment colour from column 0
      - Hard tab expansion in HlPutStr: ASCII 0x09 is expanded to
        spaces at the correct tab stop using wherex() for column
        tracking; attribute is reset to normal during tab spaces
      - PutStrTabs(): tab-aware putstr for the non-highlighted path
        in Refresh; used when cf_hard_tabs is on
      - Refresh uses PutStrTabs when cf_hard_tabs is on
      - Language label shown right-justified on system line
        (SysLineEdit) when a recognised file type is open
      - MenuConfig options added:
          3  Line numbers show/hide toggle
          8  Syntax highlight on/off toggle
          9  Highlight mono/colour toggle
          E  Hard tabs on/off toggle
          F  FORTRAN continuation column indicator toggle + char select
          G  FORTRAN statement column indicator toggle + char select
          H  COBOL comment/statement indicator toggle + char select
          I  Tab-stop line numbers in ruler toggle
      - Layout() ruler updated: FORTRAN continuation (col 6), FORTRAN
        statement (col 7), COBOL indicator (col 7) overlay characters
        drawn at fixed positions when their options are enabled; tab-stop
        line numbers render tab ordinal as two digits at each stop
      - Refresh() updated to render each line from box_hsc offset so
        horizontally scrolled views display the correct portion of text
      - MenuAbout updated: shows dual credit (Miguel Garcia / CP/M,
        Mickey W. Lawless / DOS) with project URL
      - Layout() ruler start and width use cf_gutter so the ruler
        moves with the line-number visibility toggle
      - Refresh and ClearBox clamp to max_row = cf_rows-2-BOX_ROW
        to prevent writes into the separator and system lines

    Bugs fixed:
      - ESC key in MenuConfig: K_ESC (value 1012) was passed to
        toupper() which truncates to unsigned char first, making
        1012 -> 244 -> not K_ESC; fixed by checking _k == K_ESC
        before calling toupper(), same pattern as Menu()
      - Single-char file extensions (.C, .H) produced garbled type
        tag "[C" with no closing bracket; fixed with a loop that
        copies up to 3 extension chars before appending ']'
      - Operator colour noise: * / - : were painted as operators on
        declarations, negative literals, and struct member access;
        removed from the operator set, keeping only unambiguous
        operators: + = < > ! & | ^ ~ % ?
      - Ruler position did not track line-number toggle: Layout()
        now uses cf_gutter instead of cf_num
      - Multi-line block comments: only the opening line was
        highlighted; fixed by promoting in_comment from a local
        variable to the global hl_in_comment
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
   PutStrTabs  --  print a string, expanding hard tab characters
                   to spaces at the correct tab stops.
                   col is the starting visual column (0-based,
                   relative to the left edge of the text area).
   --------------------------------------------------------------- */
void PutStrTabs(char *s, int col)
{
    int spaces, ts;
    char ch;

    ts = (int)cf_tab_cols;
    if (ts < 1) ts = 8;

    while ((ch = *s++)) {
        if (ch == '\t') {
            spaces = ts - (col % ts);
            while (spaces--) {
                putchr(' ');
                ++col;
            }
        }
        else {
            putchr((unsigned char)ch);
            ++col;
        }
    }
}


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
    int i, k, w, col;

    CrtClear();

    /* Status bar template */
    CrtLocate(PS_ROW, PS_INF);
    putstr(PS_TXT);

    /* Max lines */
    CrtLocate(PS_ROW, PS_LIN_MAX);
    putint("%04d", cf_mx_lines);

    /* Max columns */
    CrtLocate(PS_ROW, PS_COL_MAX);
    putint("%02d", 1 + ln_vis);

#if CRT_LONG
    /* Ruler: starts at the gutter column so it tracks line-number visibility.
       Builds character by character; special column markers overlay the base
       tab-stop pattern when their respective options are enabled.

       FORTRAN fixed-format columns (1-based):
         col  6 = continuation indicator (cf_fort_cont_en / cf_fort_cont_chr)
         col  7 = first statement column (cf_fort_stmt_en / cf_fort_stmt_chr)
       COBOL fixed-format columns (1-based):
         col  7 = indicator area (cf_cobol_ind_en / cf_cobol_ind_chr)
       Tab-stop line numbers (cf_tab_line_num):
         At each tab-stop position print the tab number (1-based) as two
         digits split across the stop char: tens digit replaces the stop,
         units digit is printed one column to the right.  E.g. tab width 10,
         stop at col 0: "1" then "0"; next stop col 10: "2" then "0", etc.
    */
    CrtLocate(BOX_ROW - 1, 0);
    putchrx(' ', cf_gutter);          /* blank the gutter portion of ruler row */

    w = cf_cols - cf_gutter;

    for (i = k = 0; i < w; ++i) {
        /* 1-based column number within the text area (ruler col 1 = col 1) */
        col = i + 1;

        /* Check special column override markers first */
        if (cf_fort_cont_en && col == 6) {
            putchr((unsigned char)cf_fort_cont_chr);
            if (k == cf_tab_cols - 1) k = 0; else ++k;
            continue;
        }
        if (cf_fort_stmt_en && col == 7) {
            putchr((unsigned char)cf_fort_stmt_chr);
            if (k == cf_tab_cols - 1) k = 0; else ++k;
            continue;
        }
        if (cf_cobol_ind_en && col == 7) {
            /* COBOL indicator area; suppress FORTRAN stmt marker if both on */
            putchr((unsigned char)cf_cobol_ind_chr);
            if (k == cf_tab_cols - 1) k = 0; else ++k;
            continue;
        }

        /* Tab-stop position */
        if (k == 0) {
            if (cf_tab_line_num) {
                /* Print tab number as two digits.  Tab number = (i / ts) + 1.
                   We print the tens digit here; units will print next column. */
                int ts  = (int)cf_tab_cols;
                int tn  = (i / (ts > 0 ? ts : 1)) + 1;
                int dig = tn / 10;
                putchr((char)('0' + (dig > 9 ? 9 : dig)));
            } else {
                putchr(cf_rul_tab);
            }
        } else {
            /* Check if the PREVIOUS column was a tab-stop tens digit; if so
               this is the units digit of the tab number. */
            if (cf_tab_line_num && k == 1) {
                int ts  = (int)cf_tab_cols;
                /* i-1 was the tab stop, so tab number for that stop: */
                int tn  = ((i - 1) / (ts > 0 ? ts : 1)) + 1;
                putchr((char)('0' + (tn % 10)));
            } else {
                putchr(cf_rul_chr);
            }
        }

        if (++k == cf_tab_cols) k = 0;
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
   SysLineEdit  --  show "Esc = menu" hint while editing.
                    When a source file is loaded, the language name
                    is shown right-justified on the same row.
   --------------------------------------------------------------- */
void SysLineEdit(void)
{
#if OPT_HILIGHT
    char *lang = NULL;

    switch (hl_lang) {
        case HL_LANG_C:       lang = "C/C++";    break;
        case HL_LANG_ASM:     lang = "ASM";      break;
        case HL_LANG_BASIC:   lang = "BASIC";    break;
        case HL_LANG_PASCAL:  lang = "Pascal";   break;
        case HL_LANG_FORTRAN: lang = "FORTRAN";  break;
        case HL_LANG_PLI:     lang = "PL/I";     break;
        case HL_LANG_COBOL:   lang = "COBOL";    break;
        case HL_LANG_PLM:     lang = "PL/M";     break;
        default:              lang = NULL;        break;
    }
#endif

    SysLine(GetKeyName(K_ESC));
    putstr(" = menu");

#if OPT_HILIGHT
    if (hl_enabled && lang) {
        int llen  = (int)strlen(lang);
        int used  = (int)strlen(GetKeyName(K_ESC)) + 7; /* " = menu" */
        int pad   = cf_cols - used - llen - 1;

        if (pad > 0)
            putchrx(' ', pad);

        putstr(lang);
    }
#endif
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
    int i, max_row;
    max_row = cf_rows - 2 - BOX_ROW;
    if (max_row > box_rows) max_row = box_rows;
    if (max_row < 0) max_row = 0;
    for (i = 0; i < max_row; ++i)
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
                CrtLocate(BOX_ROW + i, cf_gutter);
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
   HlDetect  --  detect language from filename extension.
                 Returns a HL_LANG_* constant (0 = not recognised).
   --------------------------------------------------------------- */
#if OPT_HILIGHT
int HlDetect(char *fname)
{
    char *p, *ext;

    if (!fname || !fname[0])
        return HL_LANG_NONE;

    ext = NULL;
    for (p = fname; *p; ++p)
        if (*p == '.') ext = p;

    if (!ext)
        return HL_LANG_NONE;

    /* C / C++ */
    if (stricmp(ext, ".c")   == 0) return HL_LANG_C;
    if (stricmp(ext, ".h")   == 0) return HL_LANG_C;
    if (stricmp(ext, ".cpp") == 0) return HL_LANG_C;
    if (stricmp(ext, ".hpp") == 0) return HL_LANG_C;
    if (stricmp(ext, ".cc")  == 0) return HL_LANG_C;

    /* Assembly */
    if (stricmp(ext, ".asm") == 0) return HL_LANG_ASM;
    if (stricmp(ext, ".s")   == 0) return HL_LANG_ASM;
    if (stricmp(ext, ".inc") == 0) return HL_LANG_ASM;

    /* BASIC */
    if (stricmp(ext, ".bas") == 0) return HL_LANG_BASIC;
    if (stricmp(ext, ".bas") == 0) return HL_LANG_BASIC;

    /* Pascal */
    if (stricmp(ext, ".pas") == 0) return HL_LANG_PASCAL;
    if (stricmp(ext, ".pp")  == 0) return HL_LANG_PASCAL;

    /* FORTRAN */
    if (stricmp(ext, ".f")   == 0) return HL_LANG_FORTRAN;
    if (stricmp(ext, ".for") == 0) return HL_LANG_FORTRAN;
    if (stricmp(ext, ".f77") == 0) return HL_LANG_FORTRAN;
    if (stricmp(ext, ".f90") == 0) return HL_LANG_FORTRAN;
    if (stricmp(ext, ".ftn") == 0) return HL_LANG_FORTRAN;

    /* PL/I */
    if (stricmp(ext, ".pli") == 0) return HL_LANG_PLI;
    if (stricmp(ext, ".pl1") == 0) return HL_LANG_PLI;
    if (stricmp(ext, ".pli") == 0) return HL_LANG_PLI;

    /* COBOL */
    if (stricmp(ext, ".cob")   == 0) return HL_LANG_COBOL;
    if (stricmp(ext, ".cbl")   == 0) return HL_LANG_COBOL;
    if (stricmp(ext, ".cobol") == 0) return HL_LANG_COBOL;

    /* PL/M */
    if (stricmp(ext, ".plm")   == 0) return HL_LANG_PLM;
    if (stricmp(ext, ".plm86") == 0) return HL_LANG_PLM;
    if (stricmp(ext, ".plm51") == 0) return HL_LANG_PLM;

    return HL_LANG_NONE;
}

/* ---------------------------------------------------------------
   HlIsKeyword  --  return 1 if word (s, len) is a keyword in the
                    current language (hl_lang).
                    Keywords are stored upper-case; the caller must
                    pass an upper-cased copy or use the ucase flag.
   --------------------------------------------------------------- */
static int HlIsKeyword(char *s, int n)
{
    /* --- C / C++ ------------------------------------------------ */
    static char *kw_c[] = {
        "auto",     "break",    "case",     "char",
        "const",    "continue", "default",  "do",
        "double",   "else",     "enum",     "extern",
        "float",    "for",      "goto",     "if",
        "int",      "long",     "register", "return",
        "short",    "signed",   "sizeof",   "static",
        "struct",   "switch",   "typedef",  "union",
        "unsigned", "void",     "volatile", "while",
        /* C++ extras */
        "bool",     "catch",    "class",    "delete",
        "false",    "friend",   "inline",   "namespace",
        "new",      "operator", "private",  "protected",
        "public",   "template", "this",     "throw",
        "true",     "try",      "using",    "virtual",
        /* Turbo C / DOS extras */
        "far",      "near",     "huge",     "interrupt",
        "pascal",   "cdecl",    "asm",
        NULL
    };
    /* --- x86 Assembly ------------------------------------------- */
    static char *kw_asm[] = {
        /* Instructions (common subset) */
        "adc",  "add",  "and",  "call", "cbw",  "clc",  "cld",
        "cli",  "cmc",  "cmp",  "cmps", "cwd",  "daa",  "das",
        "dec",  "div",  "hlt",  "idiv", "imul", "in",   "inc",
        "int",  "into", "iret", "jae",  "jb",   "jbe",  "jc",
        "je",   "jg",   "jge",  "jl",   "jle",  "jmp",  "jna",
        "jnb",  "jnc",  "jne",  "jng",  "jnl",  "jno",  "jnp",
        "jns",  "jnz",  "jo",   "jp",   "jpe",  "jpo",  "js",
        "jz",   "lahf", "lds",  "lea",  "les",  "lods", "loop",
        "loope","loopne","loopnz","loopz","mov", "movs", "mul",
        "neg",  "nop",  "not",  "or",   "out",  "pop",  "popf",
        "push", "pushf","rcl",  "rcr",  "rep",  "repe", "repne",
        "repnz","repz", "ret",  "retf", "retn", "rol",  "ror",
        "sahf", "sal",  "sar",  "sbb",  "scas", "shl",  "shr",
        "stc",  "std",  "sti",  "stos", "sub",  "test", "wait",
        "xchg", "xlat", "xor",
        /* Registers */
        "ax","bx","cx","dx","si","di","bp","sp",
        "al","ah","bl","bh","cl","ch","dl","dh",
        "cs","ds","es","ss","ip","flags",
        /* Directives */
        "db",   "dw",   "dd",   "dq",   "dt",
        "equ",  "org",  "end",  "proc", "endp",
        "macro","endm", "segment","ends","assume",
        "public","extrn","include","if","endif","else","ifdef","ifndef",
        "model","stack","code",  "data", "const","fardata",
        NULL
    };
    /* --- BASIC -------------------------------------------------- */
    static char *kw_bas[] = {
        "AND",    "AS",      "BEEP",    "CALL",    "CASE",
        "CHAIN",  "CIRCLE",  "CLEAR",   "CLOSE",   "CLS",
        "COLOR",  "COMMON",  "CONST",   "DATA",    "DATE",
        "DEF",    "DEFDBL",  "DEFINT",  "DEFLNG",  "DEFSNG",
        "DEFSTR", "DIM",     "DO",      "DRAW",    "ELSE",
        "ELSEIF", "END",     "ENVIRON", "EOF",     "ERASE",
        "ERROR",  "EXIT",    "FIELD",   "FILES",   "FOR",
        "FUNCTION","GET",    "GOSUB",   "GOTO",    "IF",
        "INPUT",  "INSTR",   "INT",     "KEY",     "KILL",
        "LET",    "LINE",    "LOCATE",  "LOOP",    "LSET",
        "MID",    "MKDIR",   "MOD",     "NAME",    "NEXT",
        "NOT",    "ON",      "OPEN",    "OPTION",  "OR",
        "PAINT",  "PEEK",    "PLAY",    "POKE",    "PRESET",
        "PRINT",  "PSET",    "PUT",     "RANDOMIZE","READ",
        "REM",    "RESTORE", "RESUME",  "RETURN",  "RMDIR",
        "RSET",   "RUN",     "SCREEN",  "SELECT",  "SHARED",
        "SLEEP",  "SOUND",   "STATIC",  "STEP",    "STOP",
        "STRING", "SUB",     "SWAP",    "SYSTEM",  "THEN",
        "TIME",   "TIMER",   "TO",      "TYPE",    "UBOUND",
        "UNLOCK", "UNTIL",   "USING",   "VIEW",    "WAIT",
        "WEND",   "WHILE",   "WIDTH",   "WINDOW",  "WRITE",
        "XOR",
        NULL
    };
    /* --- Pascal ------------------------------------------------- */
    static char *kw_pas[] = {
        "AND",       "ARRAY",    "BEGIN",    "CASE",     "CONST",
        "DIV",       "DO",       "DOWNTO",   "ELSE",     "END",
        "FILE",      "FOR",      "FORWARD",  "FUNCTION", "GOTO",
        "IF",        "IN",       "LABEL",    "MOD",      "NIL",
        "NOT",       "OF",       "OR",       "PACKED",   "PROCEDURE",
        "PROGRAM",   "RECORD",   "REPEAT",   "SET",      "SHL",
        "SHR",       "STRING",   "THEN",     "TO",       "TYPE",
        "UNIT",      "UNTIL",    "USES",     "VAR",      "WHILE",
        "WITH",      "XOR",
        /* Turbo Pascal extras */
        "ABSOLUTE",  "ASSEMBLER","CONSTRUCTOR","DESTRUCTOR","EXTERNAL",
        "IMPLEMENTATION","INHERITED","INLINE","INTERFACE","INTERRUPT",
        "NEAR",      "FAR",      "OBJECT",   "OVERRIDE",  "PRIVATE",
        "PUBLIC",    "UNIT",     "USES",     "VIRTUAL",
        NULL
    };
    /* --- FORTRAN 77/90 ----------------------------------------- */
    static char *kw_for[] = {
        "ASSIGN",    "BACKSPACE","BLOCK",    "CALL",     "CHARACTER",
        "CLOSE",     "COMMON",   "COMPLEX",  "CONTINUE", "DATA",
        "DIMENSION", "DO",       "DOUBLE",   "ELSE",     "ELSEIF",
        "END",       "ENDDO",    "ENDIF",    "ENDFILE",  "ENTRY",
        "EQUIVALENCE","EXTERNAL","FORMAT",   "FUNCTION", "GO",
        "GOTO",      "IF",       "IMPLICIT", "INCLUDE",  "INQUIRE",
        "INTEGER",   "INTRINSIC","LOGICAL",  "NAMELIST", "OPEN",
        "PARAMETER", "PAUSE",    "PRECISION","PRINT",    "PROGRAM",
        "READ",      "REAL",     "RETURN",   "REWIND",   "SAVE",
        "STOP",      "SUBROUTINE","THEN",    "TO",       "WRITE",
        /* FORTRAN 90 extras */
        "ALLOCATABLE","ALLOCATE","CASE",     "CONTAINS", "CYCLE",
        "DEALLOCATE","DEFAULT",  "EXIT",     "INTENT",   "MODULE",
        "NULLIFY",   "ONLY",     "OPERATOR", "OPTIONAL", "POINTER",
        "PRIVATE",   "PUBLIC",   "RECURSIVE","SELECT",   "SEQUENCE",
        "TARGET",    "TYPE",     "USE",      "WHERE",
        NULL
    };
    /* --- PL/I --------------------------------------------------- */
    static char *kw_pli[] = {
        "ALLOCATE",  "AREA",     "BEGIN",    "BINARY",   "BIT",
        "BUILTIN",   "BY",       "CALL",     "CHARACTER","CHECK",
        "CLOSE",     "CONDITION","CONTROLLED","DECIMAL",  "DECLARE",
        "DEFAULT",   "DEFINED",  "DELETE",   "DISPLAY",  "DO",
        "ELSE",      "END",      "ENDFILE",  "ENTRY",    "ENVIRONMENT",
        "ERROR",     "EVENT",    "EXTERNAL", "FILE",     "FINISH",
        "FIXED",     "FLOAT",    "FORMAT",   "FREE",     "FROM",
        "GET",       "GO",       "GOTO",     "IF",       "IN",
        "INITIAL",   "INTO",     "KEY",      "KEYED",    "LABEL",
        "LIKE",      "LIST",     "LOCATE",   "ON",       "OPEN",
        "OPTIONS",   "OUTPUT",   "OVERFLOW", "PACKAGE",  "PARAMETER",
        "POINTER",   "PRINT",    "PROCEDURE","PUT",      "READ",
        "RECORD",    "RECURSIVE","REORDER",  "RETURNS",  "REVERT",
        "REWRITE",   "SIGNAL",   "STATIC",   "STOP",     "STREAM",
        "STRING",    "STRUCTURE","SUBSCRIPT","THEN",     "TO",
        "UNALIGNED", "UNDERFLOW","UNION",    "UNTIL",    "UPDATE",
        "VARYING",   "WAIT",     "WHEN",     "WHILE",    "WRITE",
        NULL
    };
    /* --- COBOL -------------------------------------------------- */
    static char *kw_cob[] = {
        "ACCEPT",    "ACCESS",   "ADD",      "ADDRESS",  "ADVANCING",
        "AFTER",     "ALL",      "ALPHABET", "ALPHABETIC","ALTER",
        "AND",       "APPLY",    "ARE",      "AREA",     "AREAS",
        "AT",        "BEFORE",   "BLANK",    "BLOCK",    "BY",
        "CALL",      "CANCEL",   "CD",       "CHARACTER","CLASS",
        "CLOSE",     "COLUMN",   "COMMA",    "COMMON",   "COMPUTE",
        "CONFIGURATION","CONTAINS","COPY",   "CORR",     "CORRESPONDING",
        "COUNT",     "DATA",     "DATE",     "DAY",      "DECLARATIVES",
        "DELETE",    "DELIMITED","DELIMITER","DEPENDING","DESCENDING",
        "DISPLAY",   "DIVIDE",   "DIVISION", "DOWN",     "ELSE",
        "END",       "ENVIRONMENT","EQUAL",  "ERROR",    "EVALUATE",
        "EXIT",      "EXTEND",   "FD",       "FILE",     "FILLER",
        "FIRST",     "FOR",      "FROM",     "FUNCTION", "GENERATE",
        "GIVING",    "GO",       "GREATER",  "GROUP",    "HEADING",
        "HIGH",      "IDENTIFICATION","IF",  "IN",       "INDEX",
        "INITIAL",   "INPUT",    "INSPECT",  "INTO",     "IS",
        "JUST",      "JUSTIFIED","KEY",      "LABEL",    "LEADING",
        "LEFT",      "LESS",     "LIMIT",    "LIMITS",   "LINAGE",
        "LINE",      "LINES",    "LINKAGE",  "LOCK",     "LOW",
        "MEMORY",    "MERGE",    "MODE",     "MOVE",     "MULTIPLY",
        "NATIVE",    "NEXT",     "NOT",      "NUMBER",   "NUMERIC",
        "OBJECT",    "OCCURS",   "OF",       "OFF",      "OMITTED",
        "ON",        "OPEN",     "OR",       "ORDER",    "ORGANIZATION",
        "OTHER",     "OUTPUT",   "OVERFLOW", "PAGE",     "PERFORM",
        "PIC",       "PICTURE",  "PLUS",     "POINTER",  "PROCEDURE",
        "PROGRAM",   "QUOTE",    "RANDOM",   "RD",       "READ",
        "READY",     "RECEIVE",  "RECORD",   "REDEFINES","REEL",
        "RELEASE",   "REMAINDER","REMOVAL",  "REPLACE",  "REPLACING",
        "REPORT",    "RERUN",    "RESERVE",  "RETURN",   "REWIND",
        "REWRITE",   "RIGHT",    "ROUNDED",  "RUN",      "SD",
        "SEARCH",    "SECTION",  "SELECT",   "SEND",     "SENTENCE",
        "SEPARATE",  "SEQUENCE", "SEQUENTIAL","SET",     "SIGN",
        "SIZE",      "SORT",     "SOURCE",   "SPACE",    "SPECIAL",
        "STANDARD",  "START",    "STOP",     "STRING",   "SUBTRACT",
        "SUM",       "SUPPRESS", "SYMBOLIC", "SYNC",     "SYNCHRONIZED",
        "TABLE",     "TALLYING", "TAPE",     "TERMINAL", "TERMINATE",
        "TEST",      "THAN",     "THEN",     "THROUGH",  "THRU",
        "TIME",      "TIMES",    "TO",       "TOP",      "TRAILING",
        "TYPE",      "UNIT",     "UNSTRING", "UNTIL",    "UP",
        "UPON",      "USAGE",    "USE",      "USING",    "VALUE",
        "VARYING",   "WHEN",     "WITH",     "WORKING",  "WRITE",
        "ZERO",
        NULL
    };
    /* --- PL/M --------------------------------------------------- */
    static char *kw_plm[] = {
        "ADDRESS",  "AND",      "AT",       "BASED",    "BY",
        "BYTE",     "CALL",     "CASE",     "DATA",     "DECLARE",
        "DISABLE",  "DO",       "ELSE",     "ENABLE",   "END",
        "EOF",      "EXTERNAL", "GO",       "GOTO",     "HALT",
        "IF",       "INITIAL",  "INTEGER",  "INTERRUPT","LABEL",
        "LITERALLY","MINUS",    "MOD",      "NOT",      "OR",
        "PLUS",     "POINTER",  "PROCEDURE","PUBLIC",   "REAL",
        "REENTRANT","RETURN",   "SELECTOR", "STRUCTURE","THEN",
        "TO",       "WHILE",    "WORD",     "XOR",
        NULL
    };

    char    **table;
    char      buf[32];
    int       i;

    if (n <= 0 || n >= 32)
        return 0;

    switch (hl_lang) {
        case HL_LANG_C:      table = kw_c;   break;
        case HL_LANG_ASM:    table = kw_asm; break;
        case HL_LANG_BASIC:  table = kw_bas; break;
        case HL_LANG_PASCAL: table = kw_pas; break;
        case HL_LANG_FORTRAN:table = kw_for; break;
        case HL_LANG_PLI:    table = kw_pli; break;
        case HL_LANG_COBOL:  table = kw_cob; break;
        case HL_LANG_PLM:    table = kw_plm; break;
        default: return 0;
    }

    /* For case-sensitive languages (C/C++, ASM) compare as-is.
       For all others (BASIC, Pascal, FORTRAN, PL/I, COBOL, PL/M)
       uppercase the token first since those languages are
       case-insensitive and the tables are stored in upper-case.  */
    for (i = 0; i < n; ++i) {
        if (hl_lang == HL_LANG_C || hl_lang == HL_LANG_ASM)
            buf[i] = s[i];
        else
            buf[i] = (char)toupper((unsigned char)s[i]);
    }
    buf[n] = '\0';

    for (i = 0; table[i]; ++i)
        if (strcmp(buf, table[i]) == 0)
            return 1;

    return 0;
}

/* ---------------------------------------------------------------
   HlLineComment  --  return 1 if line s is a whole-line comment
                      for languages that use column-based comment
                      markers (FORTRAN, COBOL).
   --------------------------------------------------------------- */
static int HlLineComment(char *s)
{
    char ch;

    if (hl_lang == HL_LANG_FORTRAN) {
        /* FORTRAN 77: C/c/* in column 1 marks a comment line.
           FORTRAN 90: ! anywhere, but we handle ! as inline below. */
        ch = s[0];
        return (ch == 'C' || ch == 'c' || ch == '*');
    }
    if (hl_lang == HL_LANG_COBOL) {
        /* COBOL: column 7 (0-based index 6) is the indicator area.
           '*' or '/' in col 7 = comment line. */
        int i = 0;
        /* Skip past sequence number area (cols 0-5) */
        while (i < 6 && s[i]) ++i;
        return (s[i] == '*' || s[i] == '/');
    }
    return 0;
}

/* ---------------------------------------------------------------
   HlPutStr  --  print a line with syntax highlighting.
                 Language-specific rules are selected via hl_lang.
                 Colour vs monochrome is selected via cf_hl_mono.
   --------------------------------------------------------------- */
void HlPutStr(char *s)
{
    int   cur_attr;
    int   mono;
    int   A_NORMAL, A_KEYWORD, A_STRING, A_COMMENT;
    int   A_NUMBER, A_PREPROC, A_OPERATOR;
    char  ch, next;
    char *p;

    mono = cf_hl_mono;

    if (mono) {
        A_NORMAL   = HL_MONO_NORMAL;
        A_KEYWORD  = HL_MONO_KEYWORD;
        A_STRING   = HL_MONO_STRING;
        A_COMMENT  = HL_MONO_COMMENT;
        A_NUMBER   = HL_MONO_NUMBER;
        A_PREPROC  = HL_MONO_PREPROC;
        A_OPERATOR = HL_MONO_OPERATOR;
    }
    else {
        A_NORMAL   = HL_NORMAL;
        A_KEYWORD  = HL_KEYWORD;
        A_STRING   = HL_STRING;
        A_COMMENT  = HL_COMMENT;
        A_NUMBER   = HL_NUMBER;
        A_PREPROC  = HL_PREPROC;
        A_OPERATOR = HL_OPERATOR;
    }

    /* If we're inside a block comment from a previous line, start
       this line already in comment colour.                         */
    if (hl_in_comment) {
        cur_attr = A_COMMENT;
        CrtSetAttr(A_COMMENT);
    }
    else {
        cur_attr = A_NORMAL;
        CrtSetAttr(A_NORMAL);
    }

    p = s;

    /* ---- Whole-line comment (FORTRAN col-1 C/*, COBOL col-7 *) ---- */
    if (HlLineComment(s)) {
        CrtSetAttr(A_COMMENT);
        while (*p) CrtOut((unsigned char)*p++);
        CrtSetAttr(HL_NORMAL);
        return;
    }

    while ((ch = *p)) {
        next = *(p + 1);

        /* ---- Continuation inside a block comment ---- */
        if (hl_in_comment) {
            if (cur_attr != A_COMMENT) {
                cur_attr = A_COMMENT;
                CrtSetAttr(A_COMMENT);
            }
            if (ch == '*' && next == '/') {
                CrtOut('*'); CrtOut('/');
                p += 2;
                hl_in_comment = 0;
                cur_attr = A_NORMAL;
                CrtSetAttr(A_NORMAL);
            }
            else if (hl_lang == HL_LANG_PASCAL &&
                     ch == '*' && next == ')') {
                CrtOut('*'); CrtOut(')');
                p += 2;
                hl_in_comment = 0;
                cur_attr = A_NORMAL;
                CrtSetAttr(A_NORMAL);
            }
            else {
                CrtOut((unsigned char)ch);
                ++p;
            }
            continue;
        }

        /* ---- Language-specific line-comment openers ---- */

        /* C/C++: // */
        if ((hl_lang == HL_LANG_C) &&
            ch == '/' && next == '/') {
            CrtSetAttr(A_COMMENT);
            while (*p) CrtOut((unsigned char)*p++);
            break;
        }

        /* C/C++, PL/I, PL/M: block comment /* */
        if ((hl_lang == HL_LANG_C   ||
             hl_lang == HL_LANG_PLI ||
             hl_lang == HL_LANG_PLM) &&
            ch == '/' && next == '*') {
            CrtSetAttr(A_COMMENT);
            hl_in_comment = 1;
            cur_attr   = A_COMMENT;
            CrtOut('/'); CrtOut('*');
            p += 2;
            continue;
        }

        /* Pascal: { comment } */
        if (hl_lang == HL_LANG_PASCAL && ch == '{') {
            CrtSetAttr(A_COMMENT);
            CrtOut('{');
            ++p;
            while (*p && *p != '}') {
                CrtOut((unsigned char)*p++);
            }
            if (*p == '}') { CrtOut('}'); ++p; }
            CrtSetAttr(A_NORMAL);
            cur_attr = A_NORMAL;
            continue;
        }

        /* Pascal: (* comment *) */
        if (hl_lang == HL_LANG_PASCAL &&
            ch == '(' && next == '*') {
            CrtSetAttr(A_COMMENT);
            hl_in_comment = 1;
            cur_attr   = A_COMMENT;
            CrtOut('('); CrtOut('*');
            p += 2;
            continue;
        }

        /* BASIC: REM keyword handled via keyword table;
                  ' (apostrophe) starts a line comment */
        if (hl_lang == HL_LANG_BASIC && ch == '\'') {
            CrtSetAttr(A_COMMENT);
            while (*p) CrtOut((unsigned char)*p++);
            break;
        }

        /* FORTRAN 90 / ASM: ! starts a line comment */
        if ((hl_lang == HL_LANG_FORTRAN ||
             hl_lang == HL_LANG_ASM) && ch == '!') {
            CrtSetAttr(A_COMMENT);
            while (*p) CrtOut((unsigned char)*p++);
            break;
        }

        /* ASM: ; starts a line comment */
        if (hl_lang == HL_LANG_ASM && ch == ';') {
            CrtSetAttr(A_COMMENT);
            while (*p) CrtOut((unsigned char)*p++);
            break;
        }

        /* ---- C preprocessor # directive ---- */
        if (hl_lang == HL_LANG_C && ch == '#') {
            CrtSetAttr(A_PREPROC);
            while (*p) CrtOut((unsigned char)*p++);
            break;
        }

        /* ---- String/character literals ---- */

        /* Double-quoted string: C, C++, FORTRAN, BASIC, COBOL */
        if (ch == '"' &&
            (hl_lang == HL_LANG_C       ||
             hl_lang == HL_LANG_FORTRAN ||
             hl_lang == HL_LANG_BASIC   ||
             hl_lang == HL_LANG_COBOL)) {
            CrtSetAttr(A_STRING);
            CrtOut('"');
            ++p;
            while (*p && *p != '"') {
                if ((hl_lang == HL_LANG_C) && *p == '\\' && *(p+1))
                    CrtOut((unsigned char)*p++);
                CrtOut((unsigned char)*p++);
            }
            if (*p == '"') { CrtOut('"'); ++p; }
            CrtSetAttr(A_NORMAL);
            cur_attr = A_NORMAL;
            continue;
        }

        /* Single-quoted string/char: all except C char literals
           use '' for Pascal/FORTRAN/PL/I/COBOL/PL/M escape */
        if (ch == '\'') {
            CrtSetAttr(A_STRING);
            CrtOut('\'');
            ++p;
            if (hl_lang == HL_LANG_C) {
                /* C: backslash escape, closed by single ' */
                while (*p && *p != '\'') {
                    if (*p == '\\' && *(p+1))
                        CrtOut((unsigned char)*p++);
                    CrtOut((unsigned char)*p++);
                }
            }
            else {
                /* Other langs: '' is an escaped quote inside string */
                while (*p) {
                    if (*p == '\'' && *(p+1) == '\'') {
                        CrtOut('\''); CrtOut('\'');
                        p += 2;
                    }
                    else if (*p == '\'') {
                        break;
                    }
                    else {
                        CrtOut((unsigned char)*p++);
                    }
                }
            }
            if (*p == '\'') { CrtOut('\''); ++p; }
            CrtSetAttr(A_NORMAL);
            cur_attr = A_NORMAL;
            continue;
        }

        /* ---- Number literal ---- */
        if (isdigit((unsigned char)ch) ||
            (ch == '0' && (next == 'x' || next == 'X')))
        {
            CrtSetAttr(A_NUMBER);
            while (*p && (isxdigit((unsigned char)*p) ||
                           *p == 'x' || *p == 'X' ||
                           *p == 'u' || *p == 'U' ||
                           *p == 'l' || *p == 'L' ||
                           *p == '.' || *p == 'e' || *p == 'E' ||
                           *p == '+' || *p == '-'))
            {
                CrtOut((unsigned char)*p++);
            }
            CrtSetAttr(A_NORMAL);
            cur_attr = A_NORMAL;
            continue;
        }

        /* ---- Identifier / keyword ---- */
        if (isalpha((unsigned char)ch) || ch == '_' ||
            (hl_lang == HL_LANG_PLM && ch == '$'))
        {
            char *start = p;
            int   len   = 0;

            while (*p && (isalnum((unsigned char)*p) ||
                           *p == '_' ||
                           (hl_lang == HL_LANG_PLM && *p == '$'))) {
                ++p; ++len;
            }

            if (HlIsKeyword(start, len)) {
                CrtSetAttr(A_KEYWORD);
                while (start < p) CrtOut((unsigned char)*start++);
                CrtSetAttr(A_NORMAL);
                cur_attr = A_NORMAL;
            }
            else {
                if (cur_attr != A_NORMAL) {
                    CrtSetAttr(A_NORMAL);
                    cur_attr = A_NORMAL;
                }
                while (start < p) CrtOut((unsigned char)*start++);
            }
            continue;
        }

        /* ---- Hard tab expansion ---- */
        if (ch == '\t') {
            int col, spaces, ts;
            ts = (int)cf_tab_cols;
            if (ts < 1) ts = 8;
            col = wherex() - 1 - cf_gutter;   /* visual col within text area */
            if (col < 0) col = 0;
            spaces = ts - (col % ts);
            if (cur_attr != A_NORMAL) {
                cur_attr = A_NORMAL;
                CrtSetAttr(A_NORMAL);
            }
            while (spaces--) CrtOut(' ');
            ++p;
            continue;
        }

        /* ---- Operators ---- */
        if (ch == '+' || ch == '=' ||
            ch == '<' || ch == '>' || ch == '!' ||
            ch == '&' || ch == '|' || ch == '^' || ch == '~' ||
            ch == '%' || ch == '?' )
        {
            if (cur_attr != A_OPERATOR) {
                cur_attr = A_OPERATOR;
                CrtSetAttr(A_OPERATOR);
            }
        }
        else {
            if (cur_attr != A_NORMAL) {
                cur_attr = A_NORMAL;
                CrtSetAttr(A_NORMAL);
            }
        }

        CrtOut((unsigned char)ch);
        ++p;
    }

    CrtSetAttr(HL_NORMAL);
}
#endif /* OPT_HILIGHT */


void Refresh(int row, int line)
{
    int i;
    int max_row;   /* hard upper bound - never write into separator */
    char format[8];
    char *p;       /* pointer into line at horizontal scroll position */

#if OPT_BLOCK
    int blk, sel;
    blk = (blk_count &&
           blk_start <= GetLastLine() &&
           blk_end   >= GetFirstLine());
    sel = 0;
#endif

    /* Clamp box_rows against the actual screen height so that even if
       box_rows was miscalculated we can never overwrite the separator
       line (cf_rows - 2) or the system line (cf_rows - 1).            */
    max_row = cf_rows - 2 - BOX_ROW;   /* last safe editor row index  */
    if (max_row > box_rows) max_row = box_rows;
    if (max_row < 0) max_row = 0;

    if (cf_num) {
        strcpy(format, "%?d");
        format[1] = (char)('0' + cf_num - 1);
    }

    for (i = row; i < max_row; ++i) {
        CrtClearLine(BOX_ROW + i);

        if (line < lp_now) {
            if (cf_num && cf_lnum_show) {
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

            /* Advance to horizontal scroll offset */
            p = lp_arr[line];
            {
                int hsc = box_hsc;
                while (hsc > 0 && *p) { ++p; --hsc; }
            }

            /* Print the line from horizontal scroll position */
#if OPT_HILIGHT
            if (hl_enabled && !sel)
                HlPutStr(p);
            else
#endif
            if (cf_hard_tabs)
                PutStrTabs(p, box_hsc);
            else
                putstr(p);

            ++line;

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

        {
            int _k = getchr();
            /* Handle function/extended keys before toupper() truncates them */
            if (_k == K_ESC) {
                run = 0;
            }
            else {
                switch (toupper(_k)) {
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
                    default:     ask = 0; break;
                }
            }
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
#if OPT_HILIGHT
    hl_lang       = HL_LANG_NONE;
    hl_enabled    = 0;
    hl_in_comment = 0;
#endif
    return 0;
}

/* ---------------------------------------------------------------
   FileBrowser  --  3-column centered dialog file/directory picker.

   Dialog: 47 wide x 15 tall, centered on screen.
   Shows 3 columns of entries per row, 10 list rows = 30 visible
   entries.  Dirs shown as [NAME], selected entry in reverse video.
   Scrolling redraws the full dialog cleanly.  ESC falls back to
   the typed-filename prompt.

   Returns 1 and fills dest[] on selection, 0 on ESC.
   --------------------------------------------------------------- */

/* --- Dialog geometry --- */
#define FB_NCOLS      3     /* columns of entries                        */
#define FB_COL_W     15     /* chars per column: 1sp + 13name + 1sp      */
#define FB_LIST_ROWS 10     /* visible list rows                         */
/* Inner width = 3 cols + 2 internal column separators                  */
#define FB_INNER_W   (FB_NCOLS * FB_COL_W + (FB_NCOLS - 1))  /* 47     */
#define FB_DLG_W     (FB_INNER_W + 2)   /* 49 (inner + left|  + right|) */
/* Rows: top|path|div|list(10)|div|hint|bottom = 16                     */
#define FB_DLG_H     16
#define FB_VIS       (FB_LIST_ROWS * FB_NCOLS)  /* 30 visible            */
#define FB_MAX_ENTRIES 512

typedef struct {
    char name[13];   /* 8.3 name, NUL terminated */
    int  is_dir;
} FbEntry;

static FbEntry fb_entries[FB_MAX_ENTRIES];
static int     fb_count;

static int fb_cmp(const void *a, const void *b)
{
    const FbEntry *ea = (const FbEntry *)a;
    const FbEntry *eb = (const FbEntry *)b;
    if (ea->is_dir != eb->is_dir) return eb->is_dir - ea->is_dir;
    return stricmp(ea->name, eb->name);
}

static void fb_load(char *path)
{
    struct ffblk ff;
    char pattern[90];
    int done, at_root, skip;

    fb_count = 0;
    at_root = (strlen(path) == 3 && path[1] == ':' && path[2] == '\\');

    if (!at_root) {
        strcpy(fb_entries[0].name, "..");
        fb_entries[0].is_dir = 1;
        fb_count = 1;
    }

    strcpy(pattern, path);
    if (pattern[strlen(pattern)-1] != '\\') strcat(pattern, "\\");
    strcat(pattern, "*.*");

    done = findfirst(pattern, &ff, FA_DIREC);
    while (!done && fb_count < FB_MAX_ENTRIES) {
        if ((ff.ff_attrib & FA_DIREC) && ff.ff_name[0] != '.') {
            strncpy(fb_entries[fb_count].name, ff.ff_name, 12);
            fb_entries[fb_count].name[12] = '\0';
            fb_entries[fb_count].is_dir = 1;
            fb_count++;
        }
        done = findnext(&ff);
    }

    done = findfirst(pattern, &ff, 0);
    while (!done && fb_count < FB_MAX_ENTRIES) {
        strncpy(fb_entries[fb_count].name, ff.ff_name, 12);
        fb_entries[fb_count].name[12] = '\0';
        fb_entries[fb_count].is_dir = 0;
        fb_count++;
        done = findnext(&ff);
    }

    /* Sort, keeping ".." pinned at slot 0 */
    skip = (fb_count > 0 && strcmp(fb_entries[0].name, "..") == 0) ? 1 : 0;
    if (fb_count - skip > 1)
        qsort(fb_entries + skip, fb_count - skip, sizeof(FbEntry), fb_cmp);
}

/* Draw one 15-char entry cell at the current cursor position.
   name must already be formatted (with [] for dirs).              */
static void fb_cell(char *name, int selected)
{
    int nlen = strlen(name);
    int pad;
    if (nlen > FB_COL_W - 2) nlen = FB_COL_W - 2;
    pad = FB_COL_W - 1 - nlen;   /* always >= 1 */
    if (pad < 1) pad = 1;
    if (selected) CrtReverse(1);
    putchr(' ');
    {   /* write exactly nlen chars */
        int i;
        for (i = 0; i < nlen; i++) putchr((unsigned char)name[i]);
    }
    putchrx(' ', pad);
    if (selected) CrtReverse(0);
}

static void fb_draw(char *path, int dr, int dc, int top_row, int sel)
{
    int r, c;
    char buf[16];

    /* Row offsets:
       dr+0  top border
       dr+1  path row
       dr+2  column divider
       dr+3..dr+12  10 list rows
       dr+13 bottom divider
       dr+14 hint row
       dr+15 bottom border                                        */

    /* ---- Top border ---- */
    CrtLocate(dr, dc);
    CrtSetAttr(0x07);
    putchr('+');
    putchrx('-', FB_INNER_W);
    putchr('+');

    /* ---- Path row ---- */
    CrtLocate(dr + 1, dc);
    putchr('|');
    {
        int plen = strlen(path);
        if (plen <= FB_INNER_W) {
            putstr(path);
            putchrx(' ', FB_INNER_W - plen);
        } else {
            putstr("...");
            putstr(path + plen - (FB_INNER_W - 3));
        }
    }
    putchr('|');

    /* ---- Column header divider: +---+---+---+ ---- */
    CrtLocate(dr + 2, dc);
    putchr('+');
    for (c = 0; c < FB_NCOLS; c++) {
        putchrx('-', FB_COL_W);
        putchr(c < FB_NCOLS - 1 ? '+' : '+');
    }

    /* ---- List rows ---- */
    for (r = 0; r < FB_LIST_ROWS; r++) {
        CrtLocate(dr + 3 + r, dc);
        CrtSetAttr(0x07);
        putchr('|');
        for (c = 0; c < FB_NCOLS; c++) {
            int idx = top_row * FB_NCOLS + r * FB_NCOLS + c;
            if (idx < fb_count) {
                if (fb_entries[idx].is_dir)
                    sprintf(buf, "[%s]", fb_entries[idx].name);
                else
                    strcpy(buf, fb_entries[idx].name);
                fb_cell(buf, idx == sel);
            } else {
                putchrx(' ', FB_COL_W);
            }
            CrtSetAttr(0x07);
            putchr(c < FB_NCOLS - 1 ? '|' : '|');
        }
    }

    /* ---- Bottom divider: +---+---+---+ ---- */
    CrtLocate(dr + 13, dc);
    CrtSetAttr(0x07);
    putchr('+');
    for (c = 0; c < FB_NCOLS; c++) {
        putchrx('-', FB_COL_W);
        putchr(c < FB_NCOLS - 1 ? '+' : '+');
    }

    /* ---- Hint / counter row ---- */
    CrtLocate(dr + 14, dc);
    putchr('|');
    {
        char info[FB_INNER_W + 2];
        int  ilen, pad;
        int  page_first = top_row * FB_NCOLS + 1;
        int  page_last  = page_first + FB_VIS - 1;
        if (page_last > fb_count) page_last = fb_count;
        if (fb_count > 0)
            sprintf(info, " %d-%d of %d  Arrows Enter Esc",
                    page_first, page_last, fb_count);
        else
            strcpy(info, " (empty directory)");
        ilen = strlen(info);
        if (ilen > FB_INNER_W) ilen = FB_INNER_W;
        info[ilen] = '\0';
        pad = FB_INNER_W - ilen;
        putstr(info);
        putchrx(' ', pad);
    }
    putchr('|');

    /* ---- Bottom border ---- */
    CrtLocate(dr + 15, dc);
    putchr('+');
    putchrx('-', FB_INNER_W);
    putchr('+');
}

int FileBrowser(char *dest)
{
    char cwd[80];
    int  sel, top_row, done, result, ch;
    int  dr, dc;   /* dialog top-left */

    dr = (cf_rows - FB_DLG_H) / 2;
    dc = (cf_cols - FB_DLG_W) / 2;
    if (dr < 0) dr = 0;
    if (dc < 0) dc = 0;

    if (getcwd(cwd, sizeof(cwd)) == NULL) strcpy(cwd, "C:\\");
    { char *p = cwd; while (*p) { *p = toupper(*p); p++; } }

    sel = top_row = done = result = 0;
    fb_load(cwd);
    MouseHide();
    fb_draw(cwd, dr, dc, top_row, sel);

    while (!done) {
        ch = getchr();

        switch (ch) {
            case K_ESC:
                done = 1; result = 0;
                break;

            case K_UP:
                if (sel >= FB_NCOLS) {
                    sel -= FB_NCOLS;
                    /* scroll up if needed */
                    if (sel < top_row * FB_NCOLS)
                        top_row--;
                    if (top_row < 0) top_row = 0;
                    fb_draw(cwd, dr, dc, top_row, sel);
                }
                break;

            case K_DOWN:
                if (sel + FB_NCOLS < fb_count) {
                    sel += FB_NCOLS;
                    /* scroll down if needed */
                    while (sel >= (top_row + FB_LIST_ROWS) * FB_NCOLS)
                        top_row++;
                    fb_draw(cwd, dr, dc, top_row, sel);
                }
                break;

            case K_LEFT:
                /* left within row, or go up to parent */
                if (sel % FB_NCOLS > 0) {
                    sel--;
                    fb_draw(cwd, dr, dc, top_row, sel);
                } else if (fb_count > 0 &&
                           strcmp(fb_entries[0].name, "..") == 0) {
                    sel = 0; top_row = 0;
                    ch = K_CR;   /* treat as Enter on ".." */
                    goto do_enter;
                }
                break;

            case K_RIGHT:
                if (sel % FB_NCOLS < FB_NCOLS - 1 && sel + 1 < fb_count) {
                    sel++;
                    fb_draw(cwd, dr, dc, top_row, sel);
                } else if (sel < fb_count && fb_entries[sel].is_dir) {
                    goto do_enter;
                }
                break;

            case K_PGUP:
                if (top_row > 0) {
                    top_row -= FB_LIST_ROWS;
                    if (top_row < 0) top_row = 0;
                    sel = top_row * FB_NCOLS;
                    fb_draw(cwd, dr, dc, top_row, sel);
                }
                break;

            case K_PGDOWN:
            {
                int max_row = (fb_count - 1) / FB_NCOLS - FB_LIST_ROWS + 1;
                if (max_row < 0) max_row = 0;
                if (top_row < max_row) {
                    top_row += FB_LIST_ROWS;
                    if (top_row > max_row) top_row = max_row;
                    sel = top_row * FB_NCOLS;
                    if (sel >= fb_count) sel = fb_count - 1;
                    fb_draw(cwd, dr, dc, top_row, sel);
                }
                break;
            }

            case K_BEGIN:
                sel = top_row = 0;
                fb_draw(cwd, dr, dc, top_row, sel);
                break;

            case K_END:
                sel = fb_count - 1;
                if (sel < 0) sel = 0;
                top_row = sel / FB_NCOLS - FB_LIST_ROWS + 1;
                if (top_row < 0) top_row = 0;
                fb_draw(cwd, dr, dc, top_row, sel);
                break;

            case K_CR:
            do_enter:
                if (sel >= 0 && sel < fb_count) {
                    if (fb_entries[sel].is_dir) {
                        char newpath[80];
                        if (strcmp(fb_entries[sel].name, "..") == 0) {
                            int len = strlen(cwd);
                            while (len > 0 && cwd[len-1] != '\\') len--;
                            if (len > 0) cwd[len-1] = '\0';
                            if (strlen(cwd) == 2 && cwd[1] == ':') {
                                cwd[2] = '\\'; cwd[3] = '\0';
                            }
                        } else {
                            strcpy(newpath, cwd);
                            if (newpath[strlen(newpath)-1] != '\\')
                                strcat(newpath, "\\");
                            strcat(newpath, fb_entries[sel].name);
                            strcpy(cwd, newpath);
                        }
                        sel = top_row = 0;
                        fb_load(cwd);
                        fb_draw(cwd, dr, dc, top_row, sel);
                    } else {
                        strcpy(dest, cwd);
                        if (dest[strlen(dest)-1] != '\\')
                            strcat(dest, "\\");
                        strcat(dest, fb_entries[sel].name);
                        done = 1; result = 1;
                    }
                }
                break;
        }
    }

    MouseShow();
    RefreshAll();
    return result;
}

int MenuOpen(void)
{
    char fn[FILENAME_MAX_LEN];

    if (lp_chg && !SysLineChanges())
        return 1;

    fn[0] = '\0';

    /* Show the graphical file browser first.  If the user pressed ESC,
       fall back to the plain filename prompt so they can still type a
       path directly if they know it.                                    */
    if (!FileBrowser(fn)) {
        if (!SysLineFile(fn))
            return 1;
    }

    if (fn[0]) {
        if (ReadFile(fn))
            NewFile();
        else
            strcpy(file_name, fn);
#if OPT_HILIGHT
        hl_lang       = HlDetect(file_name);
        hl_enabled    = (hl_lang != HL_LANG_NONE) && cf_hl_enable;
        hl_in_comment = 0;
#endif
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
        putstr("3. Line numbers:    ");
        putstr(cf_lnum_show ? "ON " : "OFF");

        CrtLocate(row++, 2);
        putstr("4. Auto-indent:     ");
        putstr(cf_indent ? "ON " : "OFF");

        CrtLocate(row++, 2);
        putstr("5. Auto-list:       ");
        putstr(cf_list ? "ON " : "OFF");

        CrtLocate(row++, 2);
        putstr("6. List bullets:    ");
        putstr(cf_list_chr);

        CrtLocate(row++, 2);
        putstr("7. C-lang complete: ");
        putstr(cf_clang ? "ON " : "OFF");

#if OPT_HILIGHT
        CrtLocate(row++, 2);
        putstr("8. Syntax hilite:   ");
        putstr(cf_hl_enable ? "ON " : "OFF");

        CrtLocate(row++, 2);
        putstr("9. Hilite mono:     ");
        putstr(cf_hl_mono ? "ON " : "OFF");
#endif

        CrtLocate(row++, 2);
        putstr("A. Ruler char:      ");
        putchr(cf_rul_chr);

        CrtLocate(row++, 2);
        putstr("B. Ruler tab char:  ");
        putchr(cf_rul_tab);

        CrtLocate(row++, 2);
        putstr("C. Line num char:   ");
        putchr(cf_lnum_chr);

        CrtLocate(row++, 2);
        putstr("D. Horiz line char: ");
        putchr(cf_horz_chr);

        CrtLocate(row++, 2);
        putstr("E. Hard tabs:       ");
        putstr(cf_hard_tabs ? "ON " : "OFF");

        CrtLocate(row++, 2);
        putstr("F. FORT cont col:   ");
        putstr(cf_fort_cont_en ? "ON " : "OFF");
        putstr("  char: '");
        putchr((unsigned char)cf_fort_cont_chr);
        putchr('\'');

        CrtLocate(row++, 2);
        putstr("G. FORT stmt col:   ");
        putstr(cf_fort_stmt_en ? "ON " : "OFF");
        putstr("  char: '");
        putchr((unsigned char)cf_fort_stmt_chr);
        putchr('\'');

        CrtLocate(row++, 2);
        putstr("H. COBOL ind col:   ");
        putstr(cf_cobol_ind_en ? "ON " : "OFF");
        putstr("  char: '");
        putchr((unsigned char)cf_cobol_ind_chr);
        putchr('\'');

        CrtLocate(row++, 2);
        putstr("I. Tab line nums:   ");
        putstr(cf_tab_line_num ? "ON " : "OFF");

        row++;
        CrtLocate(row++, 2);
        putstr("S. Save and exit config");

        SysLine("Option (Esc = cancel): ");

        {
            int _k = getchr();
            /* Check K_ESC before toupper() - Turbo C's toupper macro truncates
               its argument to unsigned char first, so K_ESC (1012) becomes
               0xF4 (244) which may be treated as lowercase and shifted,
               producing 980 instead of 1012 and never matching K_ESC.         */
            if (_k == K_ESC) {
                SysLine(NULL);
                return;
            }
            ch = toupper(_k);
        }

        switch (ch) {
            case '1':
                SysLine("Tab width (1-8): ");
                buf[0] = '\0';
                if (ReadLine(buf, 2) == K_CR && buf[0]) {
                    val = atoi(buf);
                    if (val >= 1 && val <= 8)
                        CfSetTabCols((unsigned char)val);
                }
                break;

            case '2':
                SysLine("Line number width (0-6): ");
                buf[0] = '\0';
                if (ReadLine(buf, 2) == K_CR && buf[0]) {
                    val = atoi(buf);
                    if (val >= 0 && val <= 6)
                        CfSetNum((unsigned char)val);
                }
                break;

            case '3':
                CfSetLnumShow(cf_lnum_show ? 0 : 1);
                break;

            case '4':
                CfSetIndent(cf_indent ? 0 : 1);
                break;

            case '5':
                CfSetList(cf_list ? 0 : 1);
                break;

            case '6':
                SysLine("List bullets (e.g. -*>): ");
                buf[0] = '\0';
                if (ReadLine(buf, 7) == K_CR && buf[0])
                    CfSetListChr(buf);
                break;

            case '7':
                CfSetClang(cf_clang ? 0 : 1);
                break;

#if OPT_HILIGHT
            case '8':
                CfSetHlEnable(cf_hl_enable ? 0 : 1);
                if (cf_hl_enable) {
                    hl_lang       = HlDetect(file_name);
                    hl_enabled    = (hl_lang != HL_LANG_NONE);
                    hl_in_comment = 0;
                }
                else {
                    hl_enabled    = 0;
                    hl_in_comment = 0;
                }
                break;

            case '9':
                CfSetHlMono(cf_hl_mono ? 0 : 1);
                break;
#endif

            case 'A':
                SysLine("Ruler char: ");
                ch = getchr();
                if (ch > 31 && ch < 127)
                    CfSetRulChr((unsigned char)ch);
                break;

            case 'B':
                SysLine("Ruler tab char: ");
                ch = getchr();
                if (ch > 31 && ch < 127)
                    CfSetRulTab((unsigned char)ch);
                break;

            case 'C':
                SysLine("Line number separator char: ");
                ch = getchr();
                if (ch > 31 && ch < 127)
                    CfSetLnumChr((unsigned char)ch);
                break;

            case 'D':
                SysLine("Horizontal line char: ");
                ch = getchr();
                if (ch > 31 && ch < 127)
                    CfSetHorzChr((unsigned char)ch);
                break;

            case 'E':
                CfSetHardTabs(cf_hard_tabs ? 0 : 1);
                break;

            case 'F':
                /* Toggle FORTRAN continuation col, then optionally set char */
                CfSetFortContEn(cf_fort_cont_en ? 0 : 1);
                if (cf_fort_cont_en) {
                    SysLine("FORTRAN cont col char (Enter to keep current): ");
                    ch = getchr();
                    if (ch > 31 && ch < 127)
                        CfSetFortContChr((unsigned char)ch);
                }
                Layout();
                break;

            case 'G':
                /* Toggle FORTRAN statement col, then optionally set char */
                CfSetFortStmtEn(cf_fort_stmt_en ? 0 : 1);
                if (cf_fort_stmt_en) {
                    SysLine("FORTRAN stmt col char (Enter to keep current): ");
                    ch = getchr();
                    if (ch > 31 && ch < 127)
                        CfSetFortStmtChr((unsigned char)ch);
                }
                Layout();
                break;

            case 'H':
                /* Toggle COBOL indicator col, then optionally set char */
                CfSetCobolIndEn(cf_cobol_ind_en ? 0 : 1);
                if (cf_cobol_ind_en) {
                    SysLine("COBOL indicator col char (Enter to keep current): ");
                    ch = getchr();
                    if (ch > 31 && ch < 127)
                        CfSetCobolIndChr((unsigned char)ch);
                }
                Layout();
                break;

            case 'I':
                CfSetTabLineNum(cf_tab_line_num ? 0 : 1);
                Layout();
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
    CenterText(row++, "TE (CP/M) - Miguel Garcia / FloppySoftware");
    CenterText(row++, "TE-86 (DOS) - Mickey W. Lawless");
    row++;
    CenterText(row++, "github.com/mickeylawless88/te-86");
#else
    row = BOX_ROW;
    ClearBox();
    CenterText(row++, "te-86 Text Editor");
    CenterText(row++, VERSION);
    CenterText(row++, "TE (CP/M) Miguel Garcia");
    CenterText(row++, "TE-86 (DOS) Mickey W. Lawless");
    CenterText(row++, "github.com/mickeylawless88/te-86");
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

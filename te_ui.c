/*  te_ui.c

    te-86 Text Editor - User interface.

    Author: Mickey W. Lawless
    Date:   03/01/26

    Original CP/M version: Copyright (c) 2015-2021 Mickey W. Lawless
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
    putint("%02d", 1 + ln_vis);

#if CRT_LONG
    /* Ruler: starts at the gutter column so it tracks line-number visibility */
    CrtLocate(BOX_ROW - 1, 0);
    putchrx(' ', cf_gutter);          /* blank the gutter portion of ruler row */

    w = cf_cols - cf_gutter;

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
    int   in_comment = 0;
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

    cur_attr = A_NORMAL;
    CrtSetAttr(A_NORMAL);

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
        if (in_comment) {
            if (cur_attr != A_COMMENT) {
                cur_attr = A_COMMENT;
                CrtSetAttr(A_COMMENT);
            }
            if (ch == '*' && next == '/') {
                CrtOut('*'); CrtOut('/');
                p += 2;
                in_comment = 0;
                cur_attr = A_NORMAL;
                CrtSetAttr(A_NORMAL);
            }
            else if (hl_lang == HL_LANG_PASCAL &&
                     ch == '*' && next == ')') {
                CrtOut('*'); CrtOut(')');
                p += 2;
                in_comment = 0;
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
            in_comment = 1;
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
            in_comment = 1;
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

            /* Print the line - with syntax highlighting if enabled */
#if OPT_HILIGHT
            if (hl_enabled && !sel)
                HlPutStr(lp_arr[line++]);
            else
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
    hl_lang    = HL_LANG_NONE;
    hl_enabled = 0;
#endif
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
#if OPT_HILIGHT
        hl_lang    = HlDetect(file_name);
        hl_enabled = (hl_lang != HL_LANG_NONE) && cf_hl_enable;
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
                    hl_lang    = HlDetect(file_name);
                    hl_enabled = (hl_lang != HL_LANG_NONE);
                }
                else {
                    hl_enabled = 0;
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
    CenterText(row++, "Configured for");
    CenterText(row++, cf_name);
    row++;
    CenterText(row++, COPYRIGHT);
    row++;
    CenterText(row++, "github.com/mickeylawless88");
    CenterText(row,   "github.com/mickeylawless88/te-86");
#else
    row = BOX_ROW;
    ClearBox();
    CenterText(row++, "te-86 Text Editor");
    CenterText(row++, VERSION);
    CenterText(row++, "Configured for");
    CenterText(row++, cf_name);
    CenterText(row++, COPYRIGHT);
    CenterText(row++, "github.com/mickeylawless88");
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

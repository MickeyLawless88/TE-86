/*  te.h

    te-86 Text Editor - Definitions and prototypes.

    TE (CP/M)  - Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    TE-86 (DOS) - Copyright (c) 2024-2026 Mickey W. Lawless

    Licensed under the GNU General Public License v2 or later.

    Changes (TE-86 DOS port):
    -----------------------------------------------------------------
    Features added:
      - OPT_HILIGHT: syntax highlighting for 8 languages (C/C++, ASM,
        BASIC, Pascal, FORTRAN, PL/I, COBOL, PL/M)
      - HL_LANG_* constants and hl_lang / hl_enabled / hl_in_comment
        globals for per-language highlight dispatch
      - HL_MONO_* attribute constants for MDA/Hercules monochrome
        display (normal 0x07, bold 0x0F, underline 0x01, bold+uline
        0x09) - collapsed to 3 visible states for readability
      - cf_gutter macro: returns visible gutter width (line-number
        column count when shown, 0 when hidden)
      - LN_MAX_HARD constant (255): hard cap for line storage,
        decoupled from screen width so long lines load without error
      - ln_vis extern: visible editing columns (screen width minus
        gutter), used for status bar display only
      - Config v3 fields: cf_lnum_show, cf_hl_enable, cf_hl_mono
      - Config v4 field:  cf_hard_tabs (keep literal ASCII 0x09)
      - PutStrTabs() prototype: tab-aware string output
      - FileBrowser() prototype: graphical file/directory picker
      - Mouse support: MouseInit/Term/Show/Hide/GetState/SetPos/Poll
        prototypes; mouse_ok, ms_event_line/col/shift externs;
        K_MOUSE_GOTO/BLK_END/PASTE/LINE_SEL synthetic key codes
        (in te_keys.h); bios.h added for bioskey() shift detection
      - VERSION / COPYRIGHT macros updated

    Bugs fixed:
      - ln_max decoupled from screen width; files with lines up to
        255 chars now load successfully
*/

#ifndef TE_H
#define TE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <dos.h>
#include <dir.h>
#include <bios.h>
#include <alloc.h>

/* Include key codes BEFORE using them */
#include "te_keys.h"

/* Version */
#define VERSION   "te-86 v1.0 / MS-DOS Turbo C"
#define COPYRIGHT "(c) 2024-2026 Mickey W. Lawless"

/* Options - all enabled by default */
#define OPT_LWORD  1
#define OPT_RWORD  1
#define OPT_FIND   1
#define OPT_GOTO   1
#define OPT_BLOCK  1
#define OPT_MACRO  1
#define OPT_HILIGHT 1   /* syntax highlighting */

/* ---------------------------------------------------------------
   Syntax highlighting colour attributes (CGA/EGA/VGA)
   Format: background(4 bits) | foreground(4 bits)
   --------------------------------------------------------------- */
#define HL_NORMAL    0x07   /* light grey    on black  - plain text  */
#define HL_KEYWORD   0x0B   /* bright cyan   on black  - keywords    */
#define HL_STRING    0x0E   /* bright yellow on black  - strings     */
#define HL_COMMENT   0x0A   /* bright green  on black  - comments    */
#define HL_NUMBER    0x0D   /* bright magenta on black - numbers     */
#define HL_PREPROC   0x09   /* bright blue   on black  - preprocessor*/
#define HL_OPERATOR  0x0C   /* bright red    on black  - operators   */

/* ---------------------------------------------------------------
   Syntax highlighting monochrome attributes (MDA / Hercules)
   MDA only reliably renders four attribute states:
     0x07 = normal
     0x0F = bright/bold
     0x01 = underline
     0x09 = bright + underline
   Reverse (0x70) is avoided - too harsh for inline tokens.
   Token categories are collapsed to three visible states:
     bright        = structural tokens (keywords, preprocessor)
     underline      = comments
     bright+uline   = string/char literals
     normal         = everything else
   --------------------------------------------------------------- */
#define HL_MONO_NORMAL   0x07   /* normal            - plain text, numbers  */
#define HL_MONO_KEYWORD  0x0F   /* bright            - keywords             */
#define HL_MONO_STRING   0x09   /* bright+underline  - strings              */
#define HL_MONO_COMMENT  0x01   /* underline         - comments             */
#define HL_MONO_NUMBER   0x07   /* normal            - numbers (undecorated)*/
#define HL_MONO_PREPROC  0x0F   /* bright            - preprocessor         */
#define HL_MONO_OPERATOR 0x07   /* normal            - operators            */

/* CRT capabilities */
#define CRT_CAN_REV 1
#define CRT_LONG    1

/* Screen layout */
#define CRT_DEF_ROWS 25
#define CRT_DEF_COLS 80

/* Editor box */
#if CRT_LONG
#define BOX_ROW  2
#else
#define BOX_ROW  1
#endif

/* Status bar positions */
#define PS_ROW     0
#define PS_FNAME   4
#define PS_TXT     "--- | Lin:0000/0000/0000 Col:00/00 Len:00"
#define PS_INF     (cf_cols - 41)
#define PS_CLP     (cf_cols - 41)
#define PS_LIN_CUR (cf_cols - 31)
#define PS_LIN_NOW (cf_cols - 26)
#define PS_LIN_MAX (cf_cols - 21)
#define PS_COL_CUR (cf_cols - 12)
#define PS_COL_NOW (cf_cols -  2)
#define PS_COL_MAX (cf_cols -  9)

/* Filename max length: d:\path\filename.ext + NUL  */
#define FILENAME_MAX_LEN  80

/* Maximum line length for file I/O and buffer allocation.
   This is independent of screen width - a line can be longer
   than the visible editing area; it just scrolls horizontally.
   255 is the practical ceiling for the DOS small memory model. */
#define LN_MAX_HARD  255

/* Forced entry buffer */
#define FORCED_MAX 128

/* Find string buffer */
#define FIND_MAX   32

/* Macro definitions */
#if OPT_MACRO
#define MAC_START   '{'
#define MAC_END     '}'
#define MAC_SEP     ':'
#define MAC_ESCAPE  '\\'
#define MAC_SYM_MAX 10
#define MAC_SYM_SIZ 11
#define MAC_FTYPE   ".m"
#endif

/* Map high-level names to CRT functions */
#define getchr   GetKey
#define putchr   CrtOut

/* ---------------------------------------------------------------
   Configuration - stored as raw byte array in te_conf.c
   Access via functions and macros below.
   --------------------------------------------------------------- */

extern unsigned char cf_data[];

/* Accessor functions (te_conf.c) */
int           CfGetMxLines(void);
unsigned char CfGetRows(void);
unsigned char CfGetCols(void);
unsigned char CfGetTabCols(void);
unsigned char CfGetNum(void);
unsigned char CfGetClang(void);
unsigned char CfGetIndent(void);
unsigned char CfGetList(void);
char         *CfGetListChr(void);
unsigned char CfGetRulChr(void);
unsigned char CfGetRulTab(void);
unsigned char CfGetVertChr(void);
unsigned char CfGetHorzChr(void);
unsigned char CfGetLnumChr(void);
char         *CfGetCrName(void);
char         *CfGetEscName(void);
char         *CfGetName(void);
unsigned char *CfGetKeys(void);
unsigned char *CfGetKeysEx(void);
/* v3 accessors */
unsigned char CfGetLnumShow(void);
unsigned char CfGetHlEnable(void);
unsigned char CfGetHlMono(void);
/* v4 accessors */
unsigned char CfGetHardTabs(void);
/* v5 accessors */
unsigned char CfGetFortContEn(void);
unsigned char CfGetFortContChr(void);
unsigned char CfGetFortStmtEn(void);
unsigned char CfGetFortStmtChr(void);
unsigned char CfGetCobolIndEn(void);
unsigned char CfGetCobolIndChr(void);
unsigned char CfGetTabLineNum(void);

/* Setter functions */
void CfSetRows(unsigned char v);
void CfSetCols(unsigned char v);
void CfSetIndent(unsigned char v);
void CfSetList(unsigned char v);
void CfSetTabCols(unsigned char v);
void CfSetNum(unsigned char v);
void CfSetClang(unsigned char v);
void CfSetRulChr(unsigned char v);
void CfSetRulTab(unsigned char v);
void CfSetVertChr(unsigned char v);
void CfSetHorzChr(unsigned char v);
void CfSetLnumChr(unsigned char v);
void CfSetListChr(char *s);
/* v3 setters */
void CfSetLnumShow(unsigned char v);
void CfSetHlEnable(unsigned char v);
void CfSetHlMono(unsigned char v);
/* v4 setters */
void CfSetHardTabs(unsigned char v);
/* v5 setters */
void CfSetFortContEn(unsigned char v);
void CfSetFortContChr(unsigned char v);
void CfSetFortStmtEn(unsigned char v);
void CfSetFortStmtChr(unsigned char v);
void CfSetCobolIndEn(unsigned char v);
void CfSetCobolIndChr(unsigned char v);
void CfSetTabLineNum(unsigned char v);

/* Config file save/load */
int CfSave(void);
int CfLoad(void);

/* Convenience macros for common config access */
#define cf_mx_lines  CfGetMxLines()
#define cf_rows      CfGetRows()
#define cf_cols      CfGetCols()
#define cf_tab_cols  CfGetTabCols()
#define cf_num       CfGetNum()
#define cf_clang     CfGetClang()
#define cf_indent    CfGetIndent()
#define cf_list      CfGetList()
#define cf_list_chr  CfGetListChr()
#define cf_rul_chr   CfGetRulChr()
#define cf_rul_tab   CfGetRulTab()
#define cf_vert_chr  CfGetVertChr()
#define cf_horz_chr  CfGetHorzChr()
#define cf_lnum_chr  CfGetLnumChr()
#define cf_cr_name   CfGetCrName()
#define cf_esc_name  CfGetEscName()
#define cf_name      CfGetName()
#define cf_keys      CfGetKeys()
#define cf_keys_ex   CfGetKeysEx()
/* v3 macros */
#define cf_lnum_show CfGetLnumShow()
#define cf_hl_enable CfGetHlEnable()
#define cf_hl_mono   CfGetHlMono()
#define cf_hard_tabs CfGetHardTabs()
/* v5 macros */
#define cf_fort_cont_en  CfGetFortContEn()
#define cf_fort_cont_chr CfGetFortContChr()
#define cf_fort_stmt_en  CfGetFortStmtEn()
#define cf_fort_stmt_chr CfGetFortStmtChr()
#define cf_cobol_ind_en  CfGetCobolIndEn()
#define cf_cobol_ind_chr CfGetCobolIndChr()
#define cf_tab_line_num  CfGetTabLineNum()
/* cf_gutter: the actual column offset used by the editor box.
   When line numbers are hidden the gutter collapses to zero.    */
#define cf_gutter    (cf_lnum_show ? (int)CfGetNum() : 0)

/* ---------------------------------------------------------------
   Global variables - declared in te_main.c, extern elsewhere
   --------------------------------------------------------------- */

/* Editor state */
extern char   **lp_arr;       /* line pointer array */
extern int      lp_cur;       /* current line index */
extern int      lp_now;       /* total lines */
extern int      lp_chg;       /* unsaved changes flag */
extern int      ln_max;       /* max line length (hard cap, for I/O) */
extern int      ln_vis;       /* visible editing columns (screen-derived) */
extern char    *ln_dat;       /* line edit buffer */
extern int      box_rows;     /* editor box height */
extern int      box_shr;      /* box scroll row */
extern int      box_shc;      /* cursor column within line (0-based) */
extern int      box_hsc;      /* horizontal scroll offset (columns hidden left) */
extern int      sysln;        /* system line needs clear flag */
extern int      editln;       /* currently in line edit */
extern char     file_name[];

/* Forced input buffer */
extern int      fe_dat[];
extern int      fe_now;
extern int      fe_get;
extern int      fe_set;
extern int      fe_forced;

/* Find */
#if OPT_FIND
extern char     find_str[];
#endif

/* Help items */
extern int      help_items[];

/* Block selection */
#if OPT_BLOCK
extern int      blk_start;
extern int      blk_end;
extern int      blk_count;
#endif

/* Macro */
#if OPT_MACRO
extern FILE    *mac_fp;
extern int      mac_raw;
extern char     mac_sym[];
extern unsigned char mac_indent;
extern unsigned char mac_list;
#endif

/* Syntax highlighting */
#if OPT_HILIGHT
extern int      hl_enabled;    /* non-zero when highlighting is active */
extern int      hl_lang;       /* language ID (HL_LANG_* constant)     */
extern int      hl_in_comment; /* persistent block-comment depth       */

/* Language IDs returned by HlDetect() and stored in hl_lang */
#define HL_LANG_NONE    0
#define HL_LANG_C       1   /* C / C++                  */
#define HL_LANG_ASM     2   /* Assembly (x86)           */
#define HL_LANG_BASIC   3   /* BASIC                    */
#define HL_LANG_PASCAL  4   /* Pascal                   */
#define HL_LANG_FORTRAN 5   /* FORTRAN 77 / 90          */
#define HL_LANG_PLI     6   /* PL/I                     */
#define HL_LANG_COBOL   7   /* COBOL                    */
#define HL_LANG_PLM     8   /* PL/M (Intel 8080/8086)   */
#endif

/* ---------------------------------------------------------------
   Function prototypes
   --------------------------------------------------------------- */

/* te_crt.c */
void CrtInit(void);
void CrtDone(void);
void CrtClear(void);
void CrtClearLine(int row);
void CrtClearEol(void);
void CrtLocate(int row, int col);
void CrtOut(int ch);
int  CrtIn(void);
void CrtReverse(int on);
void CrtGetSize(int *rows, int *cols);
void CrtSetAttr(int attr);   /* set a raw text attribute */

/* te_keys.c */
char *GetKeyName(int key);
char *GetKeyWhat(int key);
int   GetKey(void);

/* te_mouse.c */
int   MouseInit(void);
void  MouseTerm(void);
void  MouseShow(void);
void  MouseHide(void);
int   MouseGetState(int *col, int *row);
void  MouseSetPos(int col, int row);
int   MousePoll(void);
extern int mouse_ok;
extern int ms_event_line;   /* set by MousePoll before returning K_MOUSE_* */
extern int ms_event_col;
extern int ms_event_shift;
extern int ms_event_action; /* set by K_MOUSE_CTX: K_COPY/K_CUT/K_PASTE   */
extern int ms_last_click_col; /* screen col of last right-click (menu pos) */
extern int ms_last_click_row; /* screen row of last right-click (menu pos) */

/* te_ui.c */
void  putchrx(int ch, int n);
void  putstr(char *s);
void  PutStrTabs(char *s, int col);
void  putln(char *s);
void  putint(char *fmt, int val);
void  Layout(void);
void  ShowFilename(void);
void  SysLine(char *s);
void  SysLineEdit(void);
int   SysLineWait(char *s, char *cr, char *esc);
void  SysLineCont(char *s);
void  SysLineBack(char *s);
int   SysLineConf(char *s);
int   SysLineStr(char *what, char *buf, int maxlen);
int   SysLineFile(char *fn);
int   SysLineChanges(void);
int   ReadLine(char *buf, int width);
char *CurrentFile(void);
void  ClearBox(void);
void  CenterText(int row, char *txt);
void  Refresh(int row, int line);
void  RefreshAll(void);
#if OPT_BLOCK
void  RefreshBlock(int row, int sel);
#endif
int   Menu(void);
int   FileBrowser(char *dest);
int   CtxMenu(int scol, int srow);
int   MenuNew(void);
int   MenuOpen(void);
int   MenuSave(void);
int   MenuSaveAs(void);
#if OPT_MACRO
int   MenuInsert(void);
#endif
void  MenuHelp(void);
void  MenuHelpCh(int ch);
void  MenuConfig(void);
void  MenuAbout(void);
int   MenuExit(void);

/* te_file.c */
void  ResetLines(void);
void  NewFile(void);
int   ReadFile(char *fn);
void  BackupFile(char *fn);
int   WriteFile(char *fn);

/* te_lines.c */
int   GetFirstLine(void);
int   GetLastLine(void);
int   SetLine(int line, char *text, int insert);
int   ModifyLine(int line, char *text);
int   ClearLine(int line);
int   InsertLine(int line, char *text);
int   AppendLine(int line, char *text);
int   SplitLine(int line, int pos);
int   DeleteLine(int line);
int   JoinLines(int line);

/* te_edit.c */
int   BfEdit(void);
int   ForceCh(int ch);
int   ForceStr(char *s);
int   ForceGetCh(void);
void  ForceChLeft(int len, int ch);

/* te_misc.c */
char *AllocMem(unsigned int bytes);
void *FreeArray(char **arr, int count, int flag);
#if OPT_MACRO
int   MatchStr(char *s1, char *s2);
#endif

/* te_ui.c (syntax highlighting) */
#if OPT_HILIGHT
void  HlPutStr(char *s);
int   HlDetect(char *fname);
#endif

/* te_error.c */
void  ErrLine(char *s);
void  ErrLineMem(void);
void  ErrLineLong(void);
void  ErrLineOpen(void);
void  ErrLineTooMany(void);

/* te_macro.c */
#if OPT_MACRO
int   MacroRunFile(char *fname, int raw);
int   MacroRunning(void);
void  MacroStop(void);
int   MacroGetRaw(void);
int   MacroIsCmdChar(char ch);
int   MatchSym(char *s);
void  MacroGet(void);
#endif

/* te_loop.c (main editor loop) */
void  Loop(void);
#if OPT_BLOCK
void  LoopBlkUnset(void);
#endif

#endif /* TE_H */

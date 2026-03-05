/*  te.h

    te-86 Text Editor - Turbo C MS-DOS port.

    Definitions.

    Author: Mickey W. Lawless
    Date:   03/01/26

    Original CP/M version: Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    MS-DOS Turbo C port: 2024

    Licensed under the GNU General Public License v2 or later.
*/

#ifndef TE_H
#define TE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <dos.h>
#include <alloc.h>

/* Include key codes BEFORE using them */
#include "te_keys.h"

/* Version */
#define VERSION   "te-86 v1.0 / MS-DOS Turbo C"
#define COPYRIGHT "(c) 2026 Mickey W. Lawless"

/* Options - all enabled by default */
#define OPT_LWORD  1
#define OPT_RWORD  1
#define OPT_FIND   1
#define OPT_GOTO   1
#define OPT_BLOCK  1
#define OPT_MACRO  1

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

/* ---------------------------------------------------------------
   Global variables - declared in te_main.c, extern elsewhere
   --------------------------------------------------------------- */

/* Editor state */
extern char   **lp_arr;       /* line pointer array */
extern int      lp_cur;       /* current line index */
extern int      lp_now;       /* total lines */
extern int      lp_chg;       /* unsaved changes flag */
extern int      ln_max;       /* max line length */
extern char    *ln_dat;       /* line edit buffer */
extern int      box_rows;     /* editor box height */
extern int      box_shr;      /* box scroll row */
extern int      box_shc;      /* box scroll col */
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

/* te_keys.c */
char *GetKeyName(int key);
char *GetKeyWhat(int key);
int   GetKey(void);

/* te_ui.c */
void  putchrx(int ch, int n);
void  putstr(char *s);
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

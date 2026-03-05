/*  te_main.c

    te-86 Text Editor - Entry point and global variable definitions.

    Author: Mickey W. Lawless
    Date:   03/01/26

    Original CP/M version: Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    MS-DOS Turbo C port: 2024

    Usage:  te-86 [filename]
*/

#include "te.h"

/* ---------------------------------------------------------------
   Global variable DEFINITIONS (declared extern in te.h)
   --------------------------------------------------------------- */

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

/* ---------------------------------------------------------------
   Help items table  (key-function codes, -1 = end-of-table)
   Layout: 3 per row in the help display.
   --------------------------------------------------------------- */
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

/* ---------------------------------------------------------------
   main
   --------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    int rows, cols;

    /* --- Initialise CRT --- */
    CrtInit();

    /* --- Load configuration from TE.CFG if it exists --- */
    CfLoad();

    /* --- Auto-detect screen size --- */
    CrtGetSize(&rows, &cols);
    if (cf_rows == 0 || cf_rows > (unsigned char)rows) CfSetRows((unsigned char)rows);
    if (cf_cols == 0 || cf_cols > (unsigned char)cols) CfSetCols((unsigned char)cols);

    /* --- Allocate line-pointer array --- */
    lp_arr = (char **)calloc(cf_mx_lines + 1, sizeof(char *));
    if (!lp_arr) {
        CrtDone();
        fprintf(stderr, "te-86: not enough memory\n");
        return 1;
    }

    /* --- Allocate line edit buffer --- */
    /*
     * ln_max is the maximum number of characters on one line.
     * +2 for the CR that fgets may read and the NUL terminator.
     */
    ln_max = cf_cols - cf_num - 1;   /* usable editing columns      */
    if (ln_max < 1) ln_max = 1;

    ln_dat = (char *)malloc(ln_max + 4);
    if (!ln_dat) {
        free(lp_arr);
        CrtDone();
        fprintf(stderr, "te-86: not enough memory\n");
        return 1;
    }

    /* --- Compute editor box height --- */
    /*
     * Row 0          : status bar
     * Row 1          : ruler (if CRT_LONG)
     * Row BOX_ROW .. cf_rows-3 : editor box
     * Row cf_rows-2  : horizontal separator
     * Row cf_rows-1  : system line
     */
    box_rows = cf_rows - BOX_ROW - 2;
    if (box_rows < 1) box_rows = 1;

    /* --- Draw initial layout --- */
    Layout();

    /* --- Load file from command line or start fresh --- */
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

    /* --- Enter the main edit loop --- */
    Loop();

    /* --- Clean up --- */
    FreeArray(lp_arr, cf_mx_lines, 0);
    free(lp_arr);
    free(ln_dat);

    CrtDone();

    return 0;
}

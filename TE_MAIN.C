/*  te_main.c

    te-86 Text Editor - Entry point and global definitions.

    TE (CP/M)  - Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    TE-86 (DOS) - Copyright (c) 2024-2026 Mickey W. Lawless

    Licensed under the GNU General Public License v2 or later.

    Usage:  te-86 [filename]

    Changes (TE-86 DOS port):
    -----------------------------------------------------------------
    Features added:
      - hl_lang, hl_enabled globals: set by HlDetect() on file open
      - hl_in_comment global: persistent block-comment state across
        lines; reset to 0 on every file open/new so comment state
        never bleeds from one file into the next
      - ln_vis global: visible editing columns (cf_cols - cf_num - 1)
        used by status bar; separate from ln_max (storage limit)
      - box_hsc global: horizontal scroll offset; tracks how many columns
        the view is scrolled right so long lines can be viewed and edited

    Bugs fixed:
      - ln_max now always set to LN_MAX_HARD (255) regardless of
        screen width; was previously set to cf_cols - cf_num - 1
        which caused "line too long" errors on valid files
      - Screen geometry (cf_rows / cf_cols) now always taken from
        CrtGetSize() hardware result, overriding any stale value
        that may have been saved in the .CFG file
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
int      ln_max    = LN_MAX_HARD; /* max line length - hard cap          */
int      ln_vis    = 0;           /* visible editing columns (screen)    */
char    *ln_dat    = NULL;    /* working line buffer                 */
int      box_rows  = 0;       /* editor box height in rows           */
int      box_shr   = 0;       /* editor box: cursor row within box   */
int      box_shc   = 0;       /* editor box: cursor col within line  */
int      box_hsc   = 0;       /* editor box: horizontal scroll offset*/
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

/* Syntax highlighting */
#if OPT_HILIGHT
int      hl_enabled    = 0;   /* set by HlDetect() on file open      */
int      hl_lang       = 0;   /* language ID set by HlDetect()       */
int      hl_in_comment = 0;   /* persistent block-comment state      */
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
    /* Always use the actual terminal dimensions.  Never allow the stored
       config value to exceed what the hardware reports -- if it does the
       editor box would overlap the separator / system line.               */
    CrtGetSize(&rows, &cols);
    if (rows < 8)  rows = 8;    /* absolute minimum to be usable */
    if (cols < 20) cols = 20;
    CfSetRows((unsigned char)rows);
    CfSetCols((unsigned char)cols);

    /* --- Allocate line-pointer array --- */
    lp_arr = (char **)calloc(cf_mx_lines + 1, sizeof(char *));
    if (!lp_arr) {
        CrtDone();
        fprintf(stderr, "te-86: not enough memory\n");
        return 1;
    }

    /* --- Allocate line edit buffer --- */
    /*
     * ln_max is the hard maximum characters per line (LN_MAX_HARD = 255).
     * This governs file I/O and storage - it is NOT tied to screen width,
     * so files with lines wider than the screen load without error.
     *
     * ln_vis is the number of visible editing columns derived from the
     * screen geometry.  It is used only for the status bar display and
     * horizontal scroll clamping.
     *
     * +2: room for the CR that fgets may read and the NUL terminator.
     */
    ln_max = LN_MAX_HARD;
    ln_vis = cf_cols - cf_num - 1;
    if (ln_vis < 1) ln_vis = 1;

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

#if OPT_HILIGHT
    hl_lang       = HlDetect(file_name);
    hl_enabled    = (hl_lang != HL_LANG_NONE);
    hl_in_comment = 0;
    if (!cf_hl_enable)
        hl_enabled = 0;
#endif

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

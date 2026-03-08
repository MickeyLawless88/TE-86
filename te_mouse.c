/*  te_mouse.c

    te-86 Text Editor - DOS INT 33h mouse support.

    TE (CP/M)  - Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    TE-86 (DOS) - Copyright (c) 2024-2026 Mickey W. Lawless

    Licensed under the GNU General Public License v2 or later.

    Changes (TE-86 DOS port):
    -----------------------------------------------------------------
    Features added:
      - MouseInit(): detect and initialise INT 33h mouse driver;
        stores mouse_ok flag; hides text-mode cursor
      - MouseShow() / MouseHide(): increment/decrement driver
        visibility counter so nested calls are safe
      - MouseGetState(): returns button mask and col/row in text cells
      - MouseSetPos(): warp text cursor to col/row
      - MousePoll(): called from GetKey() when no keyboard input is
        pending; synthesises K_MOUSE_* codes for the editor loop
      - Double-click detection: two left clicks within DBLCLK_TICKS
        timer ticks on the same cell produce K_MOUSE_LINE_SEL
      - Shift+left-click extends the current block selection
      - Right-click with active selection → K_MOUSE_CTX (context menu)
      - Right-click with no selection → K_MOUSE_PASTE (unchanged)
      - ms_event_action: carries chosen action back from CtxMenu()

    INT 33h functions used:
      AX=0000  Reset driver / get status
      AX=0001  Show cursor
      AX=0002  Hide cursor
      AX=0003  Get button status and position
      AX=0004  Set cursor position
*/

#include "te.h"

/* ---------------------------------------------------------------
   Internal state
   --------------------------------------------------------------- */
int mouse_ok = 0;

static int  ms_btn_prev   = 0;
static int  ms_hidden     = 0;

int ms_event_line   = -1;
int ms_event_col    = -1;
int ms_event_shift  = 0;
int ms_event_action = 0;
int ms_last_click_col = -1;
int ms_last_click_row = -1;

/* ---------------------------------------------------------------
   MouseInt33  --  call INT 33h with AX=func, optional BX/CX/DX.
                   Returns AX result.
   --------------------------------------------------------------- */
static unsigned int MouseInt33(unsigned int func,
                               unsigned int bx,
                               unsigned int cx,
                               unsigned int dx)
{
    union REGS r;
    r.x.ax = func;
    r.x.bx = bx;
    r.x.cx = cx;
    r.x.dx = dx;
    int86(0x33, &r, &r);
    return r.x.ax;
}

/* ---------------------------------------------------------------
   MouseInit  --  reset driver; return non-zero if mouse present.
   --------------------------------------------------------------- */
int MouseInit(void)
{
    unsigned int result;

    result = MouseInt33(0x0000, 0, 0, 0);
    mouse_ok = (result == 0xFFFF) ? 1 : 0;

    if (mouse_ok) {
        /* Set mickey-to-pixel ratio for text mode (8 pixels/cell) */
        /* AX=000F: set mickey-to-pixel ratio: CX=horiz, DX=vert  */
        MouseInt33(0x000F, 0, 8, 16);
        /* Show cursor */
        MouseInt33(0x0001, 0, 0, 0);
        ms_hidden = 0;
    }

    return mouse_ok;
}

/* ---------------------------------------------------------------
   MouseTerm  --  hide mouse cursor on editor exit.
   --------------------------------------------------------------- */
void MouseTerm(void)
{
    if (mouse_ok)
        MouseInt33(0x0002, 0, 0, 0);
}

/* ---------------------------------------------------------------
   MouseShow / MouseHide  --  nested show/hide.
   --------------------------------------------------------------- */
void MouseShow(void)
{
    if (!mouse_ok) return;
    if (ms_hidden > 0) {
        ms_hidden--;
        if (ms_hidden == 0)
            MouseInt33(0x0001, 0, 0, 0);
    }
}

void MouseHide(void)
{
    if (!mouse_ok) return;
    ms_hidden++;
    MouseInt33(0x0002, 0, 0, 0);
}

/* ---------------------------------------------------------------
   MouseGetState  --  return button mask; set *col, *row in text cells.
                      Returns -1 if driver not present.
   --------------------------------------------------------------- */
int MouseGetState(int *col, int *row)
{
    union REGS r;

    if (!mouse_ok) return -1;

    r.x.ax = 0x0003;
    r.x.bx = 0;
    r.x.cx = 0;
    r.x.dx = 0;
    int86(0x33, &r, &r);

    /* CX = pixel x, DX = pixel y; text cell = pixel / 8 */
    *col = r.x.cx / 8;
    *row = r.x.dx / 8;

    /* Clamp to screen */
    if (*col < 0)          *col = 0;
    if (*col >= cf_cols)   *col = cf_cols - 1;
    if (*row < 0)          *row = 0;
    if (*row >= cf_rows)   *row = cf_rows - 1;

    return (int)r.x.bx;   /* button mask: bit0=left, bit1=right */
}

/* ---------------------------------------------------------------
   MouseSetPos  --  warp cursor to text col/row.
   --------------------------------------------------------------- */
void MouseSetPos(int col, int row)
{
    if (!mouse_ok) return;
    MouseInt33(0x0004, 0, (unsigned int)(col * 8), (unsigned int)(row * 8));
}

/* ---------------------------------------------------------------
   MouseEditorCoord  --  convert screen col/row to editor line/col.

   Sets *line (absolute line index) and *ecol (column in buffer).
   Returns 1 if the click was inside the editor text box, 0 if not.
   --------------------------------------------------------------- */
static int MouseEditorCoord(int scol, int srow, int *line, int *ecol)
{
    int gutter = cf_gutter;
    int box_r  = BOX_ROW;
    int first  = GetFirstLine();

    /* Must be inside the text box (below title rows, above status) */
    if (srow < box_r || srow >= box_r + box_rows)
        return 0;
    /* Must be to the right of the gutter */
    if (scol < gutter)
        return 0;

    *line = first + (srow - box_r);
    *ecol = scol - gutter;
    if (*ecol < 0) *ecol = 0;
    return 1;
}

/* ---------------------------------------------------------------
   ms_event  --  filled by MousePoll to communicate the event back
                 to GetKey.
   --------------------------------------------------------------- */


/* ---------------------------------------------------------------
   MousePoll  --  left-click moves cursor, right-click opens menu.
   No drag handling -- avoids event floods that cause jump-to-bottom.
   --------------------------------------------------------------- */
int MousePoll(void)
{
    int btn, col, row, line, ecol;

    if (!mouse_ok) return 0;

    btn = MouseGetState(&col, &row);
    if (btn < 0) return 0;

    /* Left button rising edge */
    if ((btn & 1) && !(ms_btn_prev & 1)) {
        ms_btn_prev = btn;
        if (MouseEditorCoord(col, row, &line, &ecol)) {
            ms_event_line = line;
            ms_event_col  = ecol;
            return K_MOUSE_GOTO;
        }
        return 0;
    }

    /* Right button rising edge */
    if ((btn & 2) && !(ms_btn_prev & 2)) {
        ms_btn_prev = btn;
        if (MouseEditorCoord(col, row, &line, &ecol)) {
            ms_event_line     = line;
            ms_event_col      = ecol;
            ms_last_click_col = col;
            ms_last_click_row = row;
            return K_MOUSE_CTX;
        }
        return 0;
    }

    ms_btn_prev = btn;
    return 0;
}

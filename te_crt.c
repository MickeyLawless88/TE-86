/*  te_crt.c		

    te-86 Text Editor - CRT (console) abstraction layer.

    Author: Mickey W. Lawless
    Date:   03/01/26

    Replaces the CP/M BIOS console calls with Turbo C / MS-DOS equivalents.
    All screen operations go through this file so porting to another
    compiler only requires changing this one module.

    Original CP/M version: Copyright (c) 2015-2021 Mickey W. Lawless
    MS-DOS Turbo C port: 2024
*/

#include "te.h"

/* ---------------------------------------------------------------
   Internal state
   --------------------------------------------------------------- */
static int  crt_reverse = 0;   /* current reverse-video flag */

/* Normal and reverse text attributes (CGA/EGA/VGA) */
#define ATTR_NORMAL  0x07      /* light grey on black */
#define ATTR_REVERSE 0x70      /* black on light grey */

/* ---------------------------------------------------------------
   CrtInit  --  initialise the console
   --------------------------------------------------------------- */
void CrtInit(void)
{
    /* Switch to a known text mode (80x25) */
    textmode(C80);
    textattr(ATTR_NORMAL);
    clrscr();
    _setcursortype(_NORMALCURSOR);
    crt_reverse = 0;
}

/* ---------------------------------------------------------------
   CrtDone  --  restore console on exit
   --------------------------------------------------------------- */
void CrtDone(void)
{
    textattr(ATTR_NORMAL);
    clrscr();
    _setcursortype(_NORMALCURSOR);
    gotoxy(1, 1);
}

/* ---------------------------------------------------------------
   CrtClear  --  clear the entire screen
   --------------------------------------------------------------- */
void CrtClear(void)
{
    textattr(crt_reverse ? ATTR_REVERSE : ATTR_NORMAL);
    clrscr();
}

/* ---------------------------------------------------------------
   CrtClearLine  --  clear one screen row (0-based)
   --------------------------------------------------------------- */
void CrtClearLine(int row)
{
    gotoxy(1, row + 1);   /* Turbo C gotoxy is 1-based */
    textattr(ATTR_NORMAL);
    clreol();
}

/* ---------------------------------------------------------------
   CrtClearEol  --  clear from cursor to end of line
   --------------------------------------------------------------- */
void CrtClearEol(void)
{
    textattr(ATTR_NORMAL);
    clreol();
}

/* ---------------------------------------------------------------
   CrtLocate  --  position the cursor (0-based row, col)
   --------------------------------------------------------------- */
void CrtLocate(int row, int col)
{
    gotoxy(col + 1, row + 1);
}

/* ---------------------------------------------------------------
   CrtOut  --  output one character at current cursor position.
               Characters that would cause the terminal to wrap
               onto the next row are silently dropped -- the caller
               is responsible for horizontal scrolling.
   --------------------------------------------------------------- */
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
            /* Suppress any character that would land on or past the
               last column -- putch() would advance the cursor to the
               next row and corrupt the separator / system line.       */
            if (wherex() < (int)cf_cols)
                putch(ch);
            break;
    }
}

/* ---------------------------------------------------------------
   CrtIn  --  read one raw key from keyboard.
              Returns the ASCII value, or 0 followed by the
              extended scan code on the NEXT call (like BIOS).
              We store the pending extended code in a static.
   --------------------------------------------------------------- */
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
        /* Extended key: second byte is scan code */
        pending = getch();
        return 0;
    }

    return ch;
}

/* ---------------------------------------------------------------
   CrtReverse  --  toggle reverse video attribute
   --------------------------------------------------------------- */
void CrtReverse(int on)
{
    crt_reverse = on;
    textattr(on ? ATTR_REVERSE : ATTR_NORMAL);
}

/* ---------------------------------------------------------------
   CrtSetAttr  --  set an arbitrary text colour attribute
   --------------------------------------------------------------- */
void CrtSetAttr(int attr)
{
    textattr(attr);
}

/* ---------------------------------------------------------------
   CrtGetSize  --  return current screen dimensions
   --------------------------------------------------------------- */
void CrtGetSize(int *rows, int *cols)
{
    struct text_info ti;
    gettextinfo(&ti);
    *rows = ti.screenheight;
    *cols = ti.screenwidth;
}

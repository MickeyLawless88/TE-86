/*  te_crt.c

    te-86 Text Editor - CRT (console) abstraction layer.

    TE (CP/M)  - Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    TE-86 (DOS) - Copyright (c) 2024-2026 Mickey W. Lawless

    Licensed under the GNU General Public License v2 or later.

    Character output uses direct video RAM writes (VidPoke) so text
    attributes work correctly on all adapter types.  The video segment
    is auto-detected at CrtInit time by reading BIOS data area byte
    0040:0049: mode 7 = MDA/Hercules (0xB000), all others = 0xB800.

    Changes (TE-86 DOS port):
    -----------------------------------------------------------------
    Features added:
      - VidPoke(): writes char+attribute directly to video RAM via
        far pointer; replaces putch() for all character output
      - Auto-detection of MDA (0xB000) vs CGA/EGA/VGA (0xB800)
        video segment at CrtInit time via BIOS data area
      - crt_attr static tracks current attribute; CrtSetAttr() stores
        it so subsequent CrtOut() calls use it in the VidPoke write
      - CrtClearLine() and CrtClearEol() use VidPoke loops so cleared
        cells get the correct attribute byte

    Bugs fixed:
      - putch() uses BIOS TTY write (INT 10h AH=0Eh) which ignores
        the current text attribute entirely; replaced with direct
        video RAM writes so CrtSetAttr() actually takes effect
      - Syntax highlight colour/attribute changes now work correctly
        on MDA/Hercules, CGA, EGA, and VGA
      - Characters that would wrap past the last column are silently
        dropped to prevent terminal auto-wrap corrupting the separator
        and system lines
*/

#include "te.h"

/* ---------------------------------------------------------------
   Internal state
   --------------------------------------------------------------- */
static int           crt_reverse  = 0;    /* reverse-video flag      */
static int           crt_attr     = 0x07; /* current text attribute  */
static unsigned int  crt_vseg     = 0xB800; /* video RAM segment      */
static int           crt_cols     = 80;   /* screen width (cached)   */
static int           crt_rows     = 25;   /* screen height (cached)  */

/* Normal and reverse text attributes */
#define ATTR_NORMAL  0x07      /* light grey on black  */
#define ATTR_REVERSE 0x70      /* black on light grey  */

/* ---------------------------------------------------------------
   VidPoke  --  write char+attr directly to video RAM.
                row, col are 0-based.
   --------------------------------------------------------------- */
static void VidPoke(int row, int col, int ch, int attr)
{
    unsigned int offset;
    unsigned char far *vp;

    offset = (unsigned int)(row * crt_cols + col) * 2;
    vp = (unsigned char far *)MK_FP(crt_vseg, offset);
    *vp       = (unsigned char)ch;
    *(vp + 1) = (unsigned char)attr;
}

/* ---------------------------------------------------------------
   CrtInit  --  initialise the console; detect video adapter.
   --------------------------------------------------------------- */
void CrtInit(void)
{
    struct text_info ti;

    /* Detect adapter: BIOS data area byte at 0040:0049 holds the
       current video mode.  Mode 7 = MDA 80x25 mono.              */
    {
        unsigned char far *bda_mode = (unsigned char far *)MK_FP(0x0040, 0x0049);
        crt_vseg = (*bda_mode == 7) ? 0xB000 : 0xB800;
    }

    textmode(C80);
    textattr(ATTR_NORMAL);
    clrscr();
    _setcursortype(_NORMALCURSOR);
    crt_reverse = 0;
    crt_attr    = ATTR_NORMAL;

    gettextinfo(&ti);
    crt_cols = ti.screenwidth;
    crt_rows = ti.screenheight;
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
    int attr = crt_reverse ? ATTR_REVERSE : ATTR_NORMAL;
    textattr(attr);
    crt_attr = attr;
    clrscr();
}

/* ---------------------------------------------------------------
   CrtClearLine  --  clear one screen row (0-based)
   --------------------------------------------------------------- */
void CrtClearLine(int row)
{
    int col;
    for (col = 0; col < crt_cols; ++col)
        VidPoke(row, col, ' ', ATTR_NORMAL);
    gotoxy(1, row + 1);
    crt_attr = ATTR_NORMAL;
}

/* ---------------------------------------------------------------
   CrtClearEol  --  clear from cursor to end of line
   --------------------------------------------------------------- */
void CrtClearEol(void)
{
    int col = wherex() - 1;   /* 0-based */
    int row = wherey() - 1;
    int c;
    for (c = col; c < crt_cols; ++c)
        VidPoke(row, c, ' ', ATTR_NORMAL);
    gotoxy(col + 1, row + 1);
    crt_attr = ATTR_NORMAL;
}

/* ---------------------------------------------------------------
   CrtLocate  --  position the cursor (0-based row, col)
   --------------------------------------------------------------- */
void CrtLocate(int row, int col)
{
    gotoxy(col + 1, row + 1);
}

/* ---------------------------------------------------------------
   CrtOut  --  output one character at the current cursor position
               using a direct video RAM write so the current
               attribute is always applied correctly regardless of
               adapter type.  Characters that would wrap past the
               last column are silently dropped.
   --------------------------------------------------------------- */
void CrtOut(int ch)
{
    int x, y;

    switch (ch) {
        case '\b':
            x = wherex();
            if (x > 1) {
                --x;
                gotoxy(x, wherey());
                VidPoke(wherey() - 1, x - 1, ' ', ATTR_NORMAL);
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
            x = wherex();
            if (x < crt_cols) {
                VidPoke(wherey() - 1, x - 1, ch, crt_attr);
                gotoxy(x + 1, wherey());
            }
            break;
    }
}

/* ---------------------------------------------------------------
   CrtIn  --  read one raw key from keyboard.
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
    crt_attr    = on ? ATTR_REVERSE : ATTR_NORMAL;
    textattr(crt_attr);
}

/* ---------------------------------------------------------------
   CrtSetAttr  --  set an arbitrary text attribute.
                   Stores it in crt_attr so subsequent CrtOut
                   calls use it via the direct video RAM write.
   --------------------------------------------------------------- */
void CrtSetAttr(int attr)
{
    crt_attr = attr;
    textattr(attr);   /* keep Turbo C's internal state in sync
                         for clrscr / clreol calls               */
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

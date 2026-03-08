/*  te_error.c

    te-86 Text Editor - Error messages.

    TE (CP/M)  - Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    TE-86 (DOS) - Copyright (c) 2024-2026 Mickey W. Lawless

    Licensed under the GNU General Public License v2 or later.
*/

#include "te.h"

void ErrLine(char *s)
{
    SysLineCont(s);

    /* If we were in the middle of editing, restore the hint and cursor */
    if (editln) {
        SysLineEdit();
        sysln = 0;
        CrtLocate(BOX_ROW + box_shr, box_shc + cf_num);
    }
}

void ErrLineMem(void)    { ErrLine("Not enough memory"); }
void ErrLineLong(void)   { ErrLine("Line too long");     }
void ErrLineOpen(void)   { ErrLine("Can't open");        }
void ErrLineTooMany(void){ ErrLine("Too many lines");    }

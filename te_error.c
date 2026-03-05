/*  te_err.c

    te-86 Text Editor - Error messages.

    Author: Mickey W. Lawless
    Date:   03/01/26

    Original CP/M version: Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    MS-DOS Turbo C port: 2024
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

/*  te_misc.c

    te-86 Text Editor - Miscellaneous utilities.

    TE (CP/M)  - Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    TE-86 (DOS) - Copyright (c) 2024-2026 Mickey W. Lawless

    Licensed under the GNU General Public License v2 or later.
*/

#include "te.h"

/* ---------------------------------------------------------------
   AllocMem  --  malloc with error message on failure
   --------------------------------------------------------------- */
char *AllocMem(unsigned int bytes)
{
    char *p;

    if (!(p = (char *)malloc(bytes)))
        ErrLineMem();

    return p;
}

/* ---------------------------------------------------------------
   FreeArray  --  free every non-NULL pointer in an array.
                  If flag != 0, also free the array itself.
   --------------------------------------------------------------- */
void *FreeArray(char **arr, int count, int flag)
{
    int i;

    for (i = 0; i < count; ++i) {
        if (arr[i]) {
            free(arr[i]);
            arr[i] = NULL;
        }
    }

    if (flag)
        free(arr);

    return NULL;
}

#if OPT_MACRO
/* ---------------------------------------------------------------
   MatchStr  --  return non-zero if the two strings are equal
   --------------------------------------------------------------- */
int MatchStr(char *s1, char *s2)
{
    return !strcmp(s1, s2);
}
#endif

/*  te_keys.c

    te-86 Text Editor - Key bindings.

    TE (CP/M)  - Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    TE-86 (DOS) - Copyright (c) 2024-2026 Mickey W. Lawless

    Licensed under the GNU General Public License v2 or later.
*/

#include "te.h"

/* ---------------------------------------------------------------
   GetKeyName  --  return display name for CR and ESC keys
   --------------------------------------------------------------- */
char *GetKeyName(int key)
{
    switch (key) {
        case K_CR:  return cf_cr_name;
        case K_ESC: return cf_esc_name;
    }
    return "?";
}

/* ---------------------------------------------------------------
   GetKeyWhat  --  return short purpose string for a key code
   --------------------------------------------------------------- */
char *GetKeyWhat(int key)
{
    switch (key) {
        case K_UP:        return "Up";
        case K_DOWN:      return "Down";
        case K_LEFT:      return "Left";
        case K_RIGHT:     return "Right";
        case K_BEGIN:     return "Begin";
        case K_END:       return "End";
        case K_TOP:       return "Top";
        case K_BOTTOM:    return "Bottom";
        case K_PGUP:      return "PgUp";
        case K_PGDOWN:    return "PgDown";
        case K_TAB:       return "Indent";
        case K_CR:        return "NewLine";
        case K_ESC:       return "Escape";
        case K_RDEL:      return "DelRight";
        case K_LDEL:      return "DelLeft";
        case K_CUT:       return "Cut";
        case K_COPY:      return "Copy";
        case K_PASTE:     return "Paste";
        case K_DELETE:    return "Delete";
        case K_CLRCLP:    return "ClearClip";
#if OPT_FIND
        case K_FIND:      return "Find";
        case K_NEXT:      return "FindNext";
#endif
#if OPT_GOTO
        case K_GOTO:      return "GoLine";
#endif
#if OPT_LWORD
        case K_LWORD:     return "WordLeft";
#endif
#if OPT_RWORD
        case K_RWORD:     return "WordRight";
#endif
#if OPT_BLOCK
        case K_BLK_START: return "BlockStart";
        case K_BLK_END:   return "BlockEnd";
        case K_BLK_UNSET: return "BlockUnset";
#endif
#if OPT_MACRO
        case K_MACRO:     return "Macro";
#endif
    }
    return "?";
}

/* ---------------------------------------------------------------
   GetKey  --  translate raw keyboard input to editor key codes.

   On MS-DOS, extended keys (arrows, F-keys, etc.) produce a
   two-byte sequence: 0x00 followed by a scan code.  CrtIn()
   handles that split and returns the two bytes on successive
   calls.  We check cf_keys[] and cf_keys_ex[] exactly as the
   original CP/M code did.

   When no keyboard input is pending we call MousePoll() so mouse
   events are interleaved naturally with keystrokes.
   --------------------------------------------------------------- */
int GetKey(void)
{
    int c, x, i, ms;

    /* Poll mouse first if no keyboard input is waiting */
    while (!kbhit()) {
        ms = MousePoll();
        if (ms) return ms;
        /* Tight-loop is fine for DOS; could add a short delay here
           if power consumption matters on battery-powered hardware  */
    }

    c = CrtIn();

    /* Printable characters pass through directly */
    if (c > 31 && c != 127) {
        return c;
    }

    /* Direct mapping for common control keys */
    if (c == 27)  return K_ESC;
    if (c == 13)  return K_CR;
    if (c == 9)   return K_TAB;
    if (c == 8)   return K_LDEL;

    /* Ctrl+C = Copy, Ctrl+X = Cut, Ctrl+V = Paste (CUA conventions) */
    if (c == CTL_C) return K_COPY;
    if (c == CTL_X) return K_CUT;
    if (c == CTL_V) return K_PASTE;
    /* Ctrl+R = context menu */
    if (c == CTL_R) return K_CTX_MENU;
    /* Block selection -- hardcoded before table so .CFG cannot override */
    if (c == CTL_B) return K_BLK_START;
    if (c == CTL_E) return K_BLK_END;
    if (c == CTL_U) return K_BLK_UNSET;

    /* Extended key (0x00 or 0xE0 prefix)? */
    if (c == 0 || c == 0xE0) {
        /* Read the scan code */
        x = CrtIn();

        /* Search for matching extended key binding */
        for (i = 0; i < KEYS_MAX; ++i) {
            if (cf_keys[i] == 0 && cf_keys_ex[i] == x) {
                return i + 1000;
            }
        }
        /* Unknown extended key */
        return '?';
    }

    /* Single-byte control key - scan the key-binding table */
    for (i = 0; i < KEYS_MAX; ++i) {
        if (cf_keys[i] && c == cf_keys[i] && cf_keys_ex[i] == 0) {
            return i + 1000;
        }
    }

    return '?';
}


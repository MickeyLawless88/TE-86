/*  te_macro.c

    te-86 Text Editor - Macros.

    Author: Mickey W. Lawless
    Date:   03/01/26

    Original CP/M version: Copyright (c) 2015-2021 Mickey W. Lawless
    MS-DOS Turbo C port: 2024
*/

#include "te.h"

#if OPT_MACRO

/* ---------------------------------------------------------------
   MacroRunFile  --  open a macro file and begin execution.
                     raw != 0: treat file content as literal text
                               (used for "Insert file" menu option).
                     Returns non-zero on error.
   --------------------------------------------------------------- */
int MacroRunFile(char *fname, int raw)
{
    if (!(mac_fp = fopen(fname, "r"))) {
        ErrLineOpen();
        return -1;
    }

    mac_raw = raw;

    /* Save and disable auto-features during macro execution */
    mac_indent = cf_indent;
    mac_list   = cf_list;
    CfSetIndent(0);
    CfSetList(0);

    return 0;
}

/* ---------------------------------------------------------------
   MacroRunning  --  return non-zero if a macro is active
   --------------------------------------------------------------- */
int MacroRunning(void)
{
    return mac_fp != NULL;
}

/* ---------------------------------------------------------------
   MacroStop  --  close the macro file and restore settings
   --------------------------------------------------------------- */
void MacroStop(void)
{
    if (mac_fp) {
        fclose(mac_fp);
        mac_fp = NULL;
    }

    CfSetIndent(mac_indent);
    CfSetList(mac_list);

    /* Signal end of macro input to the forced-input buffer */
    ForceCh('\0');
}

/* ---------------------------------------------------------------
   MacroGetRaw  --  read one character from the macro file.
                    Returns '\0' when done / on error.
   --------------------------------------------------------------- */
int MacroGetRaw(void)
{
    int ch;

    if (mac_fp) {
        if (mac_raw) {
            switch (ch = fgetc(mac_fp)) {
                case '\n': ch = K_CR; break;
                case '\t': ch = ' ';  break;
            }
        }
        else {
            /* Normal mode: skip newlines */
            while ((ch = fgetc(mac_fp)) == '\n')
                ;
        }

        if (ch != EOF) {
            if (ch < 32 || ch == 127)
                ch = '?';
            return ch;
        }

        MacroStop();
    }

    return '\0';
}

/* ---------------------------------------------------------------
   MacroIsCmdChar  --  test if ch is legal inside a symbol name
   --------------------------------------------------------------- */
int MacroIsCmdChar(char ch)
{
    return isalpha((unsigned char)ch) || ch == '#' || ch == '+' || ch == '-';
}

/* ---------------------------------------------------------------
   MatchSym  --  return non-zero if mac_sym equals s
   --------------------------------------------------------------- */
int MatchSym(char *s)
{
    return MatchStr(mac_sym, s);
}

/* ---------------------------------------------------------------
   MacroGet  --  parse one macro unit and push it into the
                 forced-input buffer.
   --------------------------------------------------------------- */
void MacroGet(void)
{
    int  i, n, ch;

    if (!(ch = MacroGetRaw()))
        return;

    /* Raw mode: push the character directly */
    if (mac_raw) {
        ForceCh(ch);
        return;
    }

    /* Plain character (not a symbol start) */
    if (ch != MAC_START) {
        if (ch != MAC_ESCAPE) {
            ForceCh(ch);
        }
        else {
            if ((ch = MacroGetRaw())) {
                ForceCh(ch);
            }
            else {
                ErrLine("Bad escape sequence");
                MacroStop();
            }
        }
        return;
    }

    /* Parse symbol name: {name} or {name:repeat} */
    for (i = 0; MacroIsCmdChar((char)(ch = MacroGetRaw())) && i < MAC_SYM_MAX; ++i)
        mac_sym[i] = (char)tolower(ch);

    if (i) {
        mac_sym[i] = '\0';

        /* Optional repeat count after ':' */
        if (ch == MAC_SEP) {
            n = 0;
            while (isdigit(ch = MacroGetRaw()))
                n = n * 10 + ch - '0';
            if (n < 0 || n > FORCED_MAX)
                n = -1;
        }
        else {
            n = 1;
        }

        if (n >= 0) {
            /* Comment: {# some text} */
            if (ch == ' ' && MatchSym("#")) {
                while ((ch = MacroGetRaw())) {
                    if (ch == MAC_END) {
                        ForceCh('\0');
                        return;
                    }
                }
            }

            if (ch == MAC_END) {
                ch = 0;

                if      (MatchSym("up"))        ch = K_UP;
                else if (MatchSym("down"))      ch = K_DOWN;
                else if (MatchSym("left"))      ch = K_LEFT;
                else if (MatchSym("right"))     ch = K_RIGHT;
                else if (MatchSym("begin"))     ch = K_BEGIN;
                else if (MatchSym("end"))       ch = K_END;
                else if (MatchSym("top"))       ch = K_TOP;
                else if (MatchSym("bottom"))    ch = K_BOTTOM;
                else if (MatchSym("newline"))   ch = K_CR;
                else if (MatchSym("indent"))    ch = K_TAB;
                else if (MatchSym("delright"))  ch = K_RDEL;
                else if (MatchSym("delleft"))   ch = K_LDEL;
                else if (MatchSym("cut"))       ch = K_CUT;
                else if (MatchSym("copy"))      ch = K_COPY;
                else if (MatchSym("paste"))     ch = K_PASTE;
                else if (MatchSym("delete"))    ch = K_DELETE;
                else if (MatchSym("clearclip")) ch = K_CLRCLP;
#if OPT_BLOCK
                else if (MatchSym("blockstart"))ch = K_BLK_START;
                else if (MatchSym("blockend"))  ch = K_BLK_END;
#endif

                if (ch) {
                    while (n--)
                        if (ForceCh(ch)) break;
                    return;
                }

                /* Special / non-key commands */
                if (MatchSym("filename")) {
                    while (n--)
                        ForceStr(CurrentFile());
                    return;
                }
                else if (MatchSym("autoindent")) {
                    CfSetIndent(n ? 1 : 0);
                    ForceCh('\0');
                    return;
                }
                else if (MatchSym("autolist")) {
                    CfSetList(n ? 1 : 0);
                    ForceCh('\0');
                    return;
                }
            }
        }
    }

    /* Bad symbol */
    ErrLine("Bad symbol");
    MacroStop();
}

#endif /* OPT_MACRO */

/*  te_conf.c

    te-86 Text Editor - Configuration block.

    Author: Mickey W. Lawless
    Date:   03/01/26

    Configuration is stored as a single byte array to guarantee
    contiguous memory layout for TECF patching.
*/

#include "te.h"

/* ---------------------------------------------------------------
   Raw configuration data block - MUST be contiguous for TECF.
   TECF searches for "TE_CONF" and patches at fixed byte offsets.
   
   Layout (138 bytes total):
   Offset  Size  Field
   ------  ----  -----
      0      8   cf_ident "TE_CONF\0"
      8      1   cf_version (2)
      9      1   cf_pad (0)
     10      2   cf_mx_lines (low, high)
     12     32   cf_name
     44      1   cf_rows
     45      1   cf_cols
     46      1   cf_tab_cols
     47      1   cf_num
     48      1   cf_clang
     49      1   cf_indent
     50      1   cf_list
     51      8   cf_list_chr
     59      1   cf_rul_chr
     60      1   cf_rul_tab
     61      1   cf_vert_chr
     62      1   cf_horz_chr
     63      1   cf_lnum_chr
     64      8   cf_cr_name
     72      8   cf_esc_name
     80     29   cf_keys
    109     29   cf_keys_ex
   --------------------------------------------------------------- */

unsigned char cf_data[138] = {
    /* 0-7: cf_ident */
    'T', 'E', '_', 'C', 'O', 'N', 'F', '\0',
    /* 8: cf_version */
    2,
    /* 9: cf_pad */
    0,
    /* 10-11: cf_mx_lines = 512 (little-endian: 0x00, 0x02) */
    0x00, 0x02,
    /* 12-43: cf_name "MS-DOS" + padding */
    'M', 'S', '-', 'D', 'O', 'S', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 44: cf_rows */
    25,
    /* 45: cf_cols */
    80,
    /* 46: cf_tab_cols */
    4,
    /* 47: cf_num */
    4,
    /* 48: cf_clang */
    1,
    /* 49: cf_indent */
    1,
    /* 50: cf_list */
    1,
    /* 51-58: cf_list_chr "-*>" + padding */
    '-', '*', '>', 0, 0, 0, 0, 0,
    /* 59: cf_rul_chr */
    '.',
    /* 60: cf_rul_tab */
    '!',
    /* 61: cf_vert_chr */
    '|',
    /* 62: cf_horz_chr */
    '-',
    /* 63: cf_lnum_chr */
    '|',
    /* 64-71: cf_cr_name "Enter" + padding */
    'E', 'n', 't', 'e', 'r', 0, 0, 0,
    /* 72-79: cf_esc_name "Esc" + padding */
    'E', 's', 'c', 0, 0, 0, 0, 0,
    /* 80-108: cf_keys (29 bytes) */
    0,      /*  0  K_UP - extended */
    0,      /*  1  K_DOWN - extended */
    0,      /*  2  K_LEFT - extended */
    0,      /*  3  K_RIGHT - extended */
    0,      /*  4  K_BEGIN - extended */
    0,      /*  5  K_END - extended */
    20,     /*  6  K_TOP = Ctrl-T */
    2,      /*  7  K_BOTTOM = Ctrl-B */
    0,      /*  8  K_PGUP - extended */
    0,      /*  9  K_PGDOWN - extended */
    9,      /* 10  K_TAB = Ctrl-I */
    13,     /* 11  K_CR = Enter */
    27,     /* 12  K_ESC = Escape */
    0,      /* 13  K_RDEL - extended */
    8,      /* 14  K_LDEL = Backspace */
    24,     /* 15  K_CUT = Ctrl-X */
    3,      /* 16  K_COPY = Ctrl-C */
    22,     /* 17  K_PASTE = Ctrl-V */
    4,      /* 18  K_DELETE = Ctrl-D */
    12,     /* 19  K_CLRCLP = Ctrl-L */
    6,      /* 20  K_FIND = Ctrl-F */
    14,     /* 21  K_NEXT = Ctrl-N */
    7,      /* 22  K_GOTO = Ctrl-G */
    1,      /* 23  K_LWORD = Ctrl-A */
    5,      /* 24  K_RWORD = Ctrl-E */
    11,     /* 25  K_BLK_START = Ctrl-K */
    15,     /* 26  K_BLK_END = Ctrl-O */
    21,     /* 27  K_BLK_UNSET = Ctrl-U */
    13,     /* 28  K_MACRO = Ctrl-M */
    /* 109-137: cf_keys_ex (29 bytes) */
    72,     /*  0  K_UP = Up arrow */
    80,     /*  1  K_DOWN = Down arrow */
    75,     /*  2  K_LEFT = Left arrow */
    77,     /*  3  K_RIGHT = Right arrow */
    71,     /*  4  K_BEGIN = Home */
    79,     /*  5  K_END = End */
    0,      /*  6  K_TOP */
    0,      /*  7  K_BOTTOM */
    73,     /*  8  K_PGUP = Page Up */
    81,     /*  9  K_PGDOWN = Page Down */
    0,      /* 10  K_TAB */
    0,      /* 11  K_CR */
    0,      /* 12  K_ESC */
    83,     /* 13  K_RDEL = Delete */
    0,      /* 14  K_LDEL */
    0,      /* 15  K_CUT */
    0,      /* 16  K_COPY */
    0,      /* 17  K_PASTE */
    0,      /* 18  K_DELETE */
    0,      /* 19  K_CLRCLP */
    0,      /* 20  K_FIND */
    0,      /* 21  K_NEXT */
    0,      /* 22  K_GOTO */
    0,      /* 23  K_LWORD */
    0,      /* 24  K_RWORD */
    0,      /* 25  K_BLK_START */
    0,      /* 26  K_BLK_END */
    0,      /* 27  K_BLK_UNSET */
    0       /* 28  K_MACRO */
};

/* Accessor functions to read config values from cf_data */

int CfGetMxLines(void)
{
    return cf_data[10] | (cf_data[11] << 8);
}

unsigned char CfGetRows(void)
{
    return cf_data[44];
}

unsigned char CfGetCols(void)
{
    return cf_data[45];
}

unsigned char CfGetTabCols(void)
{
    return cf_data[46];
}

unsigned char CfGetNum(void)
{
    return cf_data[47];
}

unsigned char CfGetClang(void)
{
    return cf_data[48];
}

unsigned char CfGetIndent(void)
{
    return cf_data[49];
}

unsigned char CfGetList(void)
{
    return cf_data[50];
}

char *CfGetListChr(void)
{
    return (char *)&cf_data[51];
}

unsigned char CfGetRulChr(void)
{
    return cf_data[59];
}

unsigned char CfGetRulTab(void)
{
    return cf_data[60];
}

unsigned char CfGetVertChr(void)
{
    return cf_data[61];
}

unsigned char CfGetHorzChr(void)
{
    return cf_data[62];
}

unsigned char CfGetLnumChr(void)
{
    return cf_data[63];
}

char *CfGetCrName(void)
{
    return (char *)&cf_data[64];
}

char *CfGetEscName(void)
{
    return (char *)&cf_data[72];
}

char *CfGetName(void)
{
    return (char *)&cf_data[12];
}

unsigned char *CfGetKeys(void)
{
    return &cf_data[80];
}

unsigned char *CfGetKeysEx(void)
{
    return &cf_data[109];
}

/* Setter functions for auto-detect */
void CfSetRows(unsigned char v)
{
    cf_data[44] = v;
}

void CfSetCols(unsigned char v)
{
    cf_data[45] = v;
}

void CfSetIndent(unsigned char v)
{
    cf_data[49] = v;
}

void CfSetList(unsigned char v)
{
    cf_data[50] = v;
}

void CfSetTabCols(unsigned char v)
{
    cf_data[46] = v;
}

void CfSetNum(unsigned char v)
{
    cf_data[47] = v;
}

void CfSetClang(unsigned char v)
{
    cf_data[48] = v;
}

void CfSetRulChr(unsigned char v)
{
    cf_data[59] = v;
}

void CfSetRulTab(unsigned char v)
{
    cf_data[60] = v;
}

void CfSetVertChr(unsigned char v)
{
    cf_data[61] = v;
}

void CfSetHorzChr(unsigned char v)
{
    cf_data[62] = v;
}

void CfSetLnumChr(unsigned char v)
{
    cf_data[63] = v;
}

void CfSetListChr(char *s)
{
    int i;
    for (i = 0; i < 7 && s[i]; ++i)
        cf_data[51 + i] = s[i];
    cf_data[51 + i] = '\0';
}

/* ---------------------------------------------------------------
   Save/Load configuration to/from TE.CFG file
   --------------------------------------------------------------- */
int CfSave(void)
{
    FILE *fp;
    
    fp = fopen("TE-86.CFG", "wb");
    if (!fp)
        return -1;
    
    /* Write the entire config data block */
    if (fwrite(cf_data, 1, 138, fp) != 138) {
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    return 0;
}

int CfLoad(void)
{
    FILE *fp;
    unsigned char buf[138];
    
    fp = fopen("TE-86.CFG", "rb");
    if (!fp)
        return -1;
    
    /* Read and verify */
    if (fread(buf, 1, 138, fp) != 138) {
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    
    /* Verify the marker */
    if (buf[0] != 'T' || buf[1] != 'E' || buf[2] != '_' ||
        buf[3] != 'C' || buf[4] != 'O' || buf[5] != 'N' || buf[6] != 'F') {
        return -1;
    }
    
    /* Copy to active config (skip ident and version) */
    memcpy(cf_data + 10, buf + 10, 128);
    
    return 0;
}

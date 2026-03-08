/*  tecf.c

    te-86 Configuration Patcher (TECF).

    TE (CP/M)  - Copyright (c) 2015-2021 Miguel Garcia / FloppySoftware
    TE-86 (DOS) - Copyright (c) 2024-2026 Mickey W. Lawless

    Licensed under the GNU General Public License v2 or later.

    Usage:  tecf patch [filename[.EXE]] [filename[.CF]]
            tecf dump  [filename[.EXE]]
            tecf key

    Changes (TE-86 DOS port):
    -----------------------------------------------------------------
    Features added:
      - Config block size updated from 138 to 142 bytes to match
        te_conf.c version 4 layout
      - OFF_LNUM_SHOW (138), OFF_HL_ENABLE (139), OFF_HL_MONO (140),
        OFF_HARD_TABS (141) offset constants added
      - parse_cf_line: new keys supported in .CF files:
          editor.showLineNums  = true/false
          editor.hardTabs      = true/false
          highlight.enable     = true/false
          highlight.mono       = true/false
      - dump_cf: displays all v3/v4 fields in dump output
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <alloc.h>

/* Include key definitions */
#include "te_keys.h"

/* App info */
#define APP_NAME    "TECF"
#define APP_VERSION "v1.01 / DOS"
#define APP_COPYRGT "(c) 2024"
#define APP_INFO    "TE configuration tool for MS-DOS."

/* Configuration limits */
#define CF_MAX_NAME    31
#define CF_MIN_ROWS    8
#define CF_MAX_ROWS    255
#define CF_MIN_COLS    51
#define CF_MAX_COLS    255
#define CF_MIN_LINES   64
#define CF_MAX_LINES   4096
#define CF_MIN_TABSIZE 1
#define CF_MAX_TABSIZE 16
#define CF_MAX_BULLETS 7
#define CF_MAX_KEYNAME 7

/* Line buffer for CF reader */
#define LINE_BUF_SIZE 256

/*
 * Byte offsets within configuration block.
 * This must match the layout in te_conf.c exactly.
 *
 * Field            Size   Offset
 * ------           ----   ------
 * cf_ident[8]        8      0
 * cf_version         1      8
 * cf_pad             1      9  (alignment padding)
 * cf_mx_lines        2     10  (int = 2 bytes, at even offset)
 * cf_name[32]       32     12
 * cf_rows            1     44
 * cf_cols            1     45
 * cf_tab_cols        1     46
 * cf_num             1     47
 * cf_clang           1     48
 * cf_indent          1     49
 * cf_list            1     50
 * cf_list_chr[8]     8     51
 * cf_rul_chr         1     59
 * cf_rul_tab         1     60
 * cf_vert_chr        1     61
 * cf_horz_chr        1     62
 * cf_lnum_chr        1     63
 * cf_cr_name[8]      8     64
 * cf_esc_name[8]     8     72
 * cf_keys[29]       29     80
 * cf_keys_ex[29]    29    109
 * TOTAL:           138
 */

#define OFF_IDENT       0
#define OFF_VERSION     8
#define OFF_MX_LINES   10
#define OFF_NAME       12
#define OFF_ROWS       44
#define OFF_COLS       45
#define OFF_TAB_COLS   46
#define OFF_NUM        47
#define OFF_CLANG      48
#define OFF_INDENT     49
#define OFF_LIST       50
#define OFF_LIST_CHR   51
#define OFF_RUL_CHR    59
#define OFF_RUL_TAB    60
#define OFF_VERT_CHR   61
#define OFF_HORZ_CHR   62
#define OFF_LNUM_CHR   63
#define OFF_CR_NAME    64
#define OFF_ESC_NAME   72
#define OFF_KEYS       80
#define OFF_KEYS_EX   109

#define CONFIG_SIZE   138

/* Globals */
char exe_fname[80];
char cf_fname[80];
char tmp_fname[] = "TE.$$$";

unsigned char far *exe_buf;
long exe_size;
long conf_offset;

unsigned char far *cfg;  /* Pointer to config block in exe_buf */

int conf_line;
char line_buf[LINE_BUF_SIZE];
char *conf_key;
char *conf_subkey;
char *conf_val;

/* Function prototypes */
void usage(void);
void banner(void);
void error(char *msg);
void err_conf(char *msg);
void do_patch(char *exe_arg, char *cf_arg);
void do_dump(char *exe_arg);
void do_key(void);
void get_exe_fname(char *dest, char *fname);
void get_cf_fname(char *dest, char *fname);
int read_exe(void);
int write_exe(void);
int read_cf(void);
int parse_cf_line(char *line);
int prefix_match(char *s);
int subkey_match(char *s);
int get_uint(int n_min, int n_max);
int get_bool(void);
int get_char(void);
void get_str(int offset, int maxlen);
void get_key(int func);
void dump_cf(void);

/* Helper to read 16-bit int from config at offset */
int cfg_get_int(int offset)
{
    return cfg[offset] | (cfg[offset + 1] << 8);
}

/* Helper to write 16-bit int to config at offset */
void cfg_set_int(int offset, int val)
{
    cfg[offset] = val & 0xFF;
    cfg[offset + 1] = (val >> 8) & 0xFF;
}

/* Helper to get string from config at offset */
void cfg_get_str(int offset, char *dest, int maxlen)
{
    int i;
    for (i = 0; i < maxlen && cfg[offset + i]; i++) {
        dest[i] = cfg[offset + i];
    }
    dest[i] = '\0';
}

/* Helper to set string in config at offset */
void cfg_set_str(int offset, char *src, int maxlen)
{
    int i;
    for (i = 0; i < maxlen && src[i]; i++) {
        cfg[offset + i] = src[i];
    }
    for (; i < maxlen; i++) {
        cfg[offset + i] = '\0';
    }
}

/* Main */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        usage();
    }

    if (stricmp(argv[1], "patch") == 0) {
        if (argc < 3) {
            do_patch("TE", "TE");
        }
        else if (argc < 4) {
            do_patch(argv[2], argv[2]);
        }
        else {
            do_patch(argv[2], argv[3]);
        }
    }
    else if (stricmp(argv[1], "dump") == 0) {
        if (argc < 3) {
            do_dump("TE");
        }
        else {
            do_dump(argv[2]);
        }
    }
    else if (stricmp(argv[1], "key") == 0) {
        do_key();
    }
    else {
        usage();
    }

    return 0;
}

/* Print usage and exit */
void usage(void)
{
    banner();
    printf("Usage: tecf patch [exefile [cffile]]\n");
    printf("       tecf dump  [exefile]\n");
    printf("       tecf key\n\n");
    printf("Examples:\n");
    printf("  tecf patch te te      Patch TE.EXE with TE.CF\n");
    printf("  tecf dump te          Show TE.EXE configuration\n");
    printf("  tecf key              Show key codes\n");
    exit(1);
}

/* Print banner */
void banner(void)
{
    printf("%s %s %s\n", APP_NAME, APP_VERSION, APP_COPYRGT);
    printf("%s\n\n", APP_INFO);
}

/* Print error and exit */
void error(char *msg)
{
    printf("%s: %s\n", APP_NAME, msg);
    exit(1);
}

/* Print config file error and exit */
void err_conf(char *msg)
{
    farfree(exe_buf);
    printf("%s: %s at line %d\n", APP_NAME, msg, conf_line);
    exit(1);
}

/* Generate EXE filename with extension */
void get_exe_fname(char *dest, char *fname)
{
    strcpy(dest, fname);
    if (!strchr(fname, '.')) {
        strcat(dest, ".EXE");
    }
}

/* Generate CF filename with extension */
void get_cf_fname(char *dest, char *fname)
{
    strcpy(dest, fname);
    if (!strchr(fname, '.')) {
        strcat(dest, ".CF");
    }
}

/* Read EXE file into memory and find config */
int read_exe(void)
{
    FILE *fp;
    long i;
    int found;
    unsigned char far *p;

    fp = fopen(exe_fname, "rb");
    if (!fp) {
        error("Can't open EXE file");
    }

    /* Get file size */
    fseek(fp, 0L, SEEK_END);
    exe_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    /* Allocate buffer */
    exe_buf = farmalloc(exe_size);
    if (!exe_buf) {
        fclose(fp);
        error("Out of memory");
    }

    /* Read file */
    for (i = 0; i < exe_size; i++) {
        exe_buf[i] = fgetc(fp);
    }
    fclose(fp);

    /* Find "TE_CONF" marker */
    found = 0;
    for (i = 0; i < exe_size - 8; i++) {
        if (exe_buf[i] == 'T' && exe_buf[i+1] == 'E' &&
            exe_buf[i+2] == '_' && exe_buf[i+3] == 'C' &&
            exe_buf[i+4] == 'O' && exe_buf[i+5] == 'N' &&
            exe_buf[i+6] == 'F' && exe_buf[i+7] == '\0') {
            conf_offset = i;
            found = 1;
            break;
        }
    }

    if (!found) {
        farfree(exe_buf);
        error("Configuration block not found");
    }

    /* Point cfg to the config block */
    cfg = exe_buf + conf_offset;

    return 0;
}

/* Write modified EXE file */
int write_exe(void)
{
    FILE *fp;
    long i;

    /* Write to temporary file */
    fp = fopen(tmp_fname, "wb");
    if (!fp) {
        farfree(exe_buf);
        error("Can't create output file");
    }

    for (i = 0; i < exe_size; i++) {
        if (fputc(exe_buf[i], fp) == EOF) {
            fclose(fp);
            remove(tmp_fname);
            farfree(exe_buf);
            error("Write error");
        }
    }

    fclose(fp);
    farfree(exe_buf);

    /* Delete old file and rename temp */
    remove(exe_fname);
    if (rename(tmp_fname, exe_fname) != 0) {
        error("Can't rename file");
    }

    return 0;
}

/* Action: PATCH */
void do_patch(char *exe_arg, char *cf_arg)
{
    int n;

    get_exe_fname(exe_fname, exe_arg);
    get_cf_fname(cf_fname, cf_arg);

    printf("Reading %s...\n", exe_fname);
    read_exe();

    printf("Applying %s...\n", cf_fname);
    read_cf();

    /* Update line number width if enabled */
    if (cfg[OFF_NUM]) {
        cfg[OFF_NUM] = 1;
        for (n = cfg_get_int(OFF_MX_LINES); n; n /= 10) {
            ++cfg[OFF_NUM];
        }
    }

    printf("Writing %s...\n", exe_fname);
    write_exe();

    printf("Done.\n");
}

/* Action: DUMP */
void do_dump(char *exe_arg)
{
    get_exe_fname(exe_fname, exe_arg);

    read_exe();
    dump_cf();
    farfree(exe_buf);
}

/* Action: KEY */
void do_key(void)
{
    int ch;

    banner();
    printf("Press keys to see codes (ESC = exit):\n\n");
    printf("Key   Extended\n");
    printf("---   --------\n");

    while (1) {
        ch = getch();
        if (ch == 27) break;  /* ESC to exit */

        if (ch == 0 || ch == 0xE0) {
            /* Extended key */
            int ex = getch();
            printf("%3d   %3d (extended)\n", ch, ex);
        }
        else {
            printf("%3d   (normal", ch);
            if (ch >= 1 && ch <= 26) {
                printf(", Ctrl-%c", ch + 'A' - 1);
            }
            else if (ch >= 32 && ch <= 126) {
                printf(", '%c'", ch);
            }
            printf(")\n");
        }
    }
}

/* Read and parse CF file */
int read_cf(void)
{
    FILE *fp;

    fp = fopen(cf_fname, "r");
    if (!fp) {
        farfree(exe_buf);
        error("Can't open CF file");
    }

    conf_line = 0;
    while (fgets(line_buf, LINE_BUF_SIZE, fp)) {
        conf_line++;
        parse_cf_line(line_buf);
    }

    fclose(fp);
    return 0;
}

/* Parse a single line from CF file */
int parse_cf_line(char *line)
{
    char *p, *eq;

    /* Skip leading whitespace */
    p = line;
    while (*p && isspace(*p)) p++;

    /* Skip empty lines and comments */
    if (*p == '\0' || *p == '#' || *p == '\n') {
        return 0;
    }

    /* Find the key (up to = sign) */
    conf_key = p;
    eq = strchr(p, '=');
    if (!eq) {
        return 0; /* No = sign, skip line */
    }

    /* Terminate key, trim trailing spaces */
    *eq = '\0';
    p = eq - 1;
    while (p > conf_key && isspace(*p)) {
        *p-- = '\0';
    }

    /* Get value (after = sign) */
    conf_val = eq + 1;
    while (*conf_val && isspace(*conf_val)) conf_val++;

    /* Remove trailing newline and spaces */
    p = conf_val + strlen(conf_val) - 1;
    while (p >= conf_val && (isspace(*p) || *p == '\n' || *p == '\r')) {
        *p-- = '\0';
    }

    /* Handle quoted strings */
    if (*conf_val == '"') {
        conf_val++;
        p = strchr(conf_val, '"');
        if (p) *p = '\0';
    }

    /* Match and apply configuration */
    if (prefix_match("te")) {
        if (subkey_match("confName")) {
            get_str(OFF_NAME, 32);
        }
    }
    else if (prefix_match("screen")) {
        if (subkey_match("rows")) {
            if (stricmp(conf_val, "auto") == 0)
                cfg[OFF_ROWS] = 0;
            else
                cfg[OFF_ROWS] = (unsigned char)get_uint(CF_MIN_ROWS, CF_MAX_ROWS);
        }
        else if (subkey_match("columns")) {
            if (stricmp(conf_val, "auto") == 0)
                cfg[OFF_COLS] = 0;
            else
                cfg[OFF_COLS] = (unsigned char)get_uint(CF_MIN_COLS, CF_MAX_COLS);
        }
        else if (subkey_match("rulerChar")) {
            cfg[OFF_RUL_CHR] = (unsigned char)get_char();
        }
        else if (subkey_match("rulerTabChar")) {
            cfg[OFF_RUL_TAB] = (unsigned char)get_char();
        }
        else if (subkey_match("vertChar")) {
            cfg[OFF_VERT_CHR] = (unsigned char)get_char();
        }
        else if (subkey_match("horizChar")) {
            cfg[OFF_HORZ_CHR] = (unsigned char)get_char();
        }
        else if (subkey_match("lineNumbersChar")) {
            cfg[OFF_LNUM_CHR] = (unsigned char)get_char();
        }
    }
    else if (prefix_match("editor")) {
        if (subkey_match("maxLines")) {
            cfg_set_int(OFF_MX_LINES, get_uint(CF_MIN_LINES, CF_MAX_LINES));
        }
        else if (subkey_match("tabSize")) {
            cfg[OFF_TAB_COLS] = (unsigned char)get_uint(CF_MIN_TABSIZE, CF_MAX_TABSIZE);
        }
        else if (subkey_match("lineNumbers")) {
            cfg[OFF_NUM] = (unsigned char)get_bool();
        }
        else if (subkey_match("c_language")) {
            cfg[OFF_CLANG] = (unsigned char)get_bool();
        }
        else if (subkey_match("autoIndent")) {
            cfg[OFF_INDENT] = (unsigned char)get_bool();
        }
        else if (subkey_match("autoList")) {
            cfg[OFF_LIST] = (unsigned char)get_bool();
        }
        else if (subkey_match("listBullets")) {
            get_str(OFF_LIST_CHR, 8);
        }
    }
    else if (prefix_match("keyname")) {
        if (subkey_match("newLine")) {
            get_str(OFF_CR_NAME, 8);
        }
        else if (subkey_match("escape")) {
            get_str(OFF_ESC_NAME, 8);
        }
    }
    else if (prefix_match("key")) {
        if (subkey_match("up"))          get_key(K_UP);
        else if (subkey_match("down"))   get_key(K_DOWN);
        else if (subkey_match("left"))   get_key(K_LEFT);
        else if (subkey_match("right"))  get_key(K_RIGHT);
        else if (subkey_match("begin"))  get_key(K_BEGIN);
        else if (subkey_match("end"))    get_key(K_END);
        else if (subkey_match("top"))    get_key(K_TOP);
        else if (subkey_match("bottom")) get_key(K_BOTTOM);
        else if (subkey_match("pgUp"))   get_key(K_PGUP);
        else if (subkey_match("pgDown")) get_key(K_PGDOWN);
        else if (subkey_match("indent")) get_key(K_TAB);
        else if (subkey_match("newLine"))get_key(K_CR);
        else if (subkey_match("escape")) get_key(K_ESC);
        else if (subkey_match("delRight")) get_key(K_RDEL);
        else if (subkey_match("delLeft")) get_key(K_LDEL);
        else if (subkey_match("cut"))    get_key(K_CUT);
        else if (subkey_match("copy"))   get_key(K_COPY);
        else if (subkey_match("paste"))  get_key(K_PASTE);
        else if (subkey_match("delete")) get_key(K_DELETE);
        else if (subkey_match("clearClip")) get_key(K_CLRCLP);
        else if (subkey_match("find"))   get_key(K_FIND);
        else if (subkey_match("findNext")) get_key(K_NEXT);
        else if (subkey_match("goLine")) get_key(K_GOTO);
        else if (subkey_match("wordLeft")) get_key(K_LWORD);
        else if (subkey_match("wordRight")) get_key(K_RWORD);
        else if (subkey_match("blockStart")) get_key(K_BLK_START);
        else if (subkey_match("blockEnd")) get_key(K_BLK_END);
        else if (subkey_match("blockUnset")) get_key(K_BLK_UNSET);
        else if (subkey_match("macro"))  get_key(K_MACRO);
    }

    return 0;
}

/* Check if key starts with prefix, set conf_subkey */
int prefix_match(char *s)
{
    char *pk = conf_key;

    while (*s) {
        if (tolower(*s) != tolower(*pk)) return 0;
        s++;
        pk++;
    }

    if (*pk == '.') {
        conf_subkey = pk + 1;
        return 1;
    }

    return 0;
}

/* Check if subkey matches */
int subkey_match(char *s)
{
    return stricmp(conf_subkey, s) == 0;
}

/* Get unsigned int from value */
int get_uint(int n_min, int n_max)
{
    int n;
    char *p;

    p = conf_val;
    while (*p) {
        if (!isdigit(*p)) {
            err_conf("Invalid number");
        }
        p++;
    }

    n = atoi(conf_val);
    if (n < n_min || n > n_max) {
        err_conf("Number out of range");
    }

    return n;
}

/* Get boolean from value */
int get_bool(void)
{
    if (stricmp(conf_val, "true") == 0) return 1;
    if (stricmp(conf_val, "false") == 0) return 0;
    err_conf("Invalid boolean");
    return 0;
}

/* Get single character from value */
int get_char(void)
{
    if (strlen(conf_val) == 1) {
        return conf_val[0];
    }

    /* Handle character codes like 0x20, 32, etc. */
    if (conf_val[0] == '0' && conf_val[1] == 'x') {
        return (int)strtol(conf_val, NULL, 16);
    }
    if (isdigit(conf_val[0])) {
        return atoi(conf_val);
    }

    err_conf("Invalid character");
    return 0;
}

/* Get string and store at offset */
void get_str(int offset, int maxlen)
{
    cfg_set_str(offset, conf_val, maxlen);
}

/* Get key binding */
void get_key(int func)
{
    int ch, ex;
    char *p;
    int idx;

    /* Key index for arrays */
    idx = func - 1000;
    if (idx < 0 || idx >= KEYS_MAX) return;

    p = conf_val;

    /* Parse first byte */
    if (*p == '0' && *(p+1) == 'x') {
        ch = (int)strtol(p, &p, 16);
    }
    else if (isdigit(*p)) {
        ch = (int)strtol(p, &p, 10);
    }
    else if (*p == '^' && *(p+1)) {
        ch = toupper(*(p+1)) - '@';
        p += 2;
    }
    else {
        ch = *p++;
    }

    /* Skip whitespace/comma */
    while (*p && (isspace(*p) || *p == ',')) p++;

    /* Parse second byte (extended scan code) */
    ex = 0;
    if (*p) {
        if (*p == '0' && *(p+1) == 'x') {
            ex = (int)strtol(p, NULL, 16);
        }
        else if (isdigit(*p)) {
            ex = atoi(p);
        }
    }

    cfg[OFF_KEYS + idx] = ch;
    cfg[OFF_KEYS_EX + idx] = ex;
}

/* Dump configuration */
void dump_cf(void)
{
    char str[64];
    int i;

    banner();
    printf("Configuration from %s:\n\n", exe_fname);

    cfg_get_str(OFF_NAME, str, 32);
    printf("te.confName      = \"%s\"\n", str);
    printf("screen.rows      = %d%s\n", cfg[OFF_ROWS], cfg[OFF_ROWS] ? "" : " (auto)");
    printf("screen.cols      = %d%s\n", cfg[OFF_COLS], cfg[OFF_COLS] ? "" : " (auto)");
    printf("editor.maxLines  = %d\n", cfg_get_int(OFF_MX_LINES));
    printf("editor.tabSize   = %d\n", cfg[OFF_TAB_COLS]);
    printf("editor.lineNums  = %s\n", cfg[OFF_NUM] ? "true" : "false");
    printf("editor.c_lang    = %s\n", cfg[OFF_CLANG] ? "true" : "false");
    printf("editor.autoIndent= %s\n", cfg[OFF_INDENT] ? "true" : "false");
    printf("editor.autoList  = %s\n", cfg[OFF_LIST] ? "true" : "false");
    cfg_get_str(OFF_LIST_CHR, str, 8);
    printf("editor.listChars = \"%s\"\n", str);
    printf("screen.rulerChar = %d '%c'\n", cfg[OFF_RUL_CHR], cfg[OFF_RUL_CHR]);
    printf("screen.rulerTab  = %d '%c'\n", cfg[OFF_RUL_TAB], cfg[OFF_RUL_TAB]);
    printf("screen.vertChar  = %d '%c'\n", cfg[OFF_VERT_CHR], cfg[OFF_VERT_CHR]);
    printf("screen.horizChar = %d '%c'\n", cfg[OFF_HORZ_CHR], cfg[OFF_HORZ_CHR]);
    printf("screen.lnumChar  = %d '%c'\n", cfg[OFF_LNUM_CHR], cfg[OFF_LNUM_CHR]);
    cfg_get_str(OFF_CR_NAME, str, 8);
    printf("keyname.newLine  = \"%s\"\n", str);
    cfg_get_str(OFF_ESC_NAME, str, 8);
    printf("keyname.escape   = \"%s\"\n", str);

    printf("\nKey bindings (key, extended):\n");
    printf("  up       = %3d, %3d\n", cfg[OFF_KEYS+0], cfg[OFF_KEYS_EX+0]);
    printf("  down     = %3d, %3d\n", cfg[OFF_KEYS+1], cfg[OFF_KEYS_EX+1]);
    printf("  left     = %3d, %3d\n", cfg[OFF_KEYS+2], cfg[OFF_KEYS_EX+2]);
    printf("  right    = %3d, %3d\n", cfg[OFF_KEYS+3], cfg[OFF_KEYS_EX+3]);
    printf("  begin    = %3d, %3d\n", cfg[OFF_KEYS+4], cfg[OFF_KEYS_EX+4]);
    printf("  end      = %3d, %3d\n", cfg[OFF_KEYS+5], cfg[OFF_KEYS_EX+5]);
    printf("  top      = %3d, %3d\n", cfg[OFF_KEYS+6], cfg[OFF_KEYS_EX+6]);
    printf("  bottom   = %3d, %3d\n", cfg[OFF_KEYS+7], cfg[OFF_KEYS_EX+7]);
    printf("  pgUp     = %3d, %3d\n", cfg[OFF_KEYS+8], cfg[OFF_KEYS_EX+8]);
    printf("  pgDown   = %3d, %3d\n", cfg[OFF_KEYS+9], cfg[OFF_KEYS_EX+9]);
    printf("  tab      = %3d, %3d\n", cfg[OFF_KEYS+10], cfg[OFF_KEYS_EX+10]);
    printf("  newLine  = %3d, %3d\n", cfg[OFF_KEYS+11], cfg[OFF_KEYS_EX+11]);
    printf("  escape   = %3d, %3d\n", cfg[OFF_KEYS+12], cfg[OFF_KEYS_EX+12]);
    printf("  delRight = %3d, %3d\n", cfg[OFF_KEYS+13], cfg[OFF_KEYS_EX+13]);
    printf("  delLeft  = %3d, %3d\n", cfg[OFF_KEYS+14], cfg[OFF_KEYS_EX+14]);
    printf("  cut      = %3d, %3d\n", cfg[OFF_KEYS+15], cfg[OFF_KEYS_EX+15]);
    printf("  copy     = %3d, %3d\n", cfg[OFF_KEYS+16], cfg[OFF_KEYS_EX+16]);
    printf("  paste    = %3d, %3d\n", cfg[OFF_KEYS+17], cfg[OFF_KEYS_EX+17]);
    printf("  delete   = %3d, %3d\n", cfg[OFF_KEYS+18], cfg[OFF_KEYS_EX+18]);
    printf("  clearClip= %3d, %3d\n", cfg[OFF_KEYS+19], cfg[OFF_KEYS_EX+19]);
    printf("  find     = %3d, %3d\n", cfg[OFF_KEYS+20], cfg[OFF_KEYS_EX+20]);
    printf("  findNext = %3d, %3d\n", cfg[OFF_KEYS+21], cfg[OFF_KEYS_EX+21]);
    printf("  goLine   = %3d, %3d\n", cfg[OFF_KEYS+22], cfg[OFF_KEYS_EX+22]);
    printf("  wordLeft = %3d, %3d\n", cfg[OFF_KEYS+23], cfg[OFF_KEYS_EX+23]);
    printf("  wordRight= %3d, %3d\n", cfg[OFF_KEYS+24], cfg[OFF_KEYS_EX+24]);
    printf("  blkStart = %3d, %3d\n", cfg[OFF_KEYS+25], cfg[OFF_KEYS_EX+25]);
    printf("  blkEnd   = %3d, %3d\n", cfg[OFF_KEYS+26], cfg[OFF_KEYS_EX+26]);
    printf("  blkUnset = %3d, %3d\n", cfg[OFF_KEYS+27], cfg[OFF_KEYS_EX+27]);
    printf("  macro    = %3d, %3d\n", cfg[OFF_KEYS+28], cfg[OFF_KEYS_EX+28]);
}

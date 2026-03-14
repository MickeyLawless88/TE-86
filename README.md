# TE-86 — MS-DOS Text Editor

**TE-86** is a port of Miguel Garcia's [TE text editor for CP/M](https://github.com/floppysoftware/te)
to MS-DOS, compiled with Borland Turbo C 2.0 (small memory model).

Original CP/M version © 2015–2021 Miguel Garcia / FloppySoftware  
DOS port © 2024–2026 Mickey W. Lawless

Licensed under the GNU General Public License v2 or later.

---

## Usage

```
te-86 [filename]
```

If no filename is given, the editor starts with an empty buffer.  
On exit you will be prompted to save if unsaved changes exist.

---

## Key Bindings (defaults)

| Key | Action |
|-----|--------|
| Arrow keys | Move cursor |
| Home / End | Beginning / end of line |
| Ctrl-T | Top of file |
| Ctrl-B | Bottom of file |
| PgUp / PgDn | Page up / page down |
| Enter | New line (splits at cursor) |
| Backspace | Delete left |
| Del | Delete right |
| Tab | Indent (expand to spaces, or insert hard tab) |
| Ctrl-F | Find |
| Ctrl-N | Find next |
| Ctrl-G | Go to line number |
| Ctrl-A | Word left |
| Ctrl-E | Word right |
| Ctrl-X | Cut line |
| Ctrl-C | Copy line |
| Ctrl-V | Paste line |
| Ctrl-D | Delete line |
| Ctrl-L | Clear clipboard |
| Ctrl-K | Block start |
| Ctrl-O | Block end |
| Ctrl-U | Block unset |
| Ctrl-M | Run macro |
| Esc | Main menu |

Key bindings for Ctrl sequences and extended keys (arrows, Home, End,
PgUp, PgDn, Del) are stored in the configuration block and can be
patched with the TECF utility.

---

## Configuration File

TE-86 reads and writes `TE-86.CFG` in the current directory on startup
and when "Save" is selected from the Configuration menu (Esc → Config → S).

If no `.CFG` file is present, compiled-in defaults are used.  The file
is a raw binary block — use the TECF patcher to edit it externally, or
use the in-editor Configuration menu.

---

## New Features (DOS Port)

### Mouse Support

TE-86 supports any INT 33h-compatible mouse driver.

- **Left-click** moves the cursor to the clicked position.
- **Right-click** pastes the clipboard at the clicked position.
- Mouse support is detected automatically at startup; the editor
  operates normally without a mouse.

INT 33h functions used: `AX=0000` (reset/detect), `AX=0001` (show),
`AX=0002` (hide), `AX=0003` (get position/buttons), `AX=0004` (set position).

---

### Syntax Highlighting (`OPT_HILIGHT`)

Syntax highlighting is applied automatically based on file extension.
The language name is shown right-justified on the system line while editing.

**Supported languages and extensions:**

| Language | Extensions |
|----------|-----------|
| C / C++ | `.C` `.H` `.CPP` `.CC` |
| Assembly (x86) | `.ASM` `.S` |
| BASIC | `.BAS` |
| Pascal | `.PAS` |
| FORTRAN 77/90 | `.F` `.FOR` `.F77` `.F90` |
| PL/I | `.PLI` `.PL1` |
| COBOL | `.COB` `.CBL` |
| PL/M | `.PLM` |

**Colour attributes (CGA/EGA/VGA):**

| Token type | Colour |
|------------|--------|
| Normal text | Light grey on black |
| Keywords | Bright cyan on black |
| Strings / char literals | Bright yellow on black |
| Comments | Bright green on black |
| Numbers | Bright magenta on black |
| Preprocessor | Bright blue on black |
| Operators | Bright red on black |

**Monochrome attributes (MDA/Hercules):**

MDA hardware supports four reliable attribute states. Token categories
are collapsed to three visible states:

| Token type | Attribute |
|------------|-----------|
| Keywords, preprocessor | Bright/bold |
| Strings / char literals | Bright + underline |
| Comments | Underline only |
| Everything else | Normal |

**Multi-line block comments** are tracked across lines via the
`hl_in_comment` global; comment colour correctly continues onto
continuation lines without re-scanning from the top of the file.

Highlighting can be toggled on/off and between colour and monochrome
from the Configuration menu (options `8` and `9`).

---

### Hard Tab Support

When hard tabs are enabled (`cf_hard_tabs = 1`), ASCII `0x09` tab
characters are preserved in the line buffer rather than expanded to
spaces. Files containing hard tabs round-trip through load and save
unchanged.

When hard tabs are disabled (default), the Tab key expands to spaces
up to the next tab stop, and tab characters in loaded files are
converted to spaces (with a one-time status bar warning).

Toggle from the Configuration menu (option `E`).

---

### Horizontal Scrolling

Lines longer than the visible editing area can be viewed and edited.
The view scrolls automatically as the cursor moves past the right or
left edge. The horizontal scroll offset (`box_hsc`) is stored globally
and reset appropriately on navigation, line splits, and file load.

The status bar Col display always shows the logical column within the
line buffer, not the screen column.

---

### Line Number Display

Line numbers are shown in a configurable-width gutter at the left edge
of the editor box. The gutter width is set by `cf_num` (default: 4,
giving room for 3-digit line numbers plus a separator character).

Line numbers can be toggled on/off from the Configuration menu
(option `3`). When hidden, `cf_gutter` collapses to zero and the ruler
and cursor positioning both adjust accordingly.

---

### Ruler Enhancements

The ruler line (shown above the editor box) supports several
language-specific column markers configurable from the Configuration
menu:

| Option | Description |
|--------|-------------|
| `F` | FORTRAN continuation column indicator (col 6) — on/off + character |
| `G` | FORTRAN statement column indicator (col 7) — on/off + character |
| `H` | COBOL comment/indicator column (col 7) — on/off + character |
| `I` | Tab-stop line numbers — show two-digit tab ordinal at each tab stop |

---

### File Browser

When opening a file (Esc → Open), a semi-graphical file browser is
presented first. It shows the current directory contents in a
scrollable 3-column dialog:

- Directories are shown as `[NAME]` and sorted before files.
- Arrow keys and the mouse navigate the listing.
- Enter selects a file or enters a directory.
- Esc from the browser falls back to the original typed-filename prompt.

---

### Direct Video RAM Output

All character output uses direct far-pointer writes to video RAM
(`VidPoke`) rather than BIOS TTY output (`INT 10h AH=0Eh`).

- **Video segment auto-detected** at startup: BIOS data area byte
  `0040:0049` is read; mode 7 (MDA 80×25) maps to segment `0xB000`,
  all other modes to `0xB800`.
- `CrtSetAttr()` stores the attribute in `crt_attr`; all subsequent
  `CrtOut()` calls use it directly in the video RAM write.
- Characters that would write past the last column are silently
  dropped, preventing terminal auto-wrap from corrupting the separator
  and system lines.

---

### Configuration Block Versioning

The binary configuration block (`TE_CONF`) has been extended across
three new versions while remaining backward compatible with older
`.CFG` files:

| Version | Size | New fields |
|---------|------|-----------|
| v2 (original) | 138 bytes | — |
| v3 | 141 bytes | `cf_lnum_show`, `cf_hl_enable`, `cf_hl_mono` |
| v4 | 142 bytes | `cf_hard_tabs` |
| v5 | 149 bytes | `cf_fort_cont_en/chr`, `cf_fort_stmt_en/chr`, `cf_cobol_ind_en/chr`, `cf_tab_line_num` |

`CfLoad()` reads only as many fields as the file contains; fields
absent from older files retain their compiled-in defaults.

---

## Bug Fixes (DOS Port)

### `te_crt.c`
- `putch()` (BIOS TTY write, INT 10h AH=0Eh) ignores the current text
  attribute entirely. Replaced with direct video RAM writes so
  `CrtSetAttr()` takes effect correctly on all adapter types.
- Syntax highlight colour changes now work on MDA/Hercules, CGA, EGA,
  and VGA.

### `te_edit.c`
- **Line redraw** now calls `ModifyLine` + `Refresh` — the same
  proven draw path used everywhere else in the editor — instead of a
  hand-rolled partial redraw. This eliminates ghost characters,
  duplicate text, and wrong-column writes that affected typing,
  backspace, and delete.
- Cursor column positioning uses `cf_gutter` so it tracks correctly
  when line numbers are hidden.
- `putchr('\b')` removed from the backspace handler (it fought with
  the line redraw and corrupted the display).
- Redundant `putchr(ch)` removed from the character insert path.

### `te_file.c`
- **"Line too long" no longer aborts file load.** Lines wider than
  `LN_MAX_HARD` (255 chars) are silently truncated; excess characters
  are drained with `fgetc()` so subsequent `fgets()` calls resume at
  the correct file position. A one-time warning "Long line(s)
  truncated" is shown.
- Tab-conversion and illegal-character warnings are tracked with
  independent counters so both messages are reported correctly.

### `te_lines.c`
- **`SplitLine` pointer aliasing bug fixed.** The original code passed
  `lp_arr[line] + pos` directly to `AppendLine`. `AppendLine` →
  `SetLine` shifts the `lp_arr[]` array so `lp_arr[line+1]` ends up
  pointing at the original buffer; then truncating that buffer at `pos`
  corrupted the new line, leaving it with length 0 or garbage content.
  The fix copies the right portion into a fresh allocation before
  calling `AppendLine`.

### `te_loop.c`
- **Enter key now splits at the cursor**, not always at end-of-line.
  `split_pos` is set from `box_shc` (clamped to line length) rather
  than `strlen(lp_arr[lp_cur])`.
- **Auto-indent only applies to end-of-line splits.** When Enter is
  pressed mid-line, the text after the cursor already carries its own
  indentation; prepending the auto-indent prefix would double-indent it.
- **Horizontal scroll (`box_hsc`) is always reset to 0** after a line
  split so the new line is displayed from column 0 regardless of the
  scroll position of the line above.
- Cursor column positioning uses `cf_gutter` throughout.

### `te_main.c`
- `ln_max` is now always `LN_MAX_HARD` (255), decoupled from screen
  width. Previously it was set to `cf_cols - cf_num - 1`, which caused
  false "line too long" errors when loading files with lines wider than
  the screen.
- Screen geometry (`cf_rows`, `cf_cols`) is always taken from
  `CrtGetSize()` hardware result after init, overriding any stale value
  stored in the `.CFG` file.

### `te_ui.c`
- ESC key in `MenuConfig`: `K_ESC` (value 1012) was passed to
  `toupper()` which truncates to `unsigned char` first, making
  `1012 → 244 → not K_ESC`. Fixed by testing `_k == K_ESC` before
  calling `toupper()`.
- Single-character file extensions (`.C`, `.H`) produced a garbled
  type tag `[C` with no closing bracket in the status bar. Fixed.
- Operator colour noise: `* / - :` were incorrectly painted as
  operators on declarations, negative literals, and struct member
  access. Removed from the operator set; only unambiguous operators
  remain: `+ = < > ! & | ^ ~ % ?`.
- Ruler position did not track the line-number toggle; `Layout()` now
  uses `cf_gutter` instead of `cf_num`.
- Multi-line block comments: only the opening line was highlighted in
  comment colour. Fixed by promoting `in_comment` from a local variable
  to the global `hl_in_comment`.

---

## Removed Features

### Context Menu (`CtxMenu`)
The right-click context menu (Copy/Cut/Paste popup) has been removed.
It was the source of display corruption bugs affecting the entire
editing session. Right-click now pastes the clipboard directly at the
clicked position. Ctrl+R is a no-op. Ctrl+C / Ctrl+X / Ctrl+V
continue to work as before.

---

## Building

Compile with Borland Turbo C 2.0, small memory model, targeting 8086:

```
tcc -ms -1- -O te_main.c te_loop.c te_edit.c te_file.c te_lines.c \
    te_misc.c te_keys.c te_crt.c te_ui.c te_mouse.c te_macro.c \
    te_error.c te_conf.c
```

Output: `te_main.exe` (rename to `te-86.exe` as preferred).

---

## File Reference

| File | Purpose |
|------|---------|
| `te.h` | Global definitions, config macros, prototypes |
| `te_keys.h` | Key code constants |
| `te_main.c` | Entry point, global variable definitions |
| `te_loop.c` | Main editor loop, key dispatch |
| `te_edit.c` | Line editor (`BfEdit`), forced input buffer |
| `te_file.c` | File load/save, backup |
| `te_lines.c` | Line buffer operations (insert, delete, split, join) |
| `te_misc.c` | Memory allocation, utility functions |
| `te_keys.c` | Key translation (`GetKey`) |
| `te_crt.c` | Console abstraction, video RAM output |
| `te_ui.c` | Screen layout, menus, syntax highlighting, file browser |
| `te_mouse.c` | INT 33h mouse driver interface |
| `te_macro.c` | Macro file execution |
| `te_error.c` | Error message helpers |
| `te_conf.c` | Configuration block, save/load |
| `TE-86.CFG` | Runtime configuration (binary, generated by editor) |

---

## Credits

Original TE for CP/M by **Miguel Garcia / FloppySoftware**  
`https://github.com/floppysoftware/te`

DOS port by **Mickey W. Lawless**  
`https://github.com/mickeylawless88/te-86`

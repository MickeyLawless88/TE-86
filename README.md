# TE-86 — Text Editor for MS-DOS

**TE-86** is a full-screen text editor for MS-DOS, compiled with Turbo C 2.0 or
Turbo C++ 1.0.  It is a DOS port of the **TE** editor originally written for
CP/M by Miguel Garcia / FloppySoftware, extended with syntax highlighting,
mouse support, a graphical file browser, a context menu, hard-tab support,
and numerous other improvements by Mickey W. Lawless.

---

## Table of Contents

1. [Requirements](#requirements)
2. [Building](#building)
3. [Usage](#usage)
4. [Screen Layout](#screen-layout)
5. [Keyboard Reference](#keyboard-reference)
6. [Mouse Support](#mouse-support)
7. [Block Selection](#block-selection)
8. [Context Menu](#context-menu)
9. [Clipboard](#clipboard)
10. [Find and Go To Line](#find-and-go-to-line)
11. [Macros](#macros)
12. [Syntax Highlighting](#syntax-highlighting)
13. [File Browser](#file-browser)
14. [Configuration](#configuration)
15. [Config File (TE-86.CFG)](#config-file-te-86cfg)
16. [TECF — Config Patcher](#tecf--config-patcher)
17. [Limitations](#limitations)
18. [License](#license)
19. [Changelog](#changelog)

---

## Requirements

- MS-DOS 3.3 or later (or DOSBox 0.74+)
- 8086-class CPU or better
- CGA, EGA, VGA, MDA, or Hercules display adapter
- Turbo C 2.0 or Turbo C++ 1.0 (to build from source)
- Optional: Microsoft-compatible INT 33h mouse driver (MOUSE.COM / MOUSE.SYS)

---

## Building

1. Edit `build.bat` and set `TC_DIR` to match your Turbo C installation
   (default: `C:\TC`).
2. Run:

```
build.bat
```

This compiles all source modules and links them into `te86.exe`.  Object files
are cleaned automatically at the start of each build.

The memory model is **small** (`-ms`).  If you need to edit very large files,
change `set MODEL=s` to `set MODEL=l` (large model) in `build.bat`.

---

## Usage

```
te86 [filename]
```

If a filename is supplied on the command line, that file is opened immediately.
If it does not exist, a new empty buffer is created with that name.  If no
filename is given, the editor starts with an empty unnamed buffer and the File
Browser opens so you can pick a file.

---

## Screen Layout

```
 filename.c          --- | Lin:0042/0168/6000 Col:05/76 Len:9
 :.........:.........:.........:.........:.........  (ruler)
 0042  int main(void) {                              (text area)
 0043      return 0;
 ...
 ──────────────────────────────────────────────────  (separator)
 Esc = menu                                C/C++     (status bar)
```

**Title / status line (row 0)**

| Field | Meaning |
|-------|---------|
| filename | Current file name (`(new)` if unsaved) |
| `---` / `CHG` | Unsaved-change indicator |
| `Lin:cur/total/max` | Current line / total lines / file line limit |
| `Col:cur/width` | Cursor column / visible editing width |
| `Len:n` | Length of the current line |

**Ruler (row 1)** — shows column positions; tab stops are marked with the
ruler-tab character (default `!`).

**Text area** — the main editing region.  Line numbers are shown in the left
gutter when enabled (toggleable via Config menu, option 3).

**Status / hint bar (bottom row)** — shows `Esc = menu` while editing, prompts
during operations, and displays the detected language name (e.g. `C/C++`,
`Pascal`) on the right when syntax highlighting is active.

---

## Keyboard Reference

### Navigation

| Key | Action |
|-----|--------|
| Arrow keys | Move cursor one character / line |
| `Home` | Beginning of line |
| `End` | End of line |
| `Ctrl+T` | Top of file |
| `Ctrl+W` | Bottom of file |
| `PgUp` | Scroll up one screen |
| `PgDn` | Scroll down one screen |
| `Ctrl+A` | Move left one word |
| `Ctrl+Y` | Move right one word |
| `Ctrl+G` | Go to line number (prompts) |

### Editing

| Key | Action |
|-----|--------|
| Printable keys | Insert character at cursor |
| `Enter` | New line (with auto-indent if enabled) |
| `Backspace` | Delete character to the left |
| `Delete` | Delete character under cursor |
| `Ctrl+D` | Delete entire current line |
| `Tab` | Insert tab (spaces or hard tab depending on config) |

### Clipboard

| Key | Action |
|-----|--------|
| `Ctrl+C` | Copy current line (or block if selected) |
| `Ctrl+X` | Cut current line (or block if selected) |
| `Ctrl+V` | Paste clipboard above current line |
| `Ctrl+L` | Clear clipboard |

### Block Selection

| Key | Action |
|-----|--------|
| `Ctrl+B` | Mark block **start** at current line |
| `Ctrl+E` | Mark block **end** at current line |
| `Ctrl+U` | Unset / clear block selection |

### Search

| Key | Action |
|-----|--------|
| `Ctrl+F` | Find (prompts for search string) |
| `Ctrl+N` | Find next occurrence |

### File / System

| Key | Action |
|-----|--------|
| `Esc` | Open main menu |
| `Ctrl+R` | Open context menu (Copy / Cut / Paste popup) |
| `Ctrl+M` | Run macro file |

---

## Mouse Support

TE-86 supports any MS-DOS compatible mouse driver (INT 33h).  The mouse cursor
is displayed automatically if a driver is detected at startup.

| Action | Effect |
|--------|--------|
| **Left-click** | Move the text cursor to the clicked position |
| **Right-click** | Open the Cut / Copy / Paste context menu |

Drag-selection via mouse is not supported; use `Ctrl+B` / `Ctrl+E` to mark
blocks by keyboard.  If no mouse driver is present, the editor runs normally
with keyboard only.

---

## Block Selection

A block is a contiguous range of whole lines.  Blocks are used for cut, copy,
and context-menu operations.

1. Move the cursor to the first line you want to select.
2. Press **`Ctrl+B`** to mark the block start.
3. Move the cursor to the last line you want to select.
4. Press **`Ctrl+E`** to mark the block end.

The selected lines are highlighted in reverse video.  Press **`Ctrl+U`** to
clear the selection without modifying text.

Once a block is marked, `Ctrl+C` copies it, `Ctrl+X` cuts it, and `Ctrl+R`
(context menu) gives you all three options in a popup.

---

## Context Menu

Press **`Ctrl+R`** or **right-click** anywhere in the text area to open the
Cut / Copy / Paste popup menu.

```
+------+
| Copy |
| Cut  |
| Paste|
+------+
```

Navigate with **Up / Down** arrow keys and press **Enter** to confirm, or press
the first letter (`C` for Copy, `X` for Cut, `V` for Paste) as a shortcut.
Press **Esc** or click outside the box to cancel.

- With a block active: Copy and Cut operate on the entire selected block.
- Without a block: Copy and Cut operate on the current line only.
- Paste is always available when the clipboard is non-empty.

---

## Clipboard

The clipboard holds a single line of text.  It is populated by Cut and Copy
operations (keyboard or context menu) and inserted by Paste.  Paste inserts
the clipboard line **above** the current line and moves the cursor down.
`Ctrl+L` clears the clipboard.

---

## Find and Go To Line

**Find (`Ctrl+F`)** — prompts for a search string (up to 31 characters) on the
status bar.  The search is case-sensitive and wraps around from the bottom of
the file back to the top.  Press `Ctrl+N` to repeat the last search.

**Go to line (`Ctrl+G`)** — prompts for a line number.  The cursor jumps to
that line immediately.

---

## Macros

TE-86 supports simple keystroke macro files.  A macro file is a plain text file
with the extension `.m`.

Press **`Ctrl+M`** to run a macro.  The editor prompts for the macro filename.
During execution, auto-indent and auto-list are suspended and restored when the
macro finishes.

Macro file syntax:

| Sequence | Meaning |
|----------|---------|
| Any printable text | Inserted literally into the buffer |
| `{key_name}` | Simulates pressing the named key |
| `{key_name:arg}` | Key with argument (e.g. find string) |
| `\\` | Literal backslash |

The **Insert** option in the main menu (`Esc → I`) uses the same macro engine
to insert the contents of any file verbatim into the current buffer at the
cursor position.

---

## Syntax Highlighting

Syntax highlighting is enabled by default for recognised file types.  The
language is detected automatically from the file extension when a file is
opened or saved.

### Supported Languages and Extensions

| Language | Extensions |
|----------|-----------|
| C / C++ | `.c` `.h` `.cpp` `.hpp` `.cc` |
| Assembly (x86) | `.asm` `.s` `.inc` |
| BASIC | `.bas` |
| Pascal | `.pas` `.pp` |
| FORTRAN | `.f` `.for` `.f77` `.f90` `.ftn` |
| PL/I | `.pli` `.pl1` |
| COBOL | `.cob` `.cbl` `.cobol` |
| PL/M (Intel 8080/8086) | `.plm` `.plm86` `.plm51` |

### Colour Attributes (CGA/EGA/VGA)

| Token type | Colour |
|------------|--------|
| Normal text | Light grey |
| Keywords | Bright cyan |
| String / char literals | Bright yellow |
| Comments | Bright green |
| Numbers | Bright magenta |
| Preprocessor directives | Bright blue |
| Operators | Bright red |

### Monochrome Mode (MDA / Hercules)

When **Hilite mono** is enabled (Config option 9), TE-86 uses MDA-safe
attributes instead of colours:

| Token type | Attribute |
|------------|-----------|
| Keywords, preprocessor | Bold |
| Strings / char literals | Bold + underline |
| Comments | Underline |
| Everything else | Normal |

### Toggling Highlighting

- Config option **8** — turn highlighting on or off globally.
- Config option **9** — switch between colour and monochrome highlight mode.

The current language name is shown in the bottom-right corner of the status bar
while a recognised file is open.

---

## File Browser

When you choose **Open** from the main menu (`Esc → O`) and no filename is
typed, the **File Browser** opens automatically.  It shows a scrollable,
three-column view of the files and subdirectories in the current directory.

- **Arrow keys** navigate the file list.
- **Enter** selects the highlighted file or enters a subdirectory.
- **Esc** cancels and returns to the filename prompt where you can type a path
  manually.

---

## Configuration

Press **`Esc`** then **`C`** to open the Configuration menu.  All settings take
effect immediately.  Press **`S`** inside the Config menu to save the current
settings to `TE-86.CFG` so they persist across sessions.

| Option | Setting | Description |
|--------|---------|-------------|
| 1 | Tab width (1–10) | Number of spaces per soft tab stop |
| 2 | Line number width (0–6) | Gutter width in columns (0 hides gutter) |
| 3 | Line numbers ON/OFF | Show or hide line numbers in the gutter |
| 4 | Auto-indent ON/OFF | New lines inherit the indentation of the line above |
| 5 | Auto-list ON/OFF | New lines after a list-bullet line start with a bullet |
| 6 | List bullets | Characters recognised as list-bullet starters (e.g. `- * >`) |
| 7 | C-lang complete ON/OFF | Automatic closing of `{` / `(` / `[` brackets |
| 8 | Syntax highlight ON/OFF | Enable or disable syntax colouring |
| 9 | Hilite mono ON/OFF | Use bold/underline instead of colour (for MDA/Hercules) |
| A | Ruler character | Character used for ruler fill (default `.`) |
| B | Ruler tab character | Character marking tab positions on ruler (default `!`) |
| C | Line number separator | Character between line numbers and text (default `\|`) |
| D | Horizontal line character | Character used for horizontal dividers (default `-`) |
| E | Hard tabs ON/OFF | Store literal ASCII 0x09 tab characters instead of expanding |
| S | Save and exit | Write settings to `TE-86.CFG` and return to editing |

---

## Config File (TE-86.CFG)

When `TE-86.CFG` is found in the current directory at startup, TE-86 loads it
and overrides the compiled-in defaults.  The file is written by choosing
**S** (Save) inside the Config menu.

The config block is 142 bytes and is version-stamped.  Fields added in
versions 3 and 4 (line-number visibility, highlighting flags, hard tabs) are
loaded only when the file is large enough, so older config files remain
compatible — new fields fall back to the compiled-in defaults.

> **Important:** If you are changing compiled-in defaults (e.g. key bindings or
> the line limit) and TE-86 appears to ignore the change, delete `TE-86.CFG`
> from the working directory so the saved copy does not override the new
> compiled values.

---

## TECF — Config Patcher

`TECF.EXE` is a standalone utility that patches configuration values directly
inside the `TE86.EXE` binary without recompiling.

```
TECF te86.exe
```

TECF locates the `TE_CONF` marker in the executable and presents a menu of all
patachable fields, including:

- Screen dimensions (rows × columns)
- File line limit (up to 6000)
- Tab width
- Auto-indent, auto-list, C-language completion
- Ruler, vertical, and horizontal line characters
- Key bindings for all 29 editor functions

Changes are written back to the `.EXE` file immediately.  No recompile is
needed.

---

## Limitations

- **File size:** up to **6000 lines** (compiled-in default; can be raised by
  editing `te_conf.c` or patching with TECF).
- **Line length:** up to **255 characters** per line.  Lines longer than 255
  characters are silently truncated on load with a one-time warning.
- **Clipboard:** single-line only; the last line of a cut/copied block is what
  ends up in the clipboard.
- **Memory model:** small model by default, which limits total code + data to
  64 KB each.  Switch to large model in `build.bat` if you hit memory errors
  with very large files.
- **Undo:** not supported.
- **Mouse drag selection:** not supported; use `Ctrl+B` / `Ctrl+E`.

---

## License

TE (CP/M original) — Copyright © 2015–2021 Miguel Garcia / FloppySoftware  
TE-86 (MS-DOS port) — Copyright © 2024–2026 Mickey W. Lawless

Licensed under the **GNU General Public License v2 or later**.  See
`COPYING` or <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.

---

## Changelog

### TE-86 v1.0 (2026)

This is the first versioned release of the MS-DOS port.  All changes below are
relative to the original CP/M TE source by Miguel Garcia.

#### New Features

**Direct video RAM output (`te_crt.c`)**
- All character output now uses `VidPoke()` — direct writes to video RAM via
  far pointer — replacing the BIOS TTY `putch()` call that ignored text
  attributes entirely.
- Video segment is auto-detected at startup by reading BIOS data area byte
  `0040:0049`: mode 7 selects MDA/Hercules segment `0xB000`; all other modes
  use CGA/EGA/VGA segment `0xB800`.
- `CrtSetAttr()`, `CrtClearLine()`, and `CrtClearEol()` all honour the current
  attribute, so colour and mono highlight changes are rendered correctly on
  every adapter type.

**Syntax highlighting (`te_ui.c`, `te.h`)**
- 8 languages detected automatically by file extension: C/C++, x86 Assembly,
  BASIC, Pascal, FORTRAN, PL/I, COBOL, PL/M.
- 6 token categories coloured per language: keywords, string literals,
  comments, numbers, preprocessor directives, operators.
- Full colour mode for CGA/EGA/VGA; bold/underline monochrome mode for
  MDA/Hercules adapters.
- Persistent block-comment state (`hl_in_comment`) tracked across lines so
  multi-line `/* ... */` blocks render correctly.
- Toggle on/off and switch colour/mono via the Config menu.
- Language name shown in bottom-right status bar while a file is open.

**Mouse support (`te_mouse.c`)**
- INT 33h mouse driver detected at startup; editor runs normally if no driver
  is present.
- Left-click moves the text cursor to the clicked position.
- Right-click opens the Cut / Copy / Paste context menu.
- Mouse cursor auto-hidden during screen redraws via nested `MouseHide()` /
  `MouseShow()` calls.

**Context menu (`te_ui.c`, `te_loop.c`)**
- `Ctrl+R` or right-click opens a small popup box with Copy, Cut, and Paste
  options, drawn near the cursor or click position.
- Keyboard: Up/Down to navigate, Enter to confirm, Esc to cancel; `C`, `X`,
  `V` as direct shortcuts.
- Mouse: hover highlights items, left-click selects, click outside cancels.
- Operates on the active block selection if one is set, otherwise on the
  current line.

**File Browser (`te_ui.c`)**
- Scrollable three-column graphical file-picker invoked from `Open` in the
  main menu.
- Arrow-key navigation; Enter selects file or enters subdirectory; Esc returns
  to manual filename prompt.

**Block selection (`te_loop.c`, `te_keys.c`)**
- `Ctrl+B` marks block start, `Ctrl+E` marks block end, `Ctrl+U` clears.
- Selected lines highlighted in reverse video.
- Block shortcuts hardcoded in `GetKey()` before the `cf_keys[]` table scan so
  they cannot be overridden by a saved `.CFG` file.

**Configuration system extensions (`te_conf.c`)**
- Config block extended from 138 bytes (v1/v2) → 141 bytes (v3) → 142 bytes
  (v4).
- v3 fields: `cf_lnum_show` (line number visibility), `cf_hl_enable`
  (highlighting toggle), `cf_hl_mono` (monochrome highlight mode).
- v4 field: `cf_hard_tabs` (preserve literal ASCII 0x09 tab characters).
- `CfLoad()` is backward-compatible: new fields fall back to compiled-in
  defaults when loading an older config file.
- File line limit (`cf_mx_lines`) raised from 512 → 6000.

**Hard tab support (`te_file.c`, `te_conf.c`)**
- When `cf_hard_tabs` is on, ASCII 0x09 bytes are stored in the line buffer
  unchanged rather than expanded to spaces on load, and are written back as-is
  on save.
- Toggle via Config menu option **E**.

**Auto-indent and auto-list**
- Auto-indent: new lines created with Enter copy the leading whitespace of the
  current line.
- Auto-list: when the current line begins with a recognised bullet character,
  Enter starts the next line with the same bullet.  Bullet characters are
  configurable.

**C-language bracket completion**
- When C-lang complete is on, typing `{`, `(`, or `[` automatically inserts
  the matching closing character.

**Ruler display**
- Full-width ruler line below the title bar shows column positions and tab
  stop markers.  Both characters are configurable.

**Line number gutter**
- Line numbers displayed in a configurable-width left gutter.
- Gutter width (0–6 digits) and separator character are configurable.
- When hidden (width 0), the gutter collapses completely so more text is
  visible.

**Tab width 1–10**
- Tab expansion width configurable from 1 to 10 spaces (Config option 1).

**TECF config patcher (`tecf.c`)**
- Standalone utility that patches all configuration values directly inside the
  `.EXE` binary without recompilation.

#### Bug Fixes

- **"Line too long" crash on load** — Lines wider than 255 characters are now
  silently truncated with a one-time warning instead of aborting the load.
- **Colour attributes ignored** — `putch()` uses BIOS TTY output which ignores
  the text attribute register; replaced with direct video RAM writes.
- **Line length coupled to screen width** — `ln_max` is now the hard constant
  255 rather than being derived from `cf_cols`, so files with long lines load
  correctly regardless of the configured screen width.
- **Mouse click jumps to bottom of file** — drag events fired continuously
  while the mouse button was held, flooding the event queue with block-extend
  codes that clamped to the last line.  All drag logic removed; only rising-
  edge click events are generated.
- **Context menu body never executed** — the `K_CTX_MENU` / `K_MOUSE_CTX`
  handler body in `te_loop.c` was wrapped in `#if OPT_BLOCK`, causing
  `CtxMenu()` to never be called.  The outer guard was removed.
- **Context menu instantly dismissed on right-click** — the mouse button was
  still physically held when the menu's cancel-check loop first ran.  A
  right-button release wait was added before the menu is drawn.
- **Context menu border overrun** — `CTX_W` was 10 but the labels are 6
  characters wide making each row 8 characters; the top and bottom borders drew
  2 extra dashes past the sides of the box.  `CTX_W` corrected to 8.
- **Ctrl+R swallowed by line editor** — `K_CTX_MENU` was missing from
  `BfEdit()`'s break-out switch; the key was consumed in the inner loop and
  never reached `Loop()`.
- **Ctrl+B jumps to end of file** — `cf_keys[7] = 2` mapped Ctrl+B to
  `K_BOTTOM` (go to file bottom) in the key table.  Ctrl+B, Ctrl+E, and
  Ctrl+U are now hardcoded in `GetKey()` before the table scan.
- **`ms_event_*` multiple-definition error** — the four `ms_event_*` globals
  were defined twice in `te_mouse.c` after refactoring; duplicate definitions
  removed.

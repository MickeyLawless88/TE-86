# te-86

**A full-featured programmer's text editor for MS-DOS, built with Turbo C 2.0.**

te-86 is a port and deep reimagining of Miguel Garcia's original `te` text editor, written for CP/M systems and published at [floppysoftware.es](http://www.floppysoftware.es). Garcia's CP/M editor was a compact, well-crafted tool for its platform. te-86 takes that foundation and builds something considerably larger on top of it — adding multi-language syntax highlighting, a persistent configuration system, a toggleable line-number gutter, macro scripting, block operations, and a hardened screen layer — producing an editor that is an exponential improvement on its ancestor while remaining true to the spirit of a lean, keyboard-driven DOS tool.

---

## Features

### Editor core

- Full-screen text editing in 80×25 (or auto-detected) text mode
- Status bar showing filename, current / total / max lines, column position, and line length
- Ruler line with configurable tab-stop markers
- Horizontal scrolling — lines up to 255 characters load and edit without error
- Auto-indent
- Auto-list (continues bullet characters on new lines with configurable bullet set)
- C-language bracket and brace completion
- Word-left / word-right navigation
- Find string and find-next
- Go-to line number
- Block select, cut, copy, paste, and clear clipboard with reverse-video selection display
- Macro system — record and play back editing sequences from `.m` files
- Backup-on-save

### Syntax highlighting

Language detection is automatic from the file extension and can be toggled on/off at runtime from the Config menu. The active language is shown right-aligned on the system line. Both colour (CGA/EGA/VGA) and monochrome (MDA/Hercules) attribute sets are supported.

| Language | Extensions |
|----------|------------|
| C / C++ | `.c` `.h` `.cpp` `.hpp` `.cc` |
| x86 Assembly | `.asm` `.s` `.inc` |
| BASIC | `.bas` |
| Pascal | `.pas` `.pp` |
| FORTRAN 77/90 | `.f` `.for` `.f77` `.f90` `.ftn` |
| PL/I | `.pli` `.pl1` |
| COBOL | `.cob` `.cbl` `.cobol` |
| PL/M | `.plm` `.plm86` `.plm51` |

**Colour scheme (CGA/EGA/VGA):**

| Token type | Colour |
|------------|--------|
| Keywords | Bright cyan |
| Strings / character literals | Bright yellow |
| Comments | Bright green |
| Numbers | Bright magenta |
| Preprocessor directives (`#`) | Bright blue |
| Operators | Bright red |
| Plain text | Light grey |

**Monochrome mode** uses bold, underline, and bold+underline combinations to distinguish token types on MDA and Hercules adapters with no colour information.

**Language-specific comment rules are fully implemented:**

- **C/C++:** `//` line comments and `/* */` block comments; `#` preprocessor directives
- **Pascal:** `{ }` single-line and `(* *)` block comments
- **FORTRAN:** column-1 `C`/`c`/`*` marks a whole-line comment (fixed-form 77); `!` for inline comments (free-form 90)
- **COBOL:** column-7 indicator area — `*` or `/` marks a comment line
- **BASIC:** `'` apostrophe line comments; `REM` via keyword table
- **x86 ASM:** `;` and `!` line comments
- **PL/I, PL/M:** `/* */` block comments

### Configuration

All settings are stored in `TE-86.CFG` (binary, in the current directory) and can be changed live from the Config menu without restarting the editor. If no config file is present, compiled-in defaults are used.

| Setting | Default | Notes |
|---------|---------|-------|
| Tab width | 4 | 1–8 columns |
| Line number gutter width | 4 | 0–6 digits |
| Line numbers on/off | On | Runtime toggle; ruler repositions immediately |
| Auto-indent | On | |
| Auto-list | On | |
| List bullet characters | `-*>` | Up to 7 characters |
| C-language completion | On | |
| Syntax highlighting | On | Runtime toggle |
| Monochrome highlight mode | Off | For MDA/Hercules |
| Ruler character | `.` | |
| Ruler tab character | `!` | |
| Line number separator | `\|` | |
| Horizontal line character | `-` | |

Configuration is versioned. Files written by earlier versions of te-86 load cleanly — newer fields retain their compiled-in defaults when an older config is loaded.

---

## Building

Requires **Borland Turbo C 2.0** for MS-DOS. The project builds as a 16-bit small-model DOS `.EXE`.

```
BUILD.BAT
```

Or compile manually:

```
tcc -ms -O -Z -G te_main.c te_ui.c te_crt.c te_keys.c te_edit.c te_loop.c te_file.c te_lines.c te_misc.c te_conf.c te_error.c te_macro.c
```

The resulting `TE.EXE` runs directly from DOS or from within DOSBox.

To build the configuration patcher utility:

```
tcc -ms tecf.c
```

---

## Usage

```
TE [filename]
```

If a filename is given on the command line it is opened immediately. If the file does not exist a new empty buffer is created with that name ready for saving. If no filename is given the editor starts with an empty unnamed buffer.

### Key bindings (defaults)

| Key | Action |
|-----|--------|
| Arrow keys | Move cursor |
| Home / End | Beginning / end of line |
| Page Up / Page Down | Scroll one page |
| Ctrl-T | Jump to top of file |
| Ctrl-B | Jump to bottom of file |
| Ctrl-F | Find string |
| Ctrl-N | Find next occurrence |
| Ctrl-G | Go to line number |
| Ctrl-A | Word left |
| Ctrl-E | Word right |
| Ctrl-K | Set block start |
| Ctrl-O | Set block end |
| Ctrl-U | Unset block |
| Ctrl-X | Cut block to clipboard |
| Ctrl-C | Copy block to clipboard |
| Ctrl-V | Paste clipboard |
| Ctrl-D | Delete current line |
| Ctrl-L | Clear clipboard |
| Tab / Ctrl-I | Indent |
| Ctrl-M | Run macro file |
| Enter | Insert new line |
| Backspace | Delete character left |
| Delete | Delete character right |
| Esc | Open menu |

All key bindings are remappable through `TECF.EXE` (the te-86 configuration patcher).

---

## Configuration patcher — TECF

`TECF.EXE` is a command-line tool that patches the key bindings and default settings inside `TE.EXE` directly, so the compiled-in defaults match your preferences before any `TE-86.CFG` is loaded. This is useful for creating a customised build without editing source code.

```
TECF TE.EXE
```

---

## How te-86 differs from the original CP/M `te`

Miguel Garcia's CP/M `te` was a clean, minimal editor written for the constraints of CP/M-80 machines — typically 64 KB of RAM, a monochrome character display, and a Z80 or 8080 processor running at a few MHz. Within those limits it was an accomplished piece of work. The source was clear, portable, and well-structured. It deserves credit as the seed from which te-86 grew.

te-86 is a different animal in almost every respect. The following capabilities were entirely absent from the CP/M original and were added from scratch:

**Syntax highlighting.** The CP/M `te` had no concept of token-level display. te-86 adds a full highlighting engine for eight programming languages — C/C++, x86 Assembly, BASIC, Pascal, FORTRAN, PL/I, COBOL, and PL/M — with language auto-detection from the file extension, per-language keyword tables, per-language comment-style rules (including column-position rules for FORTRAN and COBOL), correct string-escape handling, and both a colour attribute set for CGA/EGA/VGA and a monochrome set for MDA/Hercules. None of this existed in any form in the original.

**Persistent configuration.** The CP/M version was configured by recompiling. te-86 stores all settings in a versioned binary config file (`TE-86.CFG`) that loads automatically on startup, with a full live Config menu for runtime changes. The config block is patched directly into the executable by `TECF` for baseline customisation.

**Line number gutter.** Configurable digit width, separator character, and a runtime on/off toggle. When toggled, the ruler row and cursor column position both reflow immediately to match — the gutter width is computed live everywhere rather than being a fixed offset.

**Long-line support.** The CP/M version would reject any line longer than the visible screen width, producing a fatal error. te-86 decouples the internal line-length limit (255 characters) from the screen geometry entirely. Files with lines wider than 80 columns load without complaint and scroll horizontally.

**Block operations.** Arbitrary line-range cut, copy, paste, and clipboard clear, with reverse-video selection display on colour hardware and a column marker on monochrome.

**Macro scripting.** Editing sequences can be stored in `.m` files and replayed on demand via Ctrl-M.

**C-language completion.** Automatic bracket and brace pairing during editing.

**Hardened screen layer.** The CGA/EGA/VGA console layer clips all character output at the right screen edge, preventing any line from wrapping onto the separator or system rows. The editor box row bounds are clamped against the live terminal height at every redraw. Screen dimensions are always taken from the hardware at startup and are never overridden by a stale config value.

**Extended language support beyond C.** The CP/M original had no language awareness at all. te-86 supports eight languages with correct per-language tokenisation, all implemented in a single unified highlighter that selects the appropriate rules at line-render time.

The original CP/M `te` source was the architectural starting point — the line-pointer array model, the edit loop structure, the key-binding table approach. What shipped as te-86 is an exponential improvement on every dimension: capability, robustness, language support, and platform integration. The debt to Garcia's original work is acknowledged. The distance travelled from it is considerable.

---

## Repository

**Author:** Mickey W. Lawless  
**GitHub:** [github.com/mickeylawless88](https://github.com/mickeylawless88)  
**Project:** [github.com/mickeylawless88/te-86](https://github.com/mickeylawless88/te-86)  

Original CP/M `te` by Miguel Garcia / FloppySoftware — [floppysoftware.es](http://www.floppysoftware.es)

---

## License

te-86 is distributed under the **GNU General Public License, version 3** (GPL-3.0-or-later).

See [`LICENSE`](LICENSE) for the full license text.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see [https://www.gnu.org/licenses/](https://www.gnu.org/licenses/).

# How it works

A guided tour of the code, for reading alongside the source. Each section points at the ideas worth learning from that edition, especially the Linux and systems-programming ones.

1. [The idea](#1-the-idea)
2. [The web app](#2-the-web-app)
3. [The 3D field: projecting a year onto a terminal](#3-the-3d-field-projecting-a-year-onto-a-terminal)
4. [Python edition](#4-python-edition)
5. [C edition](#5-c-edition)
6. [The Makefile](#6-the-makefile)
7. [Bash edition](#7-bash-edition)
8. [BASIC edition](#8-basic-edition)
9. [Ideas for extending it](#9-ideas-for-extending-it)

---

## 1. The idea

Bardo Lattice treats a year as a three-dimensional array and places your writing inside it.

- In the **web app**, the array is `days × 24 hours × 12 five-minute slots`. The size comes from the clock, so 2026 has 365 × 24 × 12 = 105,120 cells, and a leap year has 366 days. Every five minutes the cell you are writing in seals.
- In the **local editions**, the unit is the day. The array is the year's grid of weeks × weekdays, and each cell is a real file on disk. In the 3D field, the third dimension is how much you wrote.

In both, the days before you started are part of the array. Time you did not write in is still shown, as elapsed and empty.

The measures in the web app each come from a lineage:

| Measure | Lineage | What it records |
|---|---|---|
| Instance | rebirth, saṃsāra | each opening of the program is a new instance |
| Pause | Lynch | the longest silence between keystrokes inside a cell |
| Excision | Cronenberg | characters deleted from a cell's text |
| Edits from later | Philip K. Dick | revisions made after a cell sealed |
| Cut-up | Burroughs & Gysin | text cut into short runs and reshuffled |
| Extinguish | nirvāṇa, "blowing out" | a cell's text removed, its trace kept |

---

## 2. The web app

`index.html` is a single file: HTML, CSS and JavaScript together, no libraries.

**The array index.** A moment in time maps to a cell:

```js
d = dayOfYear(date)          // 0..364 (or 365)
h = date.getHours()          // 0..23
s = Math.floor(minutes / 5)  // 0..11
i = d * 288 + h * 12 + s     // one integer per cell; 288 = 24 * 12
```

Turning cells into single integers makes "is this cell in the past?" a plain comparison: `i < nowIndex`.

**The sealing loop.** `setInterval(tick, 1000)` runs every second. `tick()` walks every block of writing and checks whether its cell is still the current one. The actual split happens in a `beforeinput` handler: when you type at the end of the open block and its cell has passed, the keystroke is cancelled and re-inserted into a new block in the new cell. The handler waits until a word ends so it never splits a word.

**Chat-style timestamps.** `relabel()` decides which blocks get a visible stamp: always on a new day, otherwise after 20 minutes or 6 blocks since the last stamp. That is why they seem to appear at arbitrary moments.

**Storage.** Everything is JSON in `localStorage`, wrapped in `try/catch` because some browsers block it.

**The field.** Each animation frame projects every elapsed cell (up to 105,120 points) onto a character grid, keeps a depth buffer so nearer points win, and draws the result on a `<canvas>` with `fillText`. The math is the same as the terminal field below.

---

## 3. The 3D field: projecting a year onto a terminal

The Python and C editions draw the same field. Here is the whole pipeline.

**1. Place each day in 3D.** For day `i` at grid position `(row, col)`:

```
x = (col / (ncols - 1) - 0.5) * 3.2     # weeks run left to right
z = (row / 6 - 0.5) * 0.62              # weekdays run front to back
y = 0                                   # the floor
```

A written day also gets a column of `|` from the floor up to a height `h`:

```
h = 0.12 + 0.9 * log(1 + words) / log(1 + most_words_in_any_day)
```

The logarithm keeps one huge day from flattening everything else.

**2. Rotate.** Two rotations: `a` around the vertical axis (the spin), then `b` around the horizontal axis (the tilt).

```
x1 =  x*cos(a) + z*sin(a)
z1 = -x*sin(a) + z*cos(a)
y1 =  y*cos(b) - z1*sin(b)
z2 =  y*sin(b) + z1*cos(b)
```

**3. Perspective.** Things farther away (larger `z2`) shrink toward the center:

```
f = dist / (dist + z2)          # dist = 4.2
screen_col = cx + x1 * f * scale * 2.0
screen_row = cy + y1 * f * scale
```

The `* 2.0` on columns is because terminal characters are about twice as tall as they are wide. Without it the year would look squashed.

**4. Depth buffer.** Many points land on the same character cell. For each cell, keep only the nearest point (smallest `z2`). That is a z-buffer, the same technique graphics cards use.

**5. Draw.** Write the surviving glyph for each cell, colored by state. A brightened row sweeps down the screen as a scan line, and today's `@` blinks.

---

## 4. Python edition

File: `python/bardo.py`. Standard library only.

**`Year` class.** Owns the folder. `init()` handles the first run (marker, files). `scan()` counts words. `cell()` and `index_at()` convert between a day index and a grid position. `glyph()` and `state()` apply the rules from [FILE-FORMAT.md](FILE-FORMAT.md).

**`pathlib.Path`.** Paths are objects: `self.dir / "2026-10-23.md"` joins with `/`, and `.read_text()` / `.write_text()` read and write in one call.

**curses.** The terminal UI library that ships with Python on Linux.

- `curses.wrapper(fn)` sets the terminal up, runs `fn`, and restores the terminal even if the program crashes. Always use it.
- `scr.getch()` waits for a key. `scr.timeout(33)` makes it return after 33 ms if nothing was pressed, which is how the field animates at about 30 frames a second.
- `read_key()` handles raw arrow escape sequences (`ESC [ A`) that some terminals send without curses translating them.

**Opening the editor.** `curses.def_prog_mode()` saves the curses terminal state, `curses.endwin()` hands the terminal back to normal, `subprocess.call()` runs the editor and waits, and `curses.reset_prog_mode()` takes the screen back.

**`shlex.split(editor)`.** Splits `$EDITOR` the way a shell would, so `EDITOR="code -w"` becomes `["code", "-w"]`.

**`argparse`.** Builds the command-line interface and the `--help` text from declarations.

---

## 5. C edition

File: `c/bardo.c`. Plain C99 plus POSIX. No ncurses: it talks to the terminal directly, which is a good way to learn what curses does for you.

**Feature macros.** The first lines, `#define _DEFAULT_SOURCE` and `#define _XOPEN_SOURCE 700`, tell the C library to expose POSIX and common Unix functions (`realpath`, `poll`, `clock_gettime`) that strict C99 hides. They must come before any `#include`.

**Dates without a library.** `is_leap`, `mdays` and `weekday` (Sakamoto's method) compute the calendar with integer arithmetic. `build_days()` fills a fixed array of 366 `Day` structs.

**Files.** `mkdir()` creates the year folder, and `errno == EEXIST` means it already existed. `access(path, F_OK)` tests whether a file exists. `fopen` / `fprintf` / `fgetc` do the reading and writing. `count_words()` reads one character at a time and counts transitions from whitespace to non-whitespace.

**Raw terminal mode with termios.** Normally the terminal is in *canonical* mode: it buffers a whole line and echoes what you type. A full-screen program needs each key immediately and silently. `raw_enter()`:

```c
tcgetattr(STDIN_FILENO, &orig_tio);         // save the current settings
t.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);  // no line buffering, no echo, no Ctrl-C signal
t.c_iflag &= ~(IXON | ICRNL);               // no Ctrl-S/Ctrl-Q flow control; keep Enter as \r
t.c_cc[VMIN] = 1;                            // read() returns after 1 byte
tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);      // apply
```

`raw_off()` puts the saved settings back. It is registered with `atexit()` and called from the signal handler, so the terminal is restored however the program ends.

**ANSI escape codes.** The screen is driven by text sequences starting with `ESC` (`\033`):

| Sequence | Does |
|---|---|
| `\033[2J` | clear the screen |
| `\033[row;colH` | move the cursor (1-based) |
| `\033[1m` `\033[2m` `\033[7m` | bold, dim, reverse video |
| `\033[38;5;209m` | 256-color foreground, color 209 |
| `\033[0m` | reset all attributes |
| `\033[?1049h` / `l` | switch to / from the alternate screen (so your scrollback is untouched) |
| `\033[?25l` / `h` | hide / show the cursor |

**Building a frame in memory.** `ob_add()` appends to a growable buffer with `vsnprintf` and `realloc`, and `ob_flush()` writes the whole frame with one `fwrite`. One write per frame avoids flicker.

**Reading keys with `poll()`.** `poll()` waits until input is ready or a timeout passes. The grid waits forever (`-1`); the field waits 33 ms so it keeps animating. Arrow keys arrive as three bytes (`ESC [ D`); after an `ESC`, `read_key()` polls 30 ms for the rest, which is how it tells a lone Escape from an arrow.

**Terminal size.** `ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)` asks the terminal driver for rows and columns. It is called every frame, so resizing the window just works.

**Running the editor: fork and exec.** The classic Unix way to start another program:

```c
pid_t pid = fork();                 // duplicate this process
if (pid == 0) {                     // in the child:
    execl("/bin/sh", "sh", "-c", "exec $0 \"$1\"", ed, path, NULL);  // become the shell, then the editor
    _exit(127);                     // only reached if exec failed
}
waitpid(pid, &st, 0);               // in the parent: wait for the child to finish
```

Going through `sh -c` lets `$EDITOR` contain arguments (`code -w`). Passing the editor and path as `$0` and `$1`, rather than pasting them into the command string, means a file path with spaces or quotes cannot break the command.

**Signals.** `signal(SIGINT, on_signal)` catches termination signals. The handler restores the terminal, resets to the default handler, and re-raises the signal so the program still exits the normal way.

**`realpath()`** turns `--dir .` into an absolute path, so messages and file names are unambiguous.

---

## 6. The Makefile

```make
PREFIX  ?= /usr/local
CFLAGS  ?= -O2 -g
CFLAGS  += -std=c99 -Wall -Wextra -pedantic
LDLIBS  += -lm

bardo: bardo.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ bardo.c $(LDLIBS)
```

- **`?=`** sets a variable only if it is not already set. You can override from the command line: `make PREFIX=~/.local` or `make CFLAGS=-O0`.
- **`+=`** appends. The warning flags are always added, even if you override `CFLAGS`.
- **`bardo: bardo.c`** is a rule: the target `bardo` depends on `bardo.c`. `make` rebuilds it only when `bardo.c` is newer than `bardo`. Run `make` twice and the second time it says there is nothing to do.
- **The recipe line must start with a Tab**, not spaces. This is the most famous `make` gotcha. Spaces give "missing separator".
- **`$@`** is an automatic variable meaning "the target name" (`bardo`).
- **`$(CC)`** defaults to `cc`. `make CC=clang` builds with clang instead.
- **`$(DESTDIR)`** in the install rule is a packaging convention: package builders install into a staging folder with `make install DESTDIR=/tmp/stage`.
- **`install -m 755`** copies a file and sets its permissions in one step (`755` = owner can read/write/run, everyone else can read/run).
- **`.PHONY`** marks targets like `clean` and `install` that are not files, so `make` always runs them even if a file named `clean` exists.
- **`$$`** in the `check` recipe passes a literal `$` through to the shell. A single `$` would be read by `make` as one of its own variables.

---

## 7. Bash edition

File: `bash/bardo.sh`.

**`set -u`** makes using an unset variable an error, which catches typos.

**Pure-Bash date arithmetic.** Calling `date` 365 times would be slow, so the script works out the calendar with `$(( ))` arithmetic, the same way as the C edition.

**`printf '%04d-%02d-%02d'`** formats numbers with leading zeros.

**One `awk` pass for the whole year.** Instead of running `wc` 365 times:

```bash
awk 'FNR == 1 { next } { w[FILENAME] += NF } END { for (f in w) print f, w[f] }' 2026/*.md
```

- `FNR` is the line number within the current file; `FNR == 1 { next }` skips each header line.
- `NF` is the number of fields (words) on a line, so `w[FILENAME] += NF` totals each file.
- `END { ... }` runs after all input and prints the totals.

**Associative arrays.** `declare -A BYNAME` creates a Bash 4 dictionary keyed by file name. This is why the script needs Bash 4 or newer (macOS ships Bash 3.2; install a newer one there with Homebrew).

**Precomputed glyphs.** `scan()` fills the arrays `W` (words), `G` (glyph) and `P` (color code) once. Drawing the grid then only concatenates strings, with no subshells. An earlier version called a function in `$( )` for each of the 365 cells, and each `$( )` forks a new process, which made every keypress take over a second.

**Reading single keys.** `read -rsn1 key`:

- **`-r`** keeps backslashes literal.
- **`-s`** silent: no echo.
- **`-n1`** returns after one character, without waiting for Enter.

If the key is `ESC`, `read -rsn2 -t 0.05 rest` grabs the next two characters within 50 ms to complete an arrow sequence. `-t 0.05` is the timeout in seconds.

**`tput smcup` / `rmcup`** switch to and from the alternate screen, like the C edition's escape codes but looked up from the terminal database. **`trap restore EXIT`** runs `restore` however the script exits.

**`sed '/./,$!d'`** in the preview deletes blank lines at the top: the range `/./,$` is "from the first non-empty line to the end", and `!d` deletes everything *outside* that range.

---

## 8. BASIC edition

File: `basic/bardo.bas`, for FreeBASIC.

- **`Dim Shared`** declares global variables visible inside every `Sub` and `Function`.
- **Arrays with initializers**: `Dim Shared WEEKDAYS(0 To 6) As String = {...}`.
- **`\`** is integer division and `Mod` is remainder, used in the calendar math.
- **`Date`** returns today as `mm-dd-yyyy`; the program rearranges it to `yyyy-mm-dd` with `Mid` and `Left`.
- **`FileExists`** comes from `#include once "file.bi"`.
- **`Open ... For Input As #f`** and `Line Input #f` read a file line by line. **`FreeFile`** returns an unused file number. Using `Open` as a function (`If Open(...) <> 0`) returns an error code instead of stopping the program.
- **`Shell`** runs the editor through the system shell.
- **`Color n`** sets the text color; 12 is light red, 14 yellow, 8 grey, 7 normal.
- **ISO dates compare as strings.** `dstr(i) > todayStr` works because `YYYY-MM-DD` sorts alphabetically in date order. The other editions use the same trick.

It is menu-driven rather than arrow-key driven because classic BASIC has no portable way to read a single keypress in a terminal.

---

## 9. Ideas for extending it

- **A `--week` or `--month` view** in `print` mode, for narrow terminals.
- **Tags**: count `#tags` in each day and color the grid by tag.
- **Search inside the browser**: `/` to type a word and highlight the days that contain it.
- **Cut-ups in C and Bash**, using the same fragment-and-shuffle method as the Python and web editions.
- **The five-minute lattice locally**: a mode that timestamps each paragraph you add to today's file every five minutes, bringing the web app's sealing into the file world.
- **A man page for every edition**, generated from one source.
- **A `bardo stats` command** that prints words per month as an ASCII bar chart.

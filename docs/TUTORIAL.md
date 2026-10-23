# Bardo Lattice tutorial

This tutorial takes you from nothing to writing in Bardo Lattice every day. It assumes you are new to the Linux terminal. Every command is broken down piece by piece, with the gotchas that trip people up.

There are two halves to the project:

1. **The web app** (`index.html`): the five-minute lattice in your browser.
2. **The local editions** (`python/`, `c/`, `bash/`, `basic/`): the year as one file per day, in your terminal.

You can use either one, or both.

---

## Contents

1. [What you need](#1-what-you-need)
2. [Getting the code](#2-getting-the-code)
3. [The web app](#3-the-web-app)
4. [Choosing a folder for your year](#4-choosing-a-folder-for-your-year)
5. [Setting your editor](#5-setting-your-editor)
6. [Python edition](#6-python-edition)
7. [C edition](#7-c-edition)
8. [Bash edition](#8-bash-edition)
9. [BASIC edition](#9-basic-edition)
10. [A day in the life](#10-a-day-in-the-life)
11. [Working with your files directly](#11-working-with-your-files-directly)
12. [Backing up](#12-backing-up)
13. [Troubleshooting](#13-troubleshooting)

---

## 1. What you need

On Fedora, most of this is already installed. To check, run each of these. Each one prints a version number if the tool is there.

```bash
git --version
python3 --version
gcc --version
make --version
bash --version
```

- **`--version`** is a flag almost every command-line tool understands. It prints the version and exits without doing anything else. It is the quickest way to ask "is this installed?"
- If one says `command not found`, install it with `dnf` (see below).

To install anything missing:

```bash
sudo dnf install git python3 gcc make
```

- **`sudo`** runs the next command as the administrator (root). It asks for your password. You need it because installing software changes system folders.
- **`dnf`** is Fedora's package manager: the tool that downloads and installs software.
- **`install`** is the `dnf` subcommand that installs packages.
- **`git python3 gcc make`** are the package names. You can list several at once.
- Gotcha: `dnf` asks "Is this ok [y/N]". The capital `N` means pressing Enter alone answers no. Type `y` and press Enter.

You only need what the editions you want to use require:

| Edition | Needs |
|---|---|
| Web app | any modern browser |
| Python | `python3` (3.8 or newer) |
| C | `gcc` (or `clang`) and `make` |
| Bash | `bash` 4 or newer, `awk` |
| BASIC | FreeBASIC (`fbc`) |

---

## 2. Getting the code

```bash
cd ~
git clone https://github.com/public-space/bardo-lattice.git
cd bardo-lattice
ls
```

- **`cd ~`** moves you to your home folder. `cd` means "change directory". `~` is shorthand for your home folder (for you, `/home/milarepa`).
- **`git clone <url>`** downloads a full copy of the repository, including its history, into a new folder named after the repo.
- **`cd bardo-lattice`** moves into that new folder.
- **`ls`** lists what is in the current folder. You should see:

```
README.md  bash  basic  c  docs  index.html  python
```

To get updates later, run this from inside the folder:

```bash
git pull
```

- **`git pull`** downloads new commits from GitHub and merges them into your copy.

---

## 3. The web app

The web app is one file, `index.html`. To use it:

```bash
xdg-open index.html
```

- **`xdg-open`** opens a file with whatever program your desktop uses for that type. For `.html`, that is your browser.
- You can also just double-click `index.html` in your file manager.

### What you will see

- **Write**: a plain editor. Type. Every five minutes the cell you are writing in seals, and your next word starts a new block in the next cell. Light timestamps appear the way chat apps mark time.
- **Array · Data**: the whole year as a heatmap (days across, hours down), a grid of one day's 288 five-minute cells, and an inspector for any cell you click.
- **Array · Field**: the year as a rotating ASCII field. Drag to turn it. Click a glyph to inspect that cell.

### Where your writing goes

The web app saves to your browser's local storage. That means:

- It stays on that browser on that computer.
- It does not sync to other devices.
- Clearing site data erases it.

Use **Copy all text** in the Write view to back it up.

### Putting it online with GitHub Pages

1. On github.com, open the repo and go to **Settings → Pages**.
2. Under **Branch**, pick `main` and the `/ (root)` folder, then **Save**.
3. After a minute it is live at `https://public-space.github.io/bardo-lattice/`.

Each visitor gets their own private lattice in their own browser.

---

## 4. Choosing a folder for your year

The local editions create the year folder **in whatever folder you are in when you run them**, not where the program lives. So first decide where your year should live. For example:

```bash
mkdir -p ~/bardo
cd ~/bardo
```

- **`mkdir`** makes a directory (folder).
- **`-p`** means "make parent folders too if needed, and don't complain if it already exists." Without `-p`, running it twice gives an error.
- **`~/bardo`** is a folder named `bardo` in your home folder.

From now on, run the editions from `~/bardo`, and your files land in `~/bardo/2026/`.

If you would rather not `cd` there, the Python, C and Bash editions accept `--dir`:

```bash
python3 ~/bardo-lattice/python/bardo.py --dir ~/bardo
```

- **`--dir PATH`** tells the program to keep the year folder inside `PATH`.

---

## 5. Setting your editor

When you open a day, the editions launch the editor named in the `EDITOR` environment variable. If it is not set, they use `nano`.

Check what it is now:

```bash
echo "$EDITOR"
```

- **`echo`** prints its arguments.
- **`$EDITOR`** is replaced by the value of the variable. Empty output means it is not set.
- The quotes keep the output intact if the value contains spaces.

Set it for this terminal session:

```bash
export EDITOR=nano
```

- **`export`** sets a variable *and* makes it visible to programs you start from this shell. Without `export`, programs you launch would not see it.
- Gotcha: no spaces around `=`. `EDITOR = nano` is an error.

Make it permanent by adding that line to your shell's startup file:

```bash
echo 'export EDITOR=nano' >> ~/.bashrc
source ~/.bashrc
```

- **`>>`** appends the output to the end of a file. A single `>` would **replace** the whole file, which you don't want here.
- **Single quotes** stop the shell from expanding anything inside them, so the exact text is written.
- **`source ~/.bashrc`** re-reads the file in your current shell so the change takes effect now, without opening a new terminal.

Some choices:

| Editor | Good for |
|---|---|
| `nano` | beginners; the shortcuts are listed at the bottom (`^O` save means Ctrl+O, `^X` exit means Ctrl+X) |
| `micro` | beginner-friendly, with normal Ctrl+S / Ctrl+Q shortcuts (`sudo dnf install micro`) |
| `vim` | powerful, steep to learn |
| `code -w` | VS Code; `-w` makes the terminal wait until you close the file, so the grid updates afterwards |

`VISUAL`, if set, takes priority over `EDITOR`.

---

## 6. Python edition

The richest edition: grid browser, live preview, the 3D field, cut-ups, stats.

### First run

```bash
cd ~/bardo
python3 ~/bardo-lattice/python/bardo.py
```

- **`python3`** runs the Python 3 interpreter.
- **`~/bardo-lattice/python/bardo.py`** is the full path to the script, so you can run it from any folder.

On the first run it says it created `2026/` with 365 day files, then opens the browser.

### Plain text instead of Markdown

```bash
python3 ~/bardo-lattice/python/bardo.py --txt
```

- **`--txt`** only matters on the **first** run in a folder. It is recorded in `2026/.bardo` and every edition follows it from then on. Passing it later has no effect (the Python edition prints a note saying so).

### Keys in the grid

| Key | Does |
|---|---|
| arrows or `h` `j` `k` `l` | move; left/right is a week, up/down is a day |
| `[` and `]` | jump back or forward a month |
| Enter or `o` | open the selected day in your editor |
| `t` | back to today |
| `f` | switch to the 3D field |
| `c` | cut-up of the year's writing |
| `r` | rescan the folder (after editing files some other way) |
| `q` | quit |

Under the grid you see the selected day's date, word count, file name, and the first lines of what you wrote.

### Keys in the field

| Key | Does |
|---|---|
| left / right arrows | turn the year around |
| up / down arrows | tilt it |
| `+` and `-` | zoom |
| space | pause or resume the spin |
| `h` `j` `k` `l` | move the `v` cursor between days |
| Enter | open the day under the cursor |
| `g` | back to the grid |

In the field every day is a dot on a flat lattice of weeks by weekdays. Days you wrote in rise as columns, taller the more you wrote, so the shape of your year becomes a skyline.

### Other commands

```bash
python3 ~/bardo-lattice/python/bardo.py print
python3 ~/bardo-lattice/python/bardo.py open today
python3 ~/bardo-lattice/python/bardo.py open 2026-03-14
python3 ~/bardo-lattice/python/bardo.py --year 2025
```

- **`print`** prints the grid and stats and exits. Handy in scripts or at login.
- **`open DATE`** opens that day straight in your editor without the browser. `today` works as a date.
- **`--year 2025`** works on a different year's folder (creating it if needed).

### Make a short command for it

Typing the full path gets old. Add an alias:

```bash
echo "alias bardo='python3 ~/bardo-lattice/python/bardo.py'" >> ~/.bashrc
source ~/.bashrc
```

- **`alias name='command'`** makes `name` a shortcut for `command` in your shell.
- Double quotes on the outside let the single quotes sit inside the text that gets written.

Now `bardo`, `bardo print`, and `bardo open today` work from anywhere. (If you install the C edition below, it also provides a `bardo` command. Pick one.)

---

## 7. C edition

Grid browser and 3D field in plain C, with a real Unix build process. This is the one to install system-wide.

### Build

```bash
cd ~/bardo-lattice/c
make
```

- **`make`** reads the `Makefile` in the current folder and runs the steps to build the default target, here the program `bardo`. You will see the compiler command it runs:

```
cc -O2 -g -std=c99 -Wall -Wextra -pedantic   -o bardo bardo.c -lm
```

That line, piece by piece:

- **`cc`** is the system C compiler (on Fedora, a name for `gcc`).
- **`-O2`** turns on optimization level 2: faster code.
- **`-g`** includes debugging information, so a debugger like `gdb` can show you source lines.
- **`-std=c99`** compiles as the C99 standard.
- **`-Wall -Wextra`** turn on most warnings. Warnings are the compiler pointing at likely bugs.
- **`-pedantic`** warns about anything that is not strictly standard C.
- **`-o bardo`** names the output file `bardo`. Without `-o`, it would be called `a.out`.
- **`bardo.c`** is the source file.
- **`-lm`** links the math library (for `sin`, `cos`, `log1p`). Gotcha: libraries go **after** the source file. Putting `-lm` first can cause "undefined reference to sin" errors, because the linker reads left to right.

### Test

```bash
make check
```

This builds the program, runs it in a throwaway folder, checks that it created the right number of day files and the `.bardo` marker, writes three words into one day, and checks that it counts them. It prints `check passed` on success.

### Run without installing

```bash
cd ~/bardo
~/bardo-lattice/c/bardo
```

### Install

```bash
cd ~/bardo-lattice/c
make install PREFIX=~/.local
```

- **`make install`** copies `bardo` to `PREFIX/bin` and the manual page to `PREFIX/share/man/man1`.
- **`PREFIX=~/.local`** installs into your home folder, so no `sudo` is needed. Gotcha: write it after `make install`, with no spaces around `=`.
- On Fedora, `~/.local/bin` is on your `PATH` by default. If `bardo` says "command not found", open a new terminal or run `echo $PATH` and check that `/home/milarepa/.local/bin` is listed.

To install for every user on the machine instead:

```bash
sudo make install
```

That uses the default `PREFIX=/usr/local`.

Now, from any folder:

```bash
bardo
bardo print
bardo open today
man bardo
```

- **`man bardo`** opens the manual page. Space scrolls, `q` quits. If `man` cannot find it, run `MANPATH=~/.local/share/man:$MANPATH man bardo`, or add `~/.local/share/man` to `MANPATH` in `~/.bashrc`.

### Uninstall and clean

```bash
make uninstall PREFIX=~/.local
make clean
```

- **`uninstall`** removes the installed files. Use the same `PREFIX` you installed with.
- **`clean`** deletes the built program from the source folder.

### Keys

Same as the Python edition, minus the cut-up. Arrows or `hjkl`, `[` `]`, Enter, `t`, `f`, `r`, `q` in the grid. Arrows, `+` `-`, space, `hjkl`, Enter, `g` in the field.

---

## 8. Bash edition

Grid browser with arrow keys, preview, and open-in-editor, written entirely in Bash.

```bash
chmod +x ~/bardo-lattice/bash/bardo.sh
cd ~/bardo
~/bardo-lattice/bash/bardo.sh
```

- **`chmod +x file`** adds execute permission so the file can run as a program.
  - **`chmod`** means "change mode".
  - **`+x`** adds the execute bit; `-x` would remove it.
  - Gotcha: you only need to do this once per file.
- Running it by path works because the first line of the script, `#!/usr/bin/env bash` (the "shebang"), tells the system to use Bash.

Keys: arrows or `hjkl` move, `[` `]` jump a month, Enter opens, `t` today, `r` rescan, `q` quit.

Also supports `print`, `open DATE`, `--txt`, `--year N`, and `--dir PATH`, same as Python.

---

## 9. BASIC edition

A menu-driven version in FreeBASIC. It shows the grid, then waits for a command.

> **Status:** this edition is written against FreeBASIC's documented syntax but has not been compiled yet. If `fbc` reports errors, open an issue or paste the error and it will be fixed.

### Install FreeBASIC

Try Fedora's package manager first:

```bash
sudo dnf install FreeBASIC
```

If `dnf` cannot find it, download the Linux build from https://www.freebasic.net/ (Downloads), unpack it, and run its `install.sh`:

```bash
tar xf FreeBASIC-*-linux-x86_64.tar.gz
cd FreeBASIC-*-linux-x86_64
sudo ./install.sh -i
```

- **`tar xf file`** unpacks an archive.
  - **`x`** extracts.
  - **`f file`** names the archive file. Gotcha: `f` must be the **last** letter in the group, because the next word is taken as the file name. `tar xf name` works; `tar fx name` does not.
  - `tar` works out the compression (gzip, xz) by itself.
- **`./install.sh -i`** runs FreeBASIC's installer. `-i` means install.

### Build and run

```bash
cd ~/bardo-lattice/basic
fbc bardo.bas
cd ~/bardo
~/bardo-lattice/basic/bardo
```

- **`fbc bardo.bas`** compiles the BASIC source into a program named `bardo` in the same folder.

### Commands at the prompt

| Type | Does |
|---|---|
| `T` | open today in your editor |
| `D 2026-03-14` | open that date |
| `N 72` | open day number 72 of the year |
| `V 2026-03-14` | read that day on screen without opening an editor |
| `L` | list every day you have written in, with word counts |
| `R` | rescan the folder |
| `Q` | quit |

Also: `./bardo print`, `./bardo --txt` (first run), `./bardo --year 2025`.

---

## 10. A day in the life

A simple routine:

1. Open a terminal and run `bardo` (or your alias).
2. The cursor starts on today, the `@`. Press Enter.
3. Your editor opens `2026/2026-10-23.md`, already headed with the date. Write below the header. Save and quit.
4. You are back at the grid. Today's glyph now shows how much you wrote.
5. Press `f` to watch the year turn, with today's column rising out of it.

To see the year at a glance every time you open a terminal, add this to the end of `~/.bashrc`:

```bash
cd ~/bardo && bardo print; cd - > /dev/null
```

- **`&&`** runs the second command only if the first succeeded (so `bardo print` only runs if the folder exists).
- **`;`** runs the next command regardless.
- **`cd -`** returns to the folder you were in before.
- **`> /dev/null`** throws away its output (`cd -` prints the folder name otherwise). `/dev/null` is a special file that discards everything written to it.

---

## 11. Working with your files directly

Because every day is a plain file, all the normal Linux tools work on your year.

Find every day that mentions a word:

```bash
grep -ril "river" ~/bardo/2026/
```

- **`grep`** searches files for text.
- **`-r`** searches recursively through the folder.
- **`-i`** ignores upper/lower case.
- **`-l`** lists only the names of matching files, not the matching lines.
- Flags can be combined into one group: `-ril` is the same as `-r -i -l`.

Count all the words you have written (minus the header lines):

```bash
tail -q -n +2 ~/bardo/2026/*.md | wc -w
```

- **`tail -n +2`** prints each file starting from line 2, skipping the header.
- **`-q`** stops `tail` from printing a `==> filename <==` banner between files.
- **`*.md`** is a glob: the shell expands it to every `.md` file in the folder.
- **`|`** (pipe) sends the output of the left command into the right command.
- **`wc -w`** counts words.

Read a whole month in order:

```bash
cat ~/bardo/2026/2026-10-*.md | less
```

- **`cat`** prints files one after another.
- **`2026-10-*`** matches every October file. Because the names are dates, they sort in order.
- **`less`** lets you scroll. Space pages down, `q` quits.

The editions recount words every time they start, and the `r` key rescans, so changes you make this way show up in the grid.

---

## 12. Backing up

Your year is a folder of text files, which makes it easy to back up.

Copy it somewhere:

```bash
cp -r ~/bardo/2026 /path/to/backup/
```

- **`cp`** copies.
- **`-r`** copies folders recursively (everything inside). Without it, `cp` refuses to copy a folder.

Or keep it in git:

```bash
cd ~/bardo
git init
git add 2026
git commit -m "my year so far"
```

- **`git init`** turns the folder into a git repository.
- **`git add 2026`** stages the year folder for the next commit.
- **`git commit -m "..."`** saves a snapshot. `-m` gives the message inline; put it in quotes.

Run `git add 2026 && git commit -m "today"` whenever you want a new snapshot. If you push it to GitHub, make that repository **private**: it is your journal.

---

## 13. Troubleshooting

**"command not found: bardo"**
The C edition is not installed, or `~/.local/bin` is not on your `PATH`. Open a new terminal, or run the program by its full path, or set up the Python alias.

**The editor opened, but it's not the one I want.**
Set `EDITOR` (section 5). `VISUAL` takes priority, so check that too: `echo "$VISUAL"`.

**I used VS Code and the grid didn't update.**
Use `code -w` as your editor. Without `-w`, VS Code returns immediately and the program recounts before you have written anything. Press `r` to rescan.

**The arrow keys print letters like `^[[A`.**
That happens if you run the Bash or BASIC edition somewhere that is not a real terminal. Use a normal terminal window. In the Python and Bash editions, `h j k l` also move.

**The grid looks squashed or cut off.**
Make the terminal wider. At 115 columns or more, the editions space the days out. At 80 columns they pack one character per day.

**Colors look wrong or I want none.**
Run `NO_COLOR=1 bardo print`. Putting a variable before a command sets it for that one command only.

**I ran it in the wrong folder and it made a `2026/` I didn't want.**
Delete that folder: `rm -r 2026`. Careful: `rm -r` deletes without asking and there is no trash can. Check you are in the right place first with `pwd` (print working directory).

**`make` says "No targets specified and no makefile found".**
You are not in the `c/` folder. Run `cd ~/bardo-lattice/c` first.

**BASIC won't compile.**
That edition is untested. Copy the full error from `fbc` and report it.

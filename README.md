# Bardo Lattice

A writing program that places every session of writing inside a three-dimensional array built from the current year.

Part of **GOOP**, an ongoing series of interchangeable-media works by Donnie Martinez.

## The array

The dimensions are generated from the clock: **days × 24 hours × 12 five-minute slots**. In 2026 that is 365 × 24 × 12 = 105,120 cells; leap years become 366 automatically, and a new year rebuilds the array. Every day of the year is present, including the days before you first opened the program. Each cell has an address such as `[295][15][09]` (day, hour, slot).

## Views

- **Write**: a plain editor. A loop runs every second and checks each block of writing against its five-minute cell. When the cell seals, your next keystroke (after the current word) starts a new block in the next cell. Light timestamps appear the way chat apps mark time: a full date on a new day, otherwise a time after a seemingly arbitrary run of minutes or blocks.
- **Array · Data**: a heatmap of the year, a grid of one day's 288 cells, and an inspector for any cell.
- **Array · Field**: the whole array rendered as a live, rotating ASCII field. Drag to turn it; click a glyph to inspect its cell.

## What each cell computes

| Measure | Lineage | What it records |
|---|---|---|
| Instance | rebirth / saṃsāra | Every opening of the program is a new instance; cells record which instances passed through them |
| Pause | Lynch | The longest silence between keystrokes inside the cell |
| Excision | Cronenberg | Characters deleted from the cell's text |
| Edits from later | Philip K. Dick | Revisions made after the cell sealed; time in the array runs both ways |
| Cut-up | Burroughs & Gysin | Text from a day or the year, cut into short runs and reshuffled |
| Extinguish | nirvāṇa, "blowing out" | Removes a cell's text permanently and keeps its trace |

## Running it

It is one self-contained file. Open `index.html` in a browser, or serve the repo with GitHub Pages. Writing is saved in the browser's local storage, so it stays on that browser and device; use **Copy all text** to back it up.

---

# The local editions: one file per day

The same lattice idea, running in your terminal on real files. Here every dot is a **day**, and every day is a file you can open, write in, back up, grep, or sync.

The first time you run any edition in a folder, it checks whether it has been run there before. If not, it creates a folder named for the year with one file per day (365, or 366 in a leap year):

```
your-folder/
└── 2026/
    ├── .bardo              marks the folder as set up; records md or txt
    ├── 2026-01-01.md       # Thursday, 2026-01-01
    ├── 2026-01-02.md
    └── ... 2026-12-31.md
```

Each file starts with a header line naming the day. Anything you write below that line is the day's writing. Markdown (`.md`) is the default; pass `--txt` on the first run for plain text. **All four editions share this format**, so you can create the folder with one and browse it with another.

## The grid

```
    Jan Feb Mar  Apr May  Jun Jul Aug  Sep Oct Nov  Dec
Mon  oooooooooooooooooooooooooooooooooooooooooo..........
     ooooooooooooooooooooooooooooooooooooooooo#..........
Wed  ooooooooooooooooooooooooooooooooooooooooo+..........
    ooooooooooooooooooooooooooooooooooooooooooo..........
Fri oooooooooooooooooooooooooooooooooooooooooo@.........
    oooooooooooooooooooooooooooooooooooooooooo..........
Sun oooooooooooooooooooooooooooooooooooooooooo..........

@ today   . not yet   o passed, empty   + under 50 words   * under 250   # 250 or more
```

Seven rows (Monday to Sunday) by week columns. The glyph for each day comes from what is actually in its file.

## The four editions, from most to least complex

| Edition | Folder | What it does | Needs |
|---|---|---|---|
| **Python** | `python/bardo.py` | Grid browser with a preview of each day, the live rotating **3D ASCII field** (written days rise by word count), a cut-up of the year's text, stats and streak | Python 3.8+, standard library only |
| **C** | `c/` | Grid browser and the 3D field in plain POSIX C, with a Makefile, a test, `make install`, and a man page | a C compiler and `make` |
| **Bash** | `bash/bardo.sh` | Grid browser with arrow keys, preview, open in your editor | bash 4+, awk |
| **BASIC** | `basic/bardo.bas` | Menu-driven: show the grid, open a day by date or number, read a day, list written days | FreeBASIC |

Every edition opens days in the editor named by `$VISUAL` or `$EDITOR`, falling back to `nano`.

### Python

```bash
python3 python/bardo.py            # browse
python3 python/bardo.py print      # print the grid and exit
python3 python/bardo.py open today # open a day in your editor
```

Keys: arrows or `h j k l` move, `[` `]` jump a month, Enter opens, `t` today, `f` field, `c` cut-up, `r` rescan, `q` quit. In the field: arrows turn, `+` `-` zoom, space pauses the spin.

### C

```bash
cd c
make                          # build ./bardo
make check                    # test it in a throwaway folder
make install PREFIX=~/.local  # install bardo and its man page for your user
```

After installing, run `bardo` from any folder, and read `man bardo`.

### Bash

```bash
chmod +x bash/bardo.sh
./bash/bardo.sh
```

### BASIC

```bash
fbc basic/bardo.bas
./basic/bardo
```

Commands at the prompt: `T` today, `D 2026-03-14` open a date, `N 72` open day 72, `V 2026-03-14` read a day, `L` list written days, `R` rescan, `Q` quit.

All editions accept `print`, `--txt` (first run only) and `--year N`. Python, C and Bash also accept `--dir PATH` to keep the year folder somewhere other than the current folder.

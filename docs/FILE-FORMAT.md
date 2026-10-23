# The year folder format

All four local editions (Python, C, Bash, BASIC) read and write the same folder layout. This page is the specification. Anything that follows it, including a script of your own, can share a year with the editions.

## Layout

```
<root>/
└── <YEAR>/
    ├── .bardo
    ├── <YEAR>-01-01.<ext>
    ├── <YEAR>-01-02.<ext>
    ├── ...
    └── <YEAR>-12-31.<ext>
```

- `<root>` is the folder the program runs in, or the folder given with `--dir`.
- `<YEAR>` is four digits, for example `2026`.
- `<ext>` is `md` or `txt`, recorded in the marker.
- There is exactly one file per calendar day: 365, or 366 in a leap year. A year is a leap year when it is divisible by 4 and not by 100, or divisible by 400.

## The marker: `.bardo`

A plain-text file of `key=value` lines:

```
format=md
created=2026-10-23
by=bardo.py
```

| Key | Meaning |
|---|---|
| `format` | `md` or `txt`. Decides the extension of every day file. Required. |
| `created` | The date the folder was first set up. Informational. |
| `by` | Which edition set it up. Informational. |

Unknown keys are ignored.

## First run

A program runs "for the first time" in a folder when `<YEAR>/.bardo` does not exist. It then:

1. Creates `<YEAR>/` if needed.
2. Writes `.bardo` with `format=md`, or `format=txt` if the user passed `--txt`.
3. Creates every missing day file.

On later runs, if any day file is missing (for example, deleted by hand), it is recreated empty. Existing files are never overwritten.

## Day files

Each day file starts with a **header line**, followed by a blank line:

Markdown (`.md`):

```
# Friday, 2026-10-23

```

Plain text (`.txt`):

```
Friday, 2026-10-23

```

The weekday is the English name. The date is ISO 8601 (`YYYY-MM-DD`).

Everything after the first line is the day's **writing**. You may change or delete the header line; the editions always skip line 1 when counting, whatever it says.

## Counting

- **Words**: the number of runs of non-whitespace characters after line 1. Whitespace is space, tab, carriage return and newline. This matches `tail -n +2 file | wc -w` and Python's `str.split()`.
- A day is **written** if its word count is greater than zero.

## Glyphs

| Glyph | Condition (checked in this order) |
|---|---|
| `@` | the day is today |
| `+` | 1 to 49 words |
| `*` | 50 to 249 words |
| `#` | 250 or more words |
| `.` | no words, and the day is after today |
| `o` | no words, and the day is today or earlier (but not today, which is `@`) |

## Grid geometry

The grid has 7 rows, Monday (row 0) to Sunday (row 6), and one column per week.

- `offset` = weekday of January 1, with Monday = 0.
- For day index `i` (January 1 is 0): `row = (i + offset) mod 7`, `col = (i + offset) div 7`.
- Number of columns = `(days + offset + 6) div 7` (53 or 54).

## Stats

- **passed**: days on or before today.
- **written**: days with at least one word.
- **words**: total words across the year.
- **streak**: consecutive written days ending today. If today has nothing written yet, the streak counts back from yesterday, so an unwritten morning does not reset it.

## Weekday calculation

The C, Bash and BASIC editions use Sakamoto's method, converted to Monday = 0:

```
t = [0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4]
if month < 3: year = year - 1
w = (year + year/4 - year/100 + year/400 + t[month-1] + day) mod 7   # Sunday = 0
weekday = (w + 6) mod 7                                              # Monday = 0
```

(`/` is integer division.) Python uses `datetime.date.weekday()`, which gives the same result.

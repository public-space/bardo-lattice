#!/usr/bin/env python3
"""
bardo -- bardo lattice (Python edition)

The year as a grid of days. Every day is a file (a node) inside a folder
named for the year, created the first time you run the program in a
directory. Browse the grid, open any day in your editor, and watch the
whole year turn as a live 3D ASCII field.

Standard library only (curses ships with Python on Linux and macOS).

Usage:
    bardo.py                    interactive browser (grid, field, cut-up)
    bardo.py print              print the grid and stats, then exit
    bardo.py open [DATE|today]  open one day in $EDITOR
    bardo.py --txt              first run only: use .txt instead of .md
    bardo.py --year 2025        work on another year's folder
    bardo.py --dir PATH         use PATH instead of the current folder
"""

import argparse
import datetime as dt
import math
import os
import random
import shlex
import shutil
import subprocess
import sys
import time
try:
    import curses
except ImportError:          # Windows without windows-curses: print and open still work
    curses = None
from pathlib import Path

MARKER = ".bardo"
WEEKDAYS = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"]
MONTHS = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
LEGEND = "@ today   . not yet   o passed, empty   + under 50 words   * under 250   # 250 or more"


# --------------------------------------------------------------------------
# The year folder: shared format used by every version of bardo
# --------------------------------------------------------------------------
class Year:
    def __init__(self, root, year, want_txt=False):
        self.root = Path(root).resolve()
        self.year = year
        self.dir = self.root / str(year)
        self.marker = self.dir / MARKER
        self.first_run = False
        self.created_files = 0
        start = dt.date(year, 1, 1)
        self.days = [start + dt.timedelta(n) for n in range((dt.date(year + 1, 1, 1) - start).days)]
        self.offset = start.weekday()                      # Monday = 0
        self.ncols = (len(self.days) + self.offset + 6) // 7
        self.ext = "txt" if want_txt else "md"
        self.words = [0] * len(self.days)
        self.init()
        self.scan()

    # first run: make the folder, the marker, and one node per day
    def init(self):
        if self.marker.exists():
            for line in self.marker.read_text(encoding="utf-8", errors="replace").splitlines():
                if line.startswith("format="):
                    self.ext = "txt" if line.split("=", 1)[1].strip() == "txt" else "md"
        else:
            self.first_run = True
            self.dir.mkdir(parents=True, exist_ok=True)
            self.marker.write_text(
                f"format={self.ext}\ncreated={dt.date.today().isoformat()}\nby=bardo.py\n", encoding="utf-8")
        for d in self.days:
            p = self.path(d)
            if not p.exists():
                p.write_text(self.header(d) + "\n\n", encoding="utf-8")
                self.created_files += 1

    def path(self, d):
        return self.dir / f"{d.isoformat()}.{self.ext}"

    def header(self, d):
        line = f"{WEEKDAYS[d.weekday()]}, {d.isoformat()}"
        return "# " + line if self.ext == "md" else line

    def body(self, i):
        try:
            text = self.path(self.days[i]).read_text(encoding="utf-8", errors="replace")
        except OSError:
            return ""
        parts = text.split("\n", 1)
        return parts[1] if len(parts) > 1 else ""

    def scan(self):
        for i in range(len(self.days)):
            self.words[i] = len(self.body(i).split())

    # grid geometry: 7 rows (Mon..Sun) by ncols weeks
    def cell(self, i):
        pos = i + self.offset
        return pos % 7, pos // 7

    def index_at(self, row, col):
        i = col * 7 + row - self.offset
        return i if 0 <= i < len(self.days) else None

    def glyph(self, i, today):
        d, w = self.days[i], self.words[i]
        if d == today:
            return "@"
        if w > 0:
            return "+" if w < 50 else "*" if w < 250 else "#"
        return "." if d > today else "o"

    def state(self, i, today):
        d, w = self.days[i], self.words[i]
        if d == today:
            return "today"
        if w > 0:
            return "written"
        return "future" if d > today else "empty"

    def stats(self, today):
        written = sum(1 for w in self.words if w)
        total = sum(self.words)
        passed = sum(1 for d in self.days if d <= today)
        streak = 0
        if self.days[0] <= today <= self.days[-1]:
            i = (today - self.days[0]).days
            if self.words[i] == 0:
                i -= 1                                    # today may still be blank
            while i >= 0 and self.words[i] > 0:
                streak += 1
                i -= 1
        return dict(written=written, words=total, passed=min(passed, len(self.days)), streak=streak)


def open_in_editor(path):
    editor = os.environ.get("VISUAL") or os.environ.get("EDITOR")
    if not editor:
        editor = next((e for e in ("nano", "vim", "vi") if shutil.which(e)), "vi")
    try:
        subprocess.call(shlex.split(editor) + [str(path)])
    except FileNotFoundError:
        print(f"Could not start the editor '{editor}'. Set $EDITOR, for example: export EDITOR=nano", file=sys.stderr)
        time.sleep(2)


# --------------------------------------------------------------------------
# Plain printing (no curses), with ANSI color when it's a terminal
# --------------------------------------------------------------------------
def print_grid(y, today, out=sys.stdout):
    color = out.isatty() and not os.environ.get("NO_COLOR")
    paint = {
        "today": "\033[1;7m", "written": "\033[38;5;209m", "future": "\033[2m", "empty": "",
    }
    width = shutil.get_terminal_size((80, 24)).columns
    cw = 2 if width >= 5 + y.ncols * 2 else 1
    # month labels
    labels = [" "] * (y.ncols * cw + 2)
    last_end = -1
    for m in range(12):
        i = (dt.date(y.year, m + 1, 1) - y.days[0]).days
        x = y.cell(i)[1] * cw
        if x > last_end:
            for k, ch in enumerate(MONTHS[m]):
                if x + k < len(labels):
                    labels[x + k] = ch
            last_end = x + 3
    lines = ["    " + "".join(labels).rstrip()]
    for row in range(7):
        line = (WEEKDAYS[row][:3] + " ") if row % 2 == 0 else "    "
        for col in range(y.ncols):
            i = y.index_at(row, col)
            if i is None:
                line += " " * cw
                continue
            g = y.glyph(i, today)
            s = y.state(i, today)
            cellstr = g + (" " if cw == 2 else "")
            line += (paint[s] + g + "\033[0m" + cellstr[1:]) if color and paint[s] else cellstr
        lines.append(line.rstrip())
    s = y.stats(today)
    lines.append("")
    lines.append(LEGEND)
    lines.append(f"{y.year}: {len(y.days)} days · {s['passed']} passed · {s['written']} written · "
                 f"{s['words']:,} words · streak {s['streak']}")
    lines.append(f"folder: {y.dir}  (.{y.ext})")
    print("\n".join(lines), file=out)


# --------------------------------------------------------------------------
# Cut-up (Burroughs & Gysin): the year's own text, cut and reshuffled
# --------------------------------------------------------------------------
def cutup(texts, max_words=60):
    words = " ".join(texts).split()
    if len(words) < 4:
        return ""
    frags, i = [], 0
    while i < len(words):
        n = random.randint(2, 5)
        frags.append(" ".join(words[i:i + n]))
        i += n
    random.shuffle(frags)
    out, n = [], 0
    for f in frags:
        out.append(f)
        n += len(f.split())
        if n >= max_words:
            break
    return " / ".join(out)


# --------------------------------------------------------------------------
# Interactive browser (curses)
# --------------------------------------------------------------------------
class App:
    def __init__(self, scr, y):
        self.scr, self.y = scr, y
        self.today = dt.date.today()
        self.sel = self.default_sel()
        self.mode = "grid"
        self.msg = ""
        if y.first_run:
            self.msg = f"First run: created {y.dir.name}/ with {y.created_files} day files (.{y.ext})."
        self.cut = ""
        # field state
        self.a, self.b, self.zoom, self.spin = 0.7, -0.5, 1.0, True
        curses.curs_set(0)
        self.scr.keypad(True)
        self.color = curses.has_colors()
        if self.color:
            curses.start_color()
            try:
                curses.use_default_colors()
                bg = -1
            except curses.error:
                bg = curses.COLOR_BLACK
            curses.init_pair(1, curses.COLOR_RED, bg)        # written
            curses.init_pair(2, curses.COLOR_YELLOW, bg)     # today
            curses.init_pair(3, curses.COLOR_BLUE, bg)       # dim accents
            curses.init_pair(4, curses.COLOR_WHITE, bg)

    def default_sel(self):
        if self.y.days[0] <= self.today <= self.y.days[-1]:
            return (self.today - self.y.days[0]).days
        return 0

    def attr(self, state):
        if state == "today":
            return curses.A_BOLD | (curses.color_pair(2) if self.color else 0)
        if state == "written":
            return curses.A_BOLD | (curses.color_pair(1) if self.color else 0)
        if state == "future":
            return curses.A_DIM
        return curses.A_NORMAL

    def put(self, row, col, s, attr=0):
        h, w = self.scr.getmaxyx()
        if row < 0 or row >= h or col >= w or col < 0:
            return
        s = s[: max(0, w - col - (1 if row == h - 1 else 0))]
        try:
            self.scr.addstr(row, col, s, attr)
        except curses.error:
            pass

    # ---- grid view ----
    def draw_grid(self):
        y, today = self.y, self.today
        h, w = self.scr.getmaxyx()
        self.scr.erase()
        s = y.stats(today)
        title = f" bardo lattice · {y.year} "
        self.put(0, 0, title, curses.A_REVERSE | curses.A_BOLD)
        self.put(0, len(title) + 1, f"{len(y.days)} days · {s['passed']} passed · {s['written']} written · "
                        f"{s['words']:,} words · streak {s['streak']}", curses.A_DIM)
        cw = 2 if w >= 6 + y.ncols * 2 else 1
        top, left = 2, 4
        last_end = -1
        for m in range(12):
            i = (dt.date(y.year, m + 1, 1) - y.days[0]).days
            x = y.cell(i)[1] * cw
            if x > last_end:
                self.put(top, left + x, MONTHS[m], curses.A_DIM)
                last_end = x + 3
        for row in range(7):
            if row % 2 == 0:
                self.put(top + 1 + row, 0, WEEKDAYS[row][:3], curses.A_DIM)
            for col in range(y.ncols):
                i = y.index_at(row, col)
                if i is None:
                    continue
                a = self.attr(y.state(i, today))
                if i == self.sel:
                    a = curses.A_REVERSE | curses.A_BOLD
                self.put(top + 1 + row, left + col * cw, y.glyph(i, today), a)
        self.put(top + 9, 0, LEGEND, curses.A_DIM)
        # selected day
        d = y.days[self.sel]
        wds = y.words[self.sel]
        rel = y.path(d).relative_to(y.root)
        name = f"{WEEKDAYS[d.weekday()]}, {d.strftime('%B')} {d.day}, {d.year}"
        self.put(top + 11, 0, name, curses.A_BOLD)
        info = f"  {wds:,} words · {rel}" if wds else f"  {'not yet' if d > today else 'empty'} · {rel}"
        self.put(top + 11, len(name), info, curses.A_DIM)
        # preview
        body = [ln for ln in y.body(self.sel).splitlines()]
        while body and not body[0].strip():
            body.pop(0)
        room = h - (top + 13) - 2
        if body:
            for k, ln in enumerate(body[: max(0, room)]):
                self.put(top + 13 + k, 2, ln)
        else:
            self.put(top + 13, 2, "Press Enter to write in this day.", curses.A_DIM)
        if self.msg:
            self.put(h - 2, 0, self.msg, curses.A_BOLD)
        self.put(h - 1, 0, " arrows/hjkl move  [ ] month  enter open  t today  f field  c cut-up  r rescan  q quit",
                 curses.A_REVERSE)
        self.scr.refresh()

    def move(self, dr, dc):
        row, col = self.y.cell(self.sel)
        i = self.y.index_at(row + dr, col + dc)
        if i is not None:
            self.sel = i

    def jump_month(self, n):
        d = self.y.days[self.sel]
        m = d.month - 1 + n
        if 0 <= m < 12:
            day = min(d.day, 28)
            self.sel = (dt.date(self.y.year, m + 1, day) - self.y.days[0]).days

    def open_sel(self):
        curses.def_prog_mode()
        curses.endwin()
        open_in_editor(self.y.path(self.y.days[self.sel]))
        curses.reset_prog_mode()
        self.y.words[self.sel] = len(self.y.body(self.sel).split())
        self.scr.clear()
        self.msg = ""

    # ---- cut-up view ----
    def draw_cut(self):
        h, w = self.scr.getmaxyx()
        self.scr.erase()
        self.put(0, 0, " cut-up · the year's text, cut into short runs and reshuffled ", curses.A_REVERSE | curses.A_BOLD)
        text = self.cut or "The year holds too little text to cut. Write in a few days first."
        words, line, row = text.split(" "), "", 2
        for wd in words:
            if len(line) + len(wd) + 1 > min(w - 4, 72):
                self.put(row, 2, line, self.attr("written") if self.cut else curses.A_DIM)
                row, line = row + 1, ""
            line = (line + " " + wd).strip()
        self.put(row, 2, line, self.attr("written") if self.cut else curses.A_DIM)
        self.put(h - 1, 0, " c cut again  g back to grid  q quit", curses.A_REVERSE)
        self.scr.refresh()

    def new_cut(self):
        texts = [self.y.body(i) for i in range(len(self.y.days)) if self.y.words[i]]
        self.cut = cutup(texts)

    # ---- field view: the live 3D ASCII year, one dot per day ----
    def draw_field(self, t):
        y, today = self.y, self.today
        h, w = self.scr.getmaxyx()
        rows, cols = h - 2, w
        if rows < 5 or cols < 20:
            return
        depth = [1e9] * (rows * cols)
        buf = [None] * (rows * cols)
        ca, sa, cb, sb = math.cos(self.a), math.sin(self.a), math.cos(self.b), math.sin(self.b)
        scale = min(cols * 0.5, rows * 1.05) * 0.62 * self.zoom
        cx, cy, dist = cols / 2, rows / 2 + 1, 4.2
        maxw = max(y.words) or 1

        def plot(x, yy, z, g, state, bias=0.0):
            x1 = x * ca + z * sa
            z1 = -x * sa + z * ca
            y1 = yy * cb - z1 * sb
            z2 = yy * sb + z1 * cb + bias
            f = dist / (dist + z2)
            c = int(cx + x1 * f * scale * 2.0)          # characters are about twice as tall as wide
            r = int(cy + y1 * f * scale)
            if 0 <= r < rows and 0 <= c < cols:
                k = r * cols + c
                if z2 < depth[k]:
                    depth[k] = z2
                    buf[k] = (g, state)

        for i, d in enumerate(y.days):
            row, col = y.cell(i)
            x = (col / (y.ncols - 1) - 0.5) * 3.2
            z = (row / 6 - 0.5) * 0.62
            st = y.state(i, today)
            plot(x, 0.0, z, y.glyph(i, today), st)
            if y.words[i]:                               # written days rise by how much was written
                hgt = 0.12 + 0.9 * math.log1p(y.words[i]) / math.log1p(maxw)
                steps = max(1, int(hgt / 0.07))
                for s in range(1, steps):
                    plot(x, -hgt * s / steps, z, "|", "written")
                plot(x, -hgt, z, y.glyph(i, today), st, -0.02)
        # the selected day hovers as a cursor
        row, col = y.cell(self.sel)
        plot((col / (y.ncols - 1) - 0.5) * 3.2, -1.25, (row / 6 - 0.5) * 0.62, "v", "today")

        self.scr.erase()
        scan = -1 if os.environ.get("BARDO_STILL") else int(t * 22) % (rows + 30)
        blink = int(t * 2) % 2 == 0
        for k, cellv in enumerate(buf):
            if not cellv:
                continue
            g, st = cellv
            r, c = divmod(k, cols)
            a = self.attr(st)
            if st == "today" and g == "@" and blink:
                a |= curses.A_REVERSE
            if r == scan:
                a |= curses.A_BOLD
            self.put(r + 1, c, g, a)
        s = y.stats(today)
        d = y.days[self.sel]
        self.put(0, 0, f" field · {y.year} · {len(y.days)} days · {s['written']} written · {s['words']:,} words ",
                 curses.A_REVERSE | curses.A_BOLD)
        self.put(0, 64, f"v {d.isoformat()} {y.words[self.sel]:,}w", curses.A_DIM)
        self.put(h - 1, 0, " arrows turn  +/- zoom  space spin  hjkl move cursor  enter open  g grid  q quit",
                 curses.A_REVERSE)
        self.scr.refresh()

    def read_key(self):
        """getch, plus decoding of raw arrow sequences that some terminals send untranslated."""
        k = self.scr.getch()
        if k != 27:
            return k
        self.scr.nodelay(True)
        seq = ""
        for _ in range(3):
            c = self.scr.getch()
            if c == -1:
                break
            seq += chr(c) if 0 <= c < 256 else ""
            if seq[-1:].isalpha() or seq[-1:] == "~":
                break
        self.scr.nodelay(False)
        arrows = {"[A": curses.KEY_UP, "[B": curses.KEY_DOWN, "[C": curses.KEY_RIGHT, "[D": curses.KEY_LEFT,
                  "OA": curses.KEY_UP, "OB": curses.KEY_DOWN, "OC": curses.KEY_RIGHT, "OD": curses.KEY_LEFT}
        return arrows.get(seq, 27)

    # ---- main loop ----
    def run(self):
        last = time.time()
        while True:
            if self.mode == "grid":
                self.scr.timeout(-1)
                self.draw_grid()
            elif self.mode == "cut":
                self.scr.timeout(-1)
                self.draw_cut()
            else:
                self.scr.timeout(33)
                now = time.time()
                if self.spin:
                    self.a += (now - last) * 0.35
                last = now
                self.draw_field(now)
            k = self.read_key()
            if k == -1:
                continue
            ch = chr(k) if 0 <= k < 256 else ""
            if ch in ("q", "Q"):
                return
            if self.mode == "grid":
                if k in (curses.KEY_LEFT,) or ch == "h": self.move(0, -1)
                elif k in (curses.KEY_RIGHT,) or ch == "l": self.move(0, 1)
                elif k in (curses.KEY_UP,) or ch == "k": self.move(-1, 0)
                elif k in (curses.KEY_DOWN,) or ch == "j": self.move(1, 0)
                elif ch == "[": self.jump_month(-1)
                elif ch == "]": self.jump_month(1)
                elif k in (10, 13, curses.KEY_ENTER) or ch == "o": self.open_sel()
                elif ch == "t": self.sel = self.default_sel()
                elif ch == "r": self.y.scan(); self.msg = "Rescanned the folder."
                elif ch == "f": self.mode = "field"; last = time.time()
                elif ch == "c": self.new_cut(); self.mode = "cut"
            elif self.mode == "cut":
                if ch == "c": self.new_cut()
                elif ch in ("g", "\x1b") or k in (10, 13): self.mode = "grid"
            else:
                if k == curses.KEY_LEFT: self.a -= 0.12
                elif k == curses.KEY_RIGHT: self.a += 0.12
                elif k == curses.KEY_UP: self.b = max(-1.5, self.b - 0.1)
                elif k == curses.KEY_DOWN: self.b = min(1.5, self.b + 0.1)
                elif ch in ("+", "="): self.zoom = min(4.0, self.zoom * 1.2)
                elif ch in ("-", "_"): self.zoom = max(0.3, self.zoom / 1.2)
                elif ch == " ": self.spin = not self.spin
                elif ch == "h": self.move(0, -1)
                elif ch == "l": self.move(0, 1)
                elif ch == "k": self.move(-1, 0)
                elif ch == "j": self.move(1, 0)
                elif k in (10, 13, curses.KEY_ENTER): self.open_sel()
                elif ch in ("g", "\x1b"): self.mode = "grid"


def main():
    ap = argparse.ArgumentParser(prog="bardo", description="bardo lattice: the year as a grid of day files")
    ap.add_argument("command", nargs="?", default="browse", choices=["browse", "print", "open"])
    ap.add_argument("date", nargs="?", help="for 'open': YYYY-MM-DD or 'today' (default today)")
    ap.add_argument("--txt", action="store_true", help="first run only: make .txt day files instead of .md")
    ap.add_argument("--year", type=int, default=dt.date.today().year, help="which year folder (default this year)")
    ap.add_argument("--dir", default=".", help="folder to keep the year folder in (default: current folder)")
    a = ap.parse_args()

    y = Year(a.dir, a.year, a.txt)
    if a.txt and not y.first_run and y.ext != "txt":
        print(f"Note: {y.dir.name}/ already uses .{y.ext}; --txt only applies on the first run.", file=sys.stderr)
    today = dt.date.today()

    if a.command == "print":
        if y.first_run:
            print(f"First run: created {y.dir} with {y.created_files} day files (.{y.ext}).\n")
        print_grid(y, today)
        return
    if a.command == "open":
        try:
            d = today if a.date in (None, "today") else dt.date.fromisoformat(a.date)
        except ValueError:
            sys.exit(f"Not a date: {a.date}. Use YYYY-MM-DD, for example {today.isoformat()}.")
        if d.year != y.year:
            y = Year(a.dir, d.year, a.txt)
        open_in_editor(y.path(d))
        return
    if not sys.stdin.isatty() or not sys.stdout.isatty():
        print_grid(y, today)
        return
    if curses is None:
        print_grid(y, today)
        return
    os.environ.setdefault("ESCDELAY", "25")
    curses.wrapper(lambda scr: App(scr, y).run())


if __name__ == "__main__":
    main()

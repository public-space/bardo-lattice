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

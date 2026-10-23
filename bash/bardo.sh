#!/usr/bin/env bash
#
# bardo -- bardo lattice (Bash edition)
#
# The year as a grid of days. The first run in a folder creates a
# subfolder named for the year with one file (a node) per day.
# Browse the grid with the arrow keys and open any day in $EDITOR.
#
# Needs bash 4 or newer (for associative arrays) and awk.
#
#   ./bardo.sh                    interactive browser
#   ./bardo.sh print              print the grid and stats
#   ./bardo.sh open [DATE|today]  open one day in $EDITOR
#   ./bardo.sh --txt              first run only: .txt instead of .md
#   ./bardo.sh --year 2025        another year's folder
#   ./bardo.sh --dir PATH         keep the year folder in PATH

set -u

if (( BASH_VERSINFO[0] < 4 )); then
  echo "bardo: needs bash 4 or newer (you have $BASH_VERSION)" >&2
  exit 1
fi

WEEKDAYS=(Monday Tuesday Wednesday Thursday Friday Saturday Sunday)
MONTHS=(January February March April May June July August September October November December)
LEGEND="@ today   . not yet   o passed, empty   + under 50 words   * under 250   # 250 or more"

# ---------- arguments ----------
CMD=browse; OPEN_DATE=""; WANT_TXT=0; ROOT="."; YEAR=$(date +%Y)
while (( $# )); do
  case "$1" in
    print|open) CMD=$1 ;;
    --txt) WANT_TXT=1 ;;
    --year) YEAR=$2; shift ;;
    --dir) ROOT=$2; shift ;;
    -h|--help) sed -n '3,17p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) if [[ $CMD == open && -z $OPEN_DATE ]]; then OPEN_DATE=$1
       else echo "bardo: unknown argument '$1'" >&2; exit 2; fi ;;
  esac
  shift
done
if [[ $CMD == open && -n $OPEN_DATE && $OPEN_DATE != today ]]; then
  if [[ ! $OPEN_DATE =~ ^([0-9]{4})-([0-9]{2})-([0-9]{2})$ ]]; then
    echo "bardo: not a date: $OPEN_DATE (use YYYY-MM-DD)" >&2; exit 2
  fi
  YEAR=${BASH_REMATCH[1]}
fi
ROOT=$(cd "$ROOT" 2>/dev/null && pwd) || { echo "bardo: no such folder" >&2; exit 1; }
YDIR="$ROOT/$YEAR"
TODAY=$(date +%Y-%m-%d)

# ---------- calendar arithmetic (no external date calls per day) ----------
is_leap() { (( ($1 % 4 == 0 && $1 % 100 != 0) || $1 % 400 == 0 )); }
month_len() {
  local l=(31 28 31 30 31 30 31 31 30 31 30 31)
  if (( $2 == 2 )) && is_leap "$1"; then echo 29; else echo "${l[$2-1]}"; fi
}
weekday() {   # Sakamoto; prints Monday=0
  local y=$1 m=$2 d=$3 t=(0 3 2 5 0 3 5 1 4 6 2 4)
  (( m < 3 )) && (( y -= 1 ))
  echo $(( ( (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7 + 6 ) % 7 ))
}

DATES=(); WD=(); MONTH_START=()
for (( m = 1; m <= 12; m++ )); do
  MONTH_START+=(${#DATES[@]})
  ml=$(month_len "$YEAR" "$m")
  for (( d = 1; d <= ml; d++ )); do
    DATES+=("$(printf '%04d-%02d-%02d' "$YEAR" "$m" "$d")")
  done
done
N=${#DATES[@]}
OFFSET=$(weekday "$YEAR" 1 1)
for (( i = 0; i < N; i++ )); do WD+=($(( (i + OFFSET) % 7 ))); done
NCOLS=$(( (N + OFFSET + 6) / 7 ))
TODAY_I=-1
for (( i = 0; i < N; i++ )); do [[ ${DATES[i]} == "$TODAY" ]] && TODAY_I=$i; done

# ---------- first run: folder, marker, one node per day ----------
EXT=md; FIRST_RUN=0; CREATED=0
(( WANT_TXT )) && EXT=txt
if [[ -f $YDIR/.bardo ]]; then
  grep -q '^format=txt' "$YDIR/.bardo" && EXT=txt || EXT=md
else
  FIRST_RUN=1
  mkdir -p "$YDIR" || exit 1
  printf 'format=%s\ncreated=%s\nby=bardo.sh\n' "$EXT" "$TODAY" > "$YDIR/.bardo"
fi
for (( i = 0; i < N; i++ )); do
  f="$YDIR/${DATES[i]}.$EXT"
  if [[ ! -e $f ]]; then
    if [[ $EXT == md ]]; then printf '# %s, %s\n\n' "${WEEKDAYS[WD[i]]}" "${DATES[i]}" > "$f"
    else printf '%s, %s\n\n' "${WEEKDAYS[WD[i]]}" "${DATES[i]}" > "$f"; fi
    (( CREATED++ ))
  fi
done

# ---------- counting words: one awk pass, then every glyph computed once ----------
declare -A BYNAME
W=(); G=(); P=(); STATS=""
scan() {
  BYNAME=()
  local name count i w written=0 total=0 passed=0 streak=0
  while read -r name count; do
    BYNAME[$name]=$count
  done < <(awk 'FNR == 1 { next } { w[FILENAME] += NF } END { for (f in w) { n = f; sub(/.*\//, "", n); print n, w[f] } }' \
             "$YDIR"/*."$EXT" 2>/dev/null)
  for (( i = 0; i < N; i++ )); do
    w=${BYNAME[${DATES[i]}.$EXT]:-0}
    W[i]=$w
    if (( i == TODAY_I )); then G[i]='@'; P[i]=$'\033[1;38;5;220m'
    elif (( w > 0 )); then
      if (( w < 50 )); then G[i]='+'; elif (( w < 250 )); then G[i]='*'; else G[i]='#'; fi
      P[i]=$'\033[1;38;5;209m'
    elif [[ ${DATES[i]} > $TODAY ]]; then G[i]='.'; P[i]=$'\033[2m'
    else G[i]='o'; P[i]=''; fi
    (( w > 0 )) && (( written++ ))
    (( total += w ))
    [[ ${DATES[i]} > $TODAY ]] || (( passed++ ))
  done
  if (( TODAY_I >= 0 )); then
    i=$TODAY_I
    (( W[i] == 0 )) && (( i-- ))
    while (( i >= 0 )) && (( W[i] > 0 )); do (( streak++, i-- )); done
  fi
  STATS="$YEAR: $N days · $passed passed · $written written · $total words · streak $streak"
}

# build the 7 grid rows into GRID_LINES; SEL marks the selected day
render_grid() {
  local color=$1 sel=$2 width cw row col i g line labels x last=-1 m
  width=${COLUMNS_NOW:-80}
  cw=1; (( width >= 5 + NCOLS * 2 )) && cw=2
  PADC=" "; (( cw == 2 )) && PADC="  "
  labels=""
  for (( m = 0; m < 12; m++ )); do
    x=$(( (MONTH_START[m] + OFFSET) / 7 * cw ))
    if (( x > last )); then
      while (( ${#labels} < x )); do labels+=" "; done
      labels+=${MONTHS[m]:0:3}; last=$(( x + 3 ))
    fi
  done
  GRID_LINES=("    $labels")
  for (( row = 0; row < 7; row++ )); do
    if (( row % 2 == 0 )); then line="${WEEKDAYS[row]:0:3} "; else line="    "; fi
    for (( col = 0; col < NCOLS; col++ )); do
      i=$(( col * 7 + row - OFFSET ))
      if (( i < 0 || i >= N )); then line+=${PADC}; continue; fi
      g=${G[i]}
      if (( i == sel )); then line+=$'\033[1;7m'"$g"$'\033[0m'
      elif (( color )) && [[ -n ${P[i]} ]]; then line+="${P[i]}$g"$'\033[0m'
      else line+=$g; fi
      (( cw == 2 )) && line+=" "
    done
    GRID_LINES+=("$line")
  done
}

print_grid() {
  local color=0
  COLUMNS_NOW=$(tput cols 2>/dev/null || echo 80)
  [[ -t 1 && -z ${NO_COLOR:-} ]] && color=1
  render_grid "$color" -1
  printf '%s\n' "${GRID_LINES[@]}"
  printf '\n%s\n%s\nfolder: %s  (.%s)\n' "$LEGEND" "$STATS" "$YDIR" "$EXT"
}

open_day() {
  local f="$YDIR/${DATES[$1]}.$EXT" ed=${VISUAL:-${EDITOR:-}}
  if [[ -z $ed ]]; then command -v nano >/dev/null && ed=nano || ed=vi; fi
  # shellcheck disable=SC2086  # let $EDITOR carry its own arguments
  $ed "$f"
}

# ---------- interactive browser ----------
browse() {
  local sel=$(( TODAY_I >= 0 ? TODAY_I : 0 )) msg="" key rest rows cols i w d r c ni body
  (( FIRST_RUN )) && msg="First run: created $YEAR/ with $CREATED day files (.$EXT)."
  restore() { printf '\033[0m\033[?25h'; tput rmcup 2>/dev/null; stty echo 2>/dev/null; }
  trap 'restore; exit' INT TERM
  trap restore EXIT
  tput smcup 2>/dev/null; printf '\033[?25l'
  while :; do
    rows=$(tput lines 2>/dev/null || echo 24); cols=$(tput cols 2>/dev/null || echo 80); COLUMNS_NOW=$cols
    render_grid 1 "$sel"
    w=${W[sel]}
    d=${DATES[sel]}
    printf '\033[H\033[2J\033[1;7m bardo lattice · %s \033[0m \033[2m%s\033[0m\n\n' "$YEAR" "${STATS#*: }"
    printf '%s\n' "${GRID_LINES[@]}"
    printf '\n\033[2m%s\033[0m\n\n' "$LEGEND"
    printf '\033[1m%s, %s %d, %s\033[0m  \033[2m' "${WEEKDAYS[WD[sel]]}" "${MONTHS[10#${d:5:2}-1]}" "$((10#${d:8:2}))" "$YEAR"
    if (( w > 0 )); then printf '%s words' "$w"; elif [[ $d > $TODAY ]]; then printf 'not yet'; else printf 'empty'; fi
    printf ' · %s/%s.%s\033[0m\n\n' "$YEAR" "$d" "$EXT"
    body=$(tail -n +2 "$YDIR/$d.$EXT" 2>/dev/null | sed '/./,$!d' | head -n $(( rows - 18 > 0 ? rows - 18 : 0 )) | cut -c1-$(( cols - 3 )))
    if [[ -n $body ]]; then printf '%s\n' "$body" | sed 's/^/  /'; else printf '  \033[2mPress Enter to write in this day.\033[0m\n'; fi
    [[ -n $msg ]] && printf '\033[%d;1H\033[1m%s\033[0m' $(( rows - 1 )) "$msg"
    printf '\033[%d;1H\033[7m%s\033[0m' "$rows" " arrows/hjkl move  [ ] month  enter open  t today  r rescan  q quit "
    msg=""
    IFS= read -rsn1 key
    if [[ $key == $'\e' ]]; then IFS= read -rsn2 -t 0.05 rest; key+=$rest; fi
    r=$(( (sel + OFFSET) % 7 )); c=$(( (sel + OFFSET) / 7 ))
    case $key in
      q|Q) break ;;
      $'\e[D'|$'\eOD'|h) ni=$(( (c - 1) * 7 + r - OFFSET )) ;;
      $'\e[C'|$'\eOC'|l) ni=$(( (c + 1) * 7 + r - OFFSET )) ;;
      $'\e[A'|$'\eOA'|k) (( r > 0 )) && ni=$(( sel - 1 )) || ni=-1 ;;
      $'\e[B'|$'\eOB'|j) (( r < 6 )) && ni=$(( sel + 1 )) || ni=-1 ;;
      '[') i=$(( 10#${d:5:2} - 2 )); (( i >= 0 )) && ni=$(( MONTH_START[i] + (10#${d:8:2} < 28 ? 10#${d:8:2} : 28) - 1 )) || ni=-1 ;;
      ']') i=$(( 10#${d:5:2} )); (( i < 12 )) && ni=$(( MONTH_START[i] + (10#${d:8:2} < 28 ? 10#${d:8:2} : 28) - 1 )) || ni=-1 ;;
      t) ni=$TODAY_I ;;
      r) scan; msg="Rescanned the folder."; ni=-1 ;;
      ''|o) restore; open_day "$sel"; scan; tput smcup 2>/dev/null; printf '\033[?25l'; ni=-1 ;;
      *) ni=-1 ;;
    esac
    (( ni >= 0 && ni < N )) && sel=$ni
  done
}

scan
case $CMD in
  print)
    (( FIRST_RUN )) && printf 'First run: created %s with %d day files (.%s).\n\n' "$YDIR" "$CREATED" "$EXT"
    print_grid ;;
  open)
    i=$TODAY_I
    if [[ -n $OPEN_DATE && $OPEN_DATE != today ]]; then
      i=-1; for (( k = 0; k < N; k++ )); do [[ ${DATES[k]} == "$OPEN_DATE" ]] && i=$k; done
    fi
    (( i < 0 )) && { echo "bardo: no such day in $YEAR" >&2; exit 2; }
    open_day "$i" ;;
  *)
    if [[ -t 0 && -t 1 ]]; then browse; else print_grid; fi ;;
esac

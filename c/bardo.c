/*
 * bardo -- bardo lattice (C edition)
 *
 * The year as a grid of days. The first run in a folder creates a
 * subfolder named for the year, holding one file (a node) per day.
 * Browse the grid, open any day in $EDITOR, and watch the year turn
 * as a live 3D ASCII field.
 *
 * Plain POSIX C99: termios for the keyboard, ANSI escape codes for the
 * screen. No ncurses needed. Build with `make`, install with `make install`.
 */
#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define VERSION "1.0"
#define MARKER ".bardo"

static const char *WEEKDAYS[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
static const char *MONTHS[] = {"January", "February", "March", "April", "May", "June", "July",
                               "August", "September", "October", "November", "December"};
static const char *LEGEND =
    "@ today   . not yet   o passed, empty   + under 50 words   * under 250   # 250 or more";

/* ------------------------------------------------------------------ */
/* the year                                                            */
/* ------------------------------------------------------------------ */
typedef struct {
    int m, d, wd;   /* month 1-12, day 1-31, weekday Monday=0 */
    int words;
} Day;

static Day days[366];
static int ndays, offset, ncols, year;
static int today_i = -1;          /* index of today in this year, or -1 */
static int today_cmp_base;        /* yyyymmdd of today */
static char ext[4] = "md";
static char root[PATH_MAX], ydir[PATH_MAX];
static int first_run, created;

static int is_leap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }
static int mdays(int y, int m) {
    static const int t[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return m == 2 && is_leap(y) ? 29 : t[m - 1];
}
/* Sakamoto's method; converted so Monday = 0 */
static int weekday(int y, int m, int d) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) y -= 1;
    int w = (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7; /* Sunday = 0 */
    return (w + 6) % 7;
}
static int ymd(int i) { return year * 10000 + days[i].m * 100 + days[i].d; }

static void day_path(int i, char *buf, size_t n) {
    if (snprintf(buf, n, "%s/%04d-%02d-%02d.%s", ydir, year, days[i].m, days[i].d, ext) >= (int)n) {
        fprintf(stderr, "bardo: path too long\n");
        exit(1);
    }
}

static void build_days(void) {
    ndays = 0;
    for (int m = 1; m <= 12; m++)
        for (int d = 1; d <= mdays(year, m); d++) {
            days[ndays].m = m;
            days[ndays].d = d;
            days[ndays].wd = weekday(year, m, d);
            days[ndays].words = 0;
            ndays++;
        }
    offset = days[0].wd;
    ncols = (ndays + offset + 6) / 7;
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    today_cmp_base = (lt->tm_year + 1900) * 10000 + (lt->tm_mon + 1) * 100 + lt->tm_mday;
    today_i = -1;
    for (int i = 0; i < ndays; i++)
        if (ymd(i) == today_cmp_base) today_i = i;
}

static void cell_of(int i, int *row, int *col) { *row = (i + offset) % 7; *col = (i + offset) / 7; }
static int index_at(int row, int col) {
    if (row < 0 || row > 6 || col < 0) return -1;
    int i = col * 7 + row - offset;
    return (i >= 0 && i < ndays) ? i : -1;
}

/* first run: folder, marker, one node per day */
static void init_year(int want_txt) {
    char path[PATH_MAX], line[256];
    if (snprintf(ydir, sizeof ydir, "%s/%d", root, year) >= (int)sizeof ydir ||
        snprintf(path, sizeof path, "%s/%s", ydir, MARKER) >= (int)sizeof path) {
        fprintf(stderr, "bardo: path too long\n");
        exit(1);
    }
    if (want_txt) strcpy(ext, "txt");
    FILE *f = fopen(path, "r");
    if (f) {
        while (fgets(line, sizeof line, f))
            if (strncmp(line, "format=", 7) == 0) strcpy(ext, strncmp(line + 7, "txt", 3) == 0 ? "txt" : "md");
        fclose(f);
    } else {
        first_run = 1;
        if (mkdir(ydir, 0755) != 0 && errno != EEXIST) {
            fprintf(stderr, "bardo: cannot create %s: %s\n", ydir, strerror(errno));
            exit(1);
        }
        f = fopen(path, "w");
        if (!f) { fprintf(stderr, "bardo: cannot write %s: %s\n", path, strerror(errno)); exit(1); }
        fprintf(f, "format=%s\ncreated=%04d-%02d-%02d\nby=bardo (C)\n", ext,
                today_cmp_base / 10000, today_cmp_base / 100 % 100, today_cmp_base % 100);
        fclose(f);
    }
    for (int i = 0; i < ndays; i++) {
        day_path(i, path, sizeof path);
        if (access(path, F_OK) == 0) continue;
        f = fopen(path, "w");
        if (!f) continue;
        fprintf(f, "%s%s, %04d-%02d-%02d\n\n", strcmp(ext, "md") == 0 ? "# " : "",
                WEEKDAYS[days[i].wd], year, days[i].m, days[i].d);
        fclose(f);
        created++;
    }
}

/* words after the first (header) line */
static int count_words(int i) {
    char path[PATH_MAX];
    day_path(i, path, sizeof path);
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    int c, words = 0, in = 0, past_header = 0;
    while ((c = fgetc(f)) != EOF) {
        if (!past_header) { if (c == '\n') past_header = 1; continue; }
        if (isspace(c)) in = 0;
        else if (!in) { in = 1; words++; }
    }
    fclose(f);
    return words;
}
static void scan(void) { for (int i = 0; i < ndays; i++) days[i].words = count_words(i); }

enum { ST_EMPTY, ST_FUTURE, ST_WRITTEN, ST_TODAY };
static int state(int i) {
    if (i == today_i) return ST_TODAY;
    if (days[i].words > 0) return ST_WRITTEN;
    return ymd(i) > today_cmp_base ? ST_FUTURE : ST_EMPTY;
}
static char glyph(int i) {
    int w = days[i].words;
    if (i == today_i) return '@';
    if (w > 0) return w < 50 ? '+' : w < 250 ? '*' : '#';
    return ymd(i) > today_cmp_base ? '.' : 'o';
}

typedef struct { int written, words, passed, streak; } Stats;
static Stats stats(void) {
    Stats s = {0, 0, 0, 0};
    for (int i = 0; i < ndays; i++) {
        if (days[i].words) s.written++;
        s.words += days[i].words;
        if (ymd(i) <= today_cmp_base) s.passed++;
    }
    if (today_i >= 0) {
        int i = today_i;
        if (days[i].words == 0) i--;
        while (i >= 0 && days[i].words > 0) { s.streak++; i--; }
    }
    return s;
}

static void term_size(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) { *rows = ws.ws_row; *cols = ws.ws_col; }
    else { *rows = 24; *cols = 80; }
}

static const char *color_of(int st) {
    switch (st) {
    case ST_TODAY: return "\033[1;38;5;220m";
    case ST_WRITTEN: return "\033[1;38;5;209m";
    case ST_FUTURE: return "\033[2m";
    default: return "";
    }
}

/* ------------------------------------------------------------------ */
/* print mode                                                           */
/* ------------------------------------------------------------------ */
static void print_grid(void) {
    int rows, cols;
    term_size(&rows, &cols);
    int color = isatty(STDOUT_FILENO) && !getenv("NO_COLOR");
    int cw = cols >= 5 + ncols * 2 ? 2 : 1;
    char labels[256];
    memset(labels, ' ', sizeof labels);
    int last_end = -1, maxx = 0;
    for (int m = 0; m < 12; m++) {
        int i = 0;
        for (int k = 0; k < m; k++) i += mdays(year, k + 1);
        int r, c;
        cell_of(i, &r, &c);
        int x = c * cw;
        if (x > last_end && x + 3 < (int)sizeof labels) {
            memcpy(labels + x, MONTHS[m], 3);
            last_end = x + 3;
            maxx = x + 3;
        }
    }
    labels[maxx] = '\0';
    printf("    %s\n", labels);
    for (int row = 0; row < 7; row++) {
        printf("%s", row % 2 == 0 ? (char[]){WEEKDAYS[row][0], WEEKDAYS[row][1], WEEKDAYS[row][2], ' ', 0} : "    ");
        for (int col = 0; col < ncols; col++) {
            int i = index_at(row, col);
            if (i < 0) { printf("%*s", cw, ""); continue; }
            const char *cc = color ? color_of(state(i)) : "";
            printf("%s%c%s%s", cc, glyph(i), *cc ? "\033[0m" : "", cw == 2 ? " " : "");
        }
        putchar('\n');
    }
    Stats s = stats();
    printf("\n%s\n%d: %d days · %d passed · %d written · %d words · streak %d\nfolder: %s  (.%s)\n",
           LEGEND, year, ndays, s.passed, s.written, s.words, s.streak, ydir, ext);
}

/* ------------------------------------------------------------------ */
/* terminal handling                                                    */
/* ------------------------------------------------------------------ */
static struct termios orig_tio;
static int raw_on;

static void raw_off(void) {
    if (!raw_on) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_tio);
    fputs("\033[0m\033[?25h\033[?1049l", stdout);
    fflush(stdout);
    raw_on = 0;
}
static void raw_enter(void) {
    struct termios t;
    tcgetattr(STDIN_FILENO, &orig_tio);
    t = orig_tio;
    t.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
    t.c_iflag &= ~(IXON | ICRNL);
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
    fputs("\033[?1049h\033[?25l", stdout);
    fflush(stdout);
    raw_on = 1;
}
static void on_signal(int sig) { raw_off(); signal(sig, SIG_DFL); raise(sig); }

enum { K_NONE = -1, K_UP = 1000, K_DOWN, K_LEFT, K_RIGHT, K_ENTER };
static int read_key(int timeout_ms) {
    struct pollfd p = {STDIN_FILENO, POLLIN, 0};
    if (poll(&p, 1, timeout_ms) <= 0) return K_NONE;
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return K_NONE;
    if (c == '\r' || c == '\n') return K_ENTER;
    if (c != 27) return c;
    unsigned char s[2];
    if (poll(&p, 1, 30) <= 0 || read(STDIN_FILENO, &s[0], 1) != 1) return 27;
    if (poll(&p, 1, 30) <= 0 || read(STDIN_FILENO, &s[1], 1) != 1) return 27;
    if (s[0] == '[' || s[0] == 'O') {
        switch (s[1]) {
        case 'A': return K_UP;
        case 'B': return K_DOWN;
        case 'C': return K_RIGHT;
        case 'D': return K_LEFT;
        }
    }
    return 27;
}

/* output buffer: build the frame, write once */
static char *ob;
static size_t ob_len, ob_cap;
static void ob_add(const char *fmt, ...) {
    va_list ap;
    for (;;) {
        va_start(ap, fmt);
        int n = vsnprintf(ob ? ob + ob_len : NULL, ob ? ob_cap - ob_len : 0, fmt, ap);
        va_end(ap);
        if (n < 0) return;
        if (ob && ob_len + (size_t)n < ob_cap) { ob_len += (size_t)n; return; }
        ob_cap = (ob_cap + (size_t)n + 1) * 2;
        ob = realloc(ob, ob_cap);
        if (!ob) exit(1);
    }
}
static void ob_flush(void) { if (ob_len) { fwrite(ob, 1, ob_len, stdout); fflush(stdout); } ob_len = 0; }
static void at(int row, int col) { ob_add("\033[%d;%dH", row + 1, col + 1); }

static void open_day(int i) {
    char path[PATH_MAX];
    day_path(i, path, sizeof path);
    const char *ed = getenv("VISUAL");
    if (!ed || !*ed) ed = getenv("EDITOR");
    if (!ed || !*ed) ed = access("/usr/bin/nano", X_OK) == 0 ? "nano" : "vi";
    int was_raw = raw_on;
    raw_off();
    pid_t pid = fork();
    if (pid == 0) {
        /* let the shell split $EDITOR so values like "code -w" work */
        execl("/bin/sh", "sh", "-c", "exec $0 \"$1\"", ed, path, (char *)NULL);
        _exit(127);
    }
    if (pid > 0) {
        int st;
        while (waitpid(pid, &st, 0) < 0 && errno == EINTR) {}
    }
    days[i].words = count_words(i);
    if (was_raw) raw_enter();
}

/* ------------------------------------------------------------------ */
/* grid view                                                            */
/* ------------------------------------------------------------------ */
static void draw_grid(int sel, const char *msg) {
    int rows, cols;
    term_size(&rows, &cols);
    Stats s = stats();
    ob_add("\033[0m\033[2J");
    at(0, 0);
    ob_add("\033[1;7m bardo lattice · %d \033[0m \033[2m%d days · %d passed · %d written · %d words · streak %d\033[0m",
           year, ndays, s.passed, s.written, s.words, s.streak);
    int cw = cols >= 6 + ncols * 2 ? 2 : 1, top = 2, left = 4, last_end = -1;
    for (int m = 0; m < 12; m++) {
        int i = 0;
        for (int k = 0; k < m; k++) i += mdays(year, k + 1);
        int r, c;
        cell_of(i, &r, &c);
        if (c * cw > last_end) { at(top, left + c * cw); ob_add("\033[2m%.3s\033[0m", MONTHS[m]); last_end = c * cw + 3; }
    }
    for (int row = 0; row < 7; row++) {
        if (row % 2 == 0) { at(top + 1 + row, 0); ob_add("\033[2m%.3s\033[0m", WEEKDAYS[row]); }
        for (int col = 0; col < ncols; col++) {
            int i = index_at(row, col);
            if (i < 0 || left + col * cw >= cols) continue;
            at(top + 1 + row, left + col * cw);
            if (i == sel) ob_add("\033[1;7m%c\033[0m", glyph(i));
            else ob_add("%s%c\033[0m", color_of(state(i)), glyph(i));
        }
    }
    at(top + 9, 0);
    ob_add("\033[2m%.*s\033[0m", cols, LEGEND);
    char path[PATH_MAX];
    day_path(sel, path, sizeof path);
    at(top + 11, 0);
    ob_add("\033[1m%s, %s %d, %d\033[0m  \033[2m", WEEKDAYS[days[sel].wd], MONTHS[days[sel].m - 1], days[sel].d, year);
    if (days[sel].words) ob_add("%d words", days[sel].words);
    else ob_add("%s", state(sel) == ST_FUTURE ? "not yet" : "empty");
    ob_add(" · %d/%04d-%02d-%02d.%s\033[0m", year, year, days[sel].m, days[sel].d, ext);
    /* preview of the day's text */
    FILE *f = fopen(path, "r");
    int row = top + 13, shown = 0;
    if (f) {
        char line[1024];
        int first = 1, started = 0;
        while (row < rows - 2 && fgets(line, sizeof line, f)) {
            if (first) { first = 0; continue; }
            line[strcspn(line, "\n")] = '\0';
            if (!started && line[0] == '\0') continue;
            started = 1;
            at(row++, 2);
            ob_add("%.*s", cols - 3 > 0 ? cols - 3 : 0, line);
            shown++;
        }
        fclose(f);
    }
    if (!shown) { at(top + 13, 2); ob_add("\033[2mPress Enter to write in this day.\033[0m"); }
    if (msg && *msg) { at(rows - 2, 0); ob_add("\033[1m%.*s\033[0m", cols, msg); }
    at(rows - 1, 0);
    ob_add("\033[7m%.*s\033[0m", cols - 1,
           " arrows/hjkl move  [ ] month  enter open  t today  f field  r rescan  q quit                    ");
    ob_flush();
}

/* ------------------------------------------------------------------ */
/* field view: the live 3D ASCII year, one dot per day                  */
/* ------------------------------------------------------------------ */
static double fa = 0.7, fb = -0.5, fzoom = 1.0;
static float *zb;
static char *gb;
static unsigned char *sb;
static int fcap;

static int frows, fcols;
static double ca_, sa_, cb_, sb_, fscale, fcx, fcy;
static void fplot(double x, double y, double z, char g, int st, double bias) {
    const double dist = 4.2;
    double x1 = x * ca_ + z * sa_, z1 = -x * sa_ + z * ca_;
    double y1 = y * cb_ - z1 * sb_, z2 = y * sb_ + z1 * cb_ + bias;
    double f = dist / (dist + z2);
    int c = (int)(fcx + x1 * f * fscale * 2.0), r = (int)(fcy + y1 * f * fscale);
    if (r < 0 || r >= frows || c < 0 || c >= fcols) return;
    int k = r * fcols + c;
    if (z2 < zb[k]) { zb[k] = (float)z2; gb[k] = g; sb[k] = (unsigned char)st; }
}

static void draw_field(int sel, double t) {
    int rows, cols;
    term_size(&rows, &cols);
    frows = rows - 2;
    fcols = cols;
    if (frows < 5 || fcols < 20) return;
    int n = frows * fcols;
    if (n > fcap) {
        fcap = n;
        zb = realloc(zb, sizeof *zb * n);
        gb = realloc(gb, (size_t)n);
        sb = realloc(sb, (size_t)n);
        if (!zb || !gb || !sb) exit(1);
    }
    for (int k = 0; k < n; k++) { zb[k] = 1e9f; gb[k] = 0; }
    ca_ = cos(fa); sa_ = sin(fa); cb_ = cos(fb); sb_ = sin(fb);
    double a = fcols * 0.5, b = frows * 1.05;
    fscale = (a < b ? a : b) * 0.62 * fzoom;
    fcx = fcols / 2.0;
    fcy = frows / 2.0 + 1;
    int maxw = 1;
    for (int i = 0; i < ndays; i++) if (days[i].words > maxw) maxw = days[i].words;
    for (int i = 0; i < ndays; i++) {
        int row, col;
        cell_of(i, &row, &col);
        double x = ((double)col / (ncols - 1) - 0.5) * 3.2, z = (row / 6.0 - 0.5) * 0.62;
        fplot(x, 0, z, glyph(i), state(i), 0);
        if (days[i].words) {
            double h = 0.12 + 0.9 * log1p(days[i].words) / log1p(maxw);
            int steps = (int)(h / 0.07);
            if (steps < 1) steps = 1;
            for (int s = 1; s < steps; s++) fplot(x, -h * s / steps, z, '|', ST_WRITTEN, 0);
            fplot(x, -h, z, glyph(i), state(i), -0.02);
        }
    }
    int row, col;
    cell_of(sel, &row, &col);
    fplot(((double)col / (ncols - 1) - 0.5) * 3.2, -1.25, (row / 6.0 - 0.5) * 0.62, 'v', ST_TODAY, 0);

    int scan = getenv("BARDO_STILL") ? -1 : (int)(t * 22) % (frows + 30);
    int blink = ((int)(t * 2)) % 2 == 0;
    Stats s = stats();
    ob_add("\033[0m\033[H\033[1;7m field · %d · %d days · %d written · %d words \033[0m\033[K", year, ndays, s.written, s.words);
    ob_add("  \033[2mv %04d-%02d-%02d %dw\033[0m", year, days[sel].m, days[sel].d, days[sel].words);
    for (int r = 0; r < frows; r++) {
        at(r + 1, 0);
        int cur = -1;
        for (int c = 0; c < fcols; c++) {
            int k = r * fcols + c;
            if (!gb[k]) {
                if (cur != -1) { ob_add("\033[0m"); cur = -1; }
                ob_add(" ");
                continue;
            }
            int st = sb[k] + (r == scan ? 10 : 0) + (gb[k] == '@' && blink ? 20 : 0);
            if (st != cur) {
                ob_add("\033[0m%s%s%s", color_of(sb[k]), r == scan ? "\033[1m" : "", gb[k] == '@' && blink ? "\033[7m" : "");
                cur = st;
            }
            ob_add("%c", gb[k]);
        }
        ob_add("\033[0m");
    }
    at(rows - 1, 0);
    ob_add("\033[7m%.*s\033[0m", cols - 1,
           " arrows turn  +/- zoom  space spin  hjkl move cursor  enter open  g grid  q quit              ");
    ob_flush();
}

static double now_s(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void move_sel(int *sel, int dr, int dc) {
    int r, c;
    cell_of(*sel, &r, &c);
    int i = index_at(r + dr, c + dc);
    if (i >= 0) *sel = i;
}
static void jump_month(int *sel, int dir) {
    int m = days[*sel].m + dir;
    if (m < 1 || m > 12) return;
    int d = days[*sel].d < 28 ? days[*sel].d : 28, i = 0;
    for (int k = 1; k < m; k++) i += mdays(year, k);
    *sel = i + d - 1;
}

static void browse(void) {
    char msg[256] = "";
    if (first_run) snprintf(msg, sizeof msg, "First run: created %d/ with %d day files (.%s).", year, created, ext);
    int sel = today_i >= 0 ? today_i : 0, mode = 0, spin = 1;
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    atexit(raw_off);
    raw_enter();
    double last = now_s();
    for (;;) {
        int k;
        if (mode == 0) {
            draw_grid(sel, msg);
            k = read_key(-1);
            msg[0] = '\0';
            if (k == 'q' || k == 'Q' || k == 3) break;
            else if (k == K_LEFT || k == 'h') move_sel(&sel, 0, -1);
            else if (k == K_RIGHT || k == 'l') move_sel(&sel, 0, 1);
            else if (k == K_UP || k == 'k') move_sel(&sel, -1, 0);
            else if (k == K_DOWN || k == 'j') move_sel(&sel, 1, 0);
            else if (k == '[') jump_month(&sel, -1);
            else if (k == ']') jump_month(&sel, 1);
            else if (k == K_ENTER || k == 'o') open_day(sel);
            else if (k == 't' && today_i >= 0) sel = today_i;
            else if (k == 'r') { scan(); strcpy(msg, "Rescanned the folder."); }
            else if (k == 'f') { mode = 1; last = now_s(); ob_add("\033[2J"); }
        } else {
            double t = now_s();
            if (spin) fa += (t - last) * 0.35;
            last = t;
            draw_field(sel, t);
            k = read_key(33);
            if (k == K_NONE) continue;
            if (k == 'q' || k == 'Q' || k == 3) break;
            else if (k == K_LEFT) fa -= 0.12;
            else if (k == K_RIGHT) fa += 0.12;
            else if (k == K_UP) fb = fb - 0.1 < -1.5 ? -1.5 : fb - 0.1;
            else if (k == K_DOWN) fb = fb + 0.1 > 1.5 ? 1.5 : fb + 0.1;
            else if (k == '+' || k == '=') fzoom = fzoom * 1.2 > 4 ? 4 : fzoom * 1.2;
            else if (k == '-' || k == '_') fzoom = fzoom / 1.2 < 0.3 ? 0.3 : fzoom / 1.2;
            else if (k == ' ') spin = !spin;
            else if (k == 'h') move_sel(&sel, 0, -1);
            else if (k == 'l') move_sel(&sel, 0, 1);
            else if (k == 'k') move_sel(&sel, -1, 0);
            else if (k == 'j') move_sel(&sel, 1, 0);
            else if (k == K_ENTER) open_day(sel);
            else if (k == 'g' || k == 27) mode = 0;
        }
    }
    raw_off();
}

static void usage(FILE *f) {
    fprintf(f,
            "usage: bardo [print | open [YYYY-MM-DD|today]] [--txt] [--year N] [--dir PATH]\n\n"
            "  (no command)   interactive browser: grid and live 3D field\n"
            "  print          print the grid and stats, then exit\n"
            "  open [DATE]    open one day in $EDITOR (default: today)\n"
            "  --txt          first run only: make .txt day files instead of .md\n"
            "  --year N       work on another year's folder (default: this year)\n"
            "  --dir PATH     keep the year folder in PATH (default: current folder)\n"
            "  --version      print the version\n");
}

int main(int argc, char **argv) {
    const char *cmd = "browse", *date = NULL, *dir = ".";
    int want_txt = 0;
    time_t now = time(NULL);
    year = localtime(&now)->tm_year + 1900;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--txt")) want_txt = 1;
        else if (!strcmp(argv[i], "--year") && i + 1 < argc) year = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--dir") && i + 1 < argc) dir = argv[++i];
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) { usage(stdout); return 0; }
        else if (!strcmp(argv[i], "--version")) { puts("bardo " VERSION); return 0; }
        else if (!strcmp(argv[i], "print") || !strcmp(argv[i], "open")) cmd = argv[i];
        else if (!strcmp(cmd, "open") && !date) date = argv[i];
        else { fprintf(stderr, "bardo: unknown argument '%s'\n\n", argv[i]); usage(stderr); return 2; }
    }
    int oy = 0, om = 0, od = 0;
    if (!strcmp(cmd, "open") && date && strcmp(date, "today")) {
        if (sscanf(date, "%d-%d-%d", &oy, &om, &od) != 3 || om < 1 || om > 12 || od < 1 || od > mdays(oy, om)) {
            fprintf(stderr, "bardo: not a date: %s (use YYYY-MM-DD)\n", date);
            return 2;
        }
        year = oy;
    }
    if (year < 1 || year > 9999) { fprintf(stderr, "bardo: year out of range\n"); return 2; }
    if (!realpath(dir, root)) { fprintf(stderr, "bardo: no such folder: %s\n", dir); return 1; }

    build_days();
    init_year(want_txt);
    scan();

    if (!strcmp(cmd, "print")) {
        if (first_run) printf("First run: created %s with %d day files (.%s).\n\n", ydir, created, ext);
        print_grid();
        return 0;
    }
    if (!strcmp(cmd, "open")) {
        int i = today_i;
        if (oy) for (int k = 0; k < ndays; k++) if (days[k].m == om && days[k].d == od) i = k;
        if (i < 0) { fprintf(stderr, "bardo: today is not in %d; give a date\n", year); return 2; }
        open_day(i);
        return 0;
    }
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) { print_grid(); return 0; }
    browse();
    return 0;
}

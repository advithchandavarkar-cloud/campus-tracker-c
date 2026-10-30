/*
 * ============================================================
 *  CAMPUS TRACKER - Interactive Student Management System
 * ============================================================
 *  Features:
 *   - Colourful boxed terminal UI with live status messages
 *   - Safe, validated input (re-prompts instead of failing)
 *   - Search by roll number OR partial name (case-insensitive)
 *   - Update with "press Enter to keep current value"
 *   - Delete with confirmation
 *   - Sort by roll / name / marks
 *   - Analytics dashboard: average, pass rate, grade histogram,
 *     and a top-3 leaderboard
 *   - Export a printable report to report.txt
 *   - Auto-save to students.txt
 *
 *  Build:  gcc -Wall -o student_manager student_manager.c
 *  Run:    ./student_manager
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_STUDENTS 100
#define NAME_LEN     50
#define FILE_NAME    "students.txt"
#define REPORT_NAME  "report.txt"
#define PASS_MARK    40.0f

/* ---------- ANSI colours ---------- */
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"

struct Student {
    int   rollNo;
    char  name[NAME_LEN];
    int   age;
    float marks;
};

/* ---------- Prototypes ---------- */
void  enableColors(void);
void  banner(void);
void  line(char c, int n);
void  ok(const char *msg);
void  err(const char *msg);
void  info(const char *msg);
void  pause(void);

void  trim(char *s);
int   readLine(const char *prompt, char *buf, int size);
int   readInt(const char *prompt, int min, int max, int *out);
int   readFloat(const char *prompt, float min, float max, float *out);
int   askYesNo(const char *prompt);
int   containsIgnoreCase(const char *text, const char *pattern);

void  loadStudents(struct Student s[], int *count);
void  saveStudents(const struct Student s[], int count);
int   findStudent(const struct Student s[], int count, int rollNo);
char  calculateGrade(float marks);
const char *gradeColor(char g);
void  printBar(float marks);
void  printHeader(void);
void  printRow(const struct Student *st);

void  addStudent(struct Student s[], int *count);
void  displayStudents(const struct Student s[], int count);
void  searchStudent(const struct Student s[], int count);
void  updateStudent(struct Student s[], int count);
void  deleteStudent(struct Student s[], int *count);
void  sortStudents(struct Student s[], int count);
void  dashboard(const struct Student s[], int count);
void  exportReport(const struct Student s[], int count);

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(void) {
    struct Student students[MAX_STUDENTS];
    int count = 0;
    int choice = -1;

    enableColors();
    loadStudents(students, &count);

    while (choice != 0) {
        banner();
        printf(" " DIM "%d student record(s) loaded" RESET "\n\n", count);
        printf("  " CYAN "[1]" RESET " Add student\n");
        printf("  " CYAN "[2]" RESET " View all students\n");
        printf("  " CYAN "[3]" RESET " Search (roll no. or name)\n");
        printf("  " CYAN "[4]" RESET " Update student\n");
        printf("  " CYAN "[5]" RESET " Delete student\n");
        printf("  " CYAN "[6]" RESET " Sort records\n");
        printf("  " MAGENTA "[7]" RESET " Analytics dashboard\n");
        printf("  " MAGENTA "[8]" RESET " Export report\n");
        printf("  " RED "[0]" RESET " Save & exit\n\n");

        if (!readInt("  Choose an option", 0, 8, &choice)) {
            choice = -1;
            continue;
        }

        printf("\n");
        switch (choice) {
            case 1: addStudent(students, &count);      break;
            case 2: displayStudents(students, count);  break;
            case 3: searchStudent(students, count);    break;
            case 4: updateStudent(students, count);    break;
            case 5: deleteStudent(students, &count);   break;
            case 6: sortStudents(students, count);     break;
            case 7: dashboard(students, count);        break;
            case 8: exportReport(students, count);     break;
            case 0:
                saveStudents(students, count);
                printf(GREEN BOLD "  Data saved. See you next time!" RESET "\n\n");
                break;
        }
        if (choice != 0) pause();
    }
    return 0;
}

/* ============================================================
 *  UI HELPERS
 * ============================================================ */
void enableColors(void) {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | 0x0004); /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
#endif
}

void line(char c, int n) {
    int i;
    for (i = 0; i < n; i++) putchar(c);
    putchar('\n');
}

void banner(void) {
    printf("\n" CYAN BOLD);
    printf("  +==========================================+\n");
    printf("  |      CAMPUS TRACKER  -  Student Hub      |\n");
    printf("  |   manage  .  analyse  .  celebrate       |\n");
    printf("  +==========================================+\n");
    printf(RESET "\n");
}

void ok(const char *msg)   { printf(GREEN  "  [OK] %s" RESET "\n", msg); }
void err(const char *msg)  { printf(RED    "  [!!] %s" RESET "\n", msg); }
void info(const char *msg) { printf(YELLOW "  [..] %s" RESET "\n", msg); }

void pause(void) {
    char tmp[8];
    printf("\n  " DIM "Press Enter to return to the menu..." RESET);
    fflush(stdout);
    fgets(tmp, sizeof tmp, stdin);
}

/* ============================================================
 *  INPUT HELPERS (fgets-based, no leftover-buffer bugs)
 * ============================================================ */
void trim(char *s) {
    char *start = s;
    size_t len;
    while (isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
}

int readLine(const char *prompt, char *buf, int size) {
    printf("%s: ", prompt);
    fflush(stdout);
    if (fgets(buf, size, stdin) == NULL) return 0;
    if (strchr(buf, '\n') == NULL) {          /* line was too long: flush rest */
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) { }
    }
    buf[strcspn(buf, "\n")] = '\0';
    trim(buf);
    return 1;
}

int readInt(const char *prompt, int min, int max, int *out) {
    char buf[64], extra;
    int v, tries;
    for (tries = 0; tries < 3; tries++) {
        char p[128];
        snprintf(p, sizeof p, "%s (%d-%d)", prompt, min, max);
        if (!readLine(p, buf, sizeof buf)) return 0;
        if (sscanf(buf, "%d %c", &v, &extra) == 1 && v >= min && v <= max) {
            *out = v;
            return 1;
        }
        err("Invalid number, try again.");
    }
    return 0;
}

int readFloat(const char *prompt, float min, float max, float *out) {
    char buf[64], extra;
    float v;
    int tries;
    for (tries = 0; tries < 3; tries++) {
        char p[128];
        snprintf(p, sizeof p, "%s (%.0f-%.0f)", prompt, min, max);
        if (!readLine(p, buf, sizeof buf)) return 0;
        if (sscanf(buf, "%f %c", &v, &extra) == 1 && v >= min && v <= max) {
            *out = v;
            return 1;
        }
        err("Invalid value, try again.");
    }
    return 0;
}

int askYesNo(const char *prompt) {
    char buf[16];
    printf(YELLOW "  %s (y/n): " RESET, prompt);
    fflush(stdout);
    if (fgets(buf, sizeof buf, stdin) == NULL) return 0;
    return tolower((unsigned char)buf[0]) == 'y';
}

int containsIgnoreCase(const char *text, const char *pattern) {
    size_t n = strlen(text), m = strlen(pattern), i, j;
    if (m == 0) return 1;
    for (i = 0; i + m <= n; i++) {
        for (j = 0; j < m; j++)
            if (tolower((unsigned char)text[i + j]) !=
                tolower((unsigned char)pattern[j])) break;
        if (j == m) return 1;
    }
    return 0;
}

/* ============================================================
 *  FILE HANDLING
 * ============================================================ */
void loadStudents(struct Student s[], int *count) {
    FILE *f = fopen(FILE_NAME, "r");
    char buf[200];
    if (!f) return;

    while (*count < MAX_STUDENTS && fgets(buf, sizeof buf, f)) {
        struct Student *st = &s[*count];
        if (sscanf(buf, "%d,%49[^,],%d,%f",
                   &st->rollNo, st->name, &st->age, &st->marks) == 4)
            (*count)++;
    }
    fclose(f);
}

void saveStudents(const struct Student s[], int count) {
    FILE *f = fopen(FILE_NAME, "w");
    int i;
    if (!f) { err("Could not save records."); return; }
    for (i = 0; i < count; i++)
        fprintf(f, "%d,%s,%d,%.2f\n",
                s[i].rollNo, s[i].name, s[i].age, s[i].marks);
    fclose(f);
}

/* ============================================================
 *  CORE LOGIC / DISPLAY
 * ============================================================ */
int findStudent(const struct Student s[], int count, int rollNo) {
    int i;
    for (i = 0; i < count; i++)
        if (s[i].rollNo == rollNo) return i;
    return -1;
}

char calculateGrade(float m) {
    if (m >= 90) return 'A';
    if (m >= 75) return 'B';
    if (m >= 60) return 'C';
    if (m >= 40) return 'D';
    return 'F';
}

const char *gradeColor(char g) {
    switch (g) {
        case 'A': return GREEN BOLD;
        case 'B': return GREEN;
        case 'C': return YELLOW;
        case 'D': return MAGENTA;
        default:  return RED;
    }
}

void printBar(float marks) {
    int filled = (int)(marks / 5.0f + 0.5f), i;   /* 20-cell bar */
    char g = calculateGrade(marks);
    printf("%s[", gradeColor(g));
    for (i = 0; i < 20; i++) putchar(i < filled ? '#' : '.');
    printf("]" RESET);
}

void printHeader(void) {
    printf("\n" BOLD "  %-6s %-22s %-4s %-7s %-5s %s" RESET "\n",
           "Roll", "Name", "Age", "Marks", "Grade", "Performance");
    printf("  ");
    line('-', 72);
}

void printRow(const struct Student *st) {
    char g = calculateGrade(st->marks);
    printf("  %-6d %-22s %-4d %-7.2f %s%-5c" RESET " ",
           st->rollNo, st->name, st->age, st->marks, gradeColor(g), g);
    printBar(st->marks);
    printf("\n");
}

/* ============================================================
 *  FEATURES
 * ============================================================ */
void addStudent(struct Student s[], int *count) {
    struct Student n;
    int roll, i;

    printf(BOLD "  -- ADD NEW STUDENT --" RESET "\n\n");
    if (*count >= MAX_STUDENTS) { err("Student list is full."); return; }

    if (!readInt("  Roll number", 1, 999999, &roll)) { err("Cancelled."); return; }
    if (findStudent(s, *count, roll) != -1) {
        err("That roll number already exists.");
        return;
    }
    n.rollNo = roll;

    do {
        if (!readLine("  Name", n.name, NAME_LEN)) return;
        if (n.name[0] == '\0') err("Name cannot be empty.");
    } while (n.name[0] == '\0');
    for (i = 0; n.name[i]; i++) if (n.name[i] == ',') n.name[i] = ' ';

    if (!readInt("  Age", 5, 100, &n.age))            { err("Cancelled."); return; }
    if (!readFloat("  Marks", 0, 100, &n.marks))      { err("Cancelled."); return; }

    s[(*count)++] = n;
    saveStudents(s, *count);

    printf("\n");
    ok("Student added and saved!");
    printf("  %s%s%s scored %.2f -> Grade %s%c" RESET "\n",
           BOLD, n.name, RESET, n.marks, gradeColor(calculateGrade(n.marks)),
           calculateGrade(n.marks));
    if (n.marks >= 90) printf(GREEN BOLD "  *** Outstanding performance! ***" RESET "\n");
    else if (n.marks < PASS_MARK) info("Heads up: this student is below the pass mark.");
}

void displayStudents(const struct Student s[], int count) {
    int i;
    printf(BOLD "  -- ALL STUDENTS (%d) --" RESET "\n", count);
    if (count == 0) { info("No student records yet. Add one first!"); return; }
    printHeader();
    for (i = 0; i < count; i++) printRow(&s[i]);
}

void searchStudent(const struct Student s[], int count) {
    char q[NAME_LEN];
    int i, found = 0;

    printf(BOLD "  -- SEARCH --" RESET "\n\n");
    if (count == 0) { info("No records to search."); return; }

    if (!readLine("  Enter roll number OR part of a name", q, sizeof q)) return;
    if (q[0] == '\0') { err("Empty search."); return; }

    {   /* numeric query -> exact roll lookup */
        char *end;
        long r = strtol(q, &end, 10);
        if (*end == '\0') {
            int idx = findStudent(s, count, (int)r);
            if (idx != -1) {
                printHeader();
                printRow(&s[idx]);
                return;
            }
        }
    }

    for (i = 0; i < count; i++) {
        if (containsIgnoreCase(s[i].name, q)) {
            if (!found) printHeader();
            printRow(&s[i]);
            found++;
        }
    }
    if (!found) err("No matching student found.");
    else printf("\n  " DIM "%d match(es)." RESET "\n", found);
}

void updateStudent(struct Student s[], int count) {
    char buf[64];
    int roll, idx, i;
    struct Student *st;

    printf(BOLD "  -- UPDATE STUDENT --" RESET "\n\n");
    if (!readInt("  Roll number to update", 1, 999999, &roll)) return;
    idx = findStudent(s, count, roll);
    if (idx == -1) { err("Student not found."); return; }
    st = &s[idx];

    info("Press Enter on any field to keep its current value.");
    printf("\n");

    printf("  Name  [%s]\n", st->name);
    if (readLine("  New name", buf, NAME_LEN) && buf[0]) {
        for (i = 0; buf[i]; i++) if (buf[i] == ',') buf[i] = ' ';
        strcpy(st->name, buf);
    }

    printf("  Age   [%d]\n", st->age);
    if (readLine("  New age", buf, sizeof buf) && buf[0]) {
        int v;
        if (sscanf(buf, "%d", &v) == 1 && v >= 5 && v <= 100) st->age = v;
        else err("Invalid age - kept old value.");
    }

    printf("  Marks [%.2f]\n", st->marks);
    if (readLine("  New marks", buf, sizeof buf) && buf[0]) {
        float v;
        if (sscanf(buf, "%f", &v) == 1 && v >= 0 && v <= 100) st->marks = v;
        else err("Invalid marks - kept old value.");
    }

    saveStudents(s, count);
    printf("\n");
    ok("Record updated!");
    printHeader();
    printRow(st);
}

void deleteStudent(struct Student s[], int *count) {
    int roll, idx, i;

    printf(BOLD "  -- DELETE STUDENT --" RESET "\n\n");
    if (!readInt("  Roll number to delete", 1, 999999, &roll)) return;
    idx = findStudent(s, *count, roll);
    if (idx == -1) { err("Student not found."); return; }

    printHeader();
    printRow(&s[idx]);
    printf("\n");
    if (!askYesNo("Really delete this record?")) { info("Deletion cancelled."); return; }

    for (i = idx; i < *count - 1; i++) s[i] = s[i + 1];
    (*count)--;
    saveStudents(s, *count);
    ok("Record deleted.");
}

/* ---------- Sorting ---------- */
static int cmpRoll(const void *a, const void *b) {
    return ((const struct Student *)a)->rollNo - ((const struct Student *)b)->rollNo;
}
static int cmpMarksDesc(const void *a, const void *b) {
    float x = ((const struct Student *)a)->marks;
    float y = ((const struct Student *)b)->marks;
    return (x < y) - (x > y);
}
static int cmpName(const void *a, const void *b) {
    const char *x = ((const struct Student *)a)->name;
    const char *y = ((const struct Student *)b)->name;
    while (*x && *y && tolower((unsigned char)*x) == tolower((unsigned char)*y)) { x++; y++; }
    return tolower((unsigned char)*x) - tolower((unsigned char)*y);
}

void sortStudents(struct Student s[], int count) {
    int opt;
    printf(BOLD "  -- SORT RECORDS --" RESET "\n\n");
    if (count < 2) { info("Need at least 2 records to sort."); return; }

    printf("  [1] By roll number\n  [2] By name (A-Z)\n  [3] By marks (high to low)\n\n");
    if (!readInt("  Sort by", 1, 3, &opt)) return;

    qsort(s, count, sizeof(struct Student),
          opt == 1 ? cmpRoll : opt == 2 ? cmpName : cmpMarksDesc);
    saveStudents(s, count);
    ok("Sorted!");
    displayStudents(s, count);
}

/* ---------- Dashboard ---------- */
void dashboard(const struct Student s[], int count) {
    int i, j, passed = 0, dist[5] = {0}, maxBin = 1;
    float sum = 0, hi, lo;
    const char labels[5] = {'A', 'B', 'C', 'D', 'F'};
    const char *medal[3] = {"1st", "2nd", "3rd"};
    int order[MAX_STUDENTS];

    printf(BOLD MAGENTA "  == ANALYTICS DASHBOARD ==" RESET "\n\n");
    if (count == 0) { info("No data to analyse yet."); return; }

    hi = lo = s[0].marks;
    for (i = 0; i < count; i++) {
        float m = s[i].marks;
        char g = calculateGrade(m);
        sum += m;
        if (m > hi) hi = m;
        if (m < lo) lo = m;
        if (m >= PASS_MARK) passed++;
        for (j = 0; j < 5; j++) if (labels[j] == g) dist[j]++;
    }
    for (j = 0; j < 5; j++) if (dist[j] > maxBin) maxBin = dist[j];

    printf("  Students     : " BOLD "%d" RESET "\n", count);
    printf("  Average marks: " BOLD "%.2f" RESET "\n", sum / count);
    printf("  Highest      : " GREEN BOLD "%.2f" RESET "\n", hi);
    printf("  Lowest       : " RED BOLD "%.2f" RESET "\n", lo);
    printf("  Pass rate    : " BOLD "%.1f%%" RESET " (%d of %d)\n\n",
           100.0f * passed / count, passed, count);

    printf(BOLD "  Grade distribution" RESET "\n");
    for (j = 0; j < 5; j++) {
        int bars = dist[j] * 25 / maxBin, k;
        printf("   %s%c" RESET " | %s", gradeColor(labels[j]), labels[j], gradeColor(labels[j]));
        for (k = 0; k < bars; k++) putchar('#');
        printf(RESET " %d\n", dist[j]);
    }

    /* top 3 leaderboard */
    for (i = 0; i < count; i++) order[i] = i;
    for (i = 0; i < count - 1; i++)
        for (j = 0; j < count - i - 1; j++)
            if (s[order[j]].marks < s[order[j + 1]].marks) {
                int t = order[j]; order[j] = order[j + 1]; order[j + 1] = t;
            }

    printf("\n" BOLD YELLOW "  Leaderboard" RESET "\n");
    for (i = 0; i < count && i < 3; i++)
        printf("   %s%s" RESET "  %-22s %6.2f   (roll %d)\n",
               i == 0 ? YELLOW BOLD : BOLD, medal[i],
               s[order[i]].name, s[order[i]].marks, s[order[i]].rollNo);
}

/* ---------- Export ---------- */
void exportReport(const struct Student s[], int count) {
    FILE *f;
    int i;
    float sum = 0;

    printf(BOLD "  -- EXPORT REPORT --" RESET "\n\n");
    if (count == 0) { info("Nothing to export."); return; }

    f = fopen(REPORT_NAME, "w");
    if (!f) { err("Could not create report file."); return; }

    fprintf(f, "STUDENT PERFORMANCE REPORT\n==========================\n\n");
    fprintf(f, "%-8s %-24s %-5s %-8s %s\n", "Roll", "Name", "Age", "Marks", "Grade");
    fprintf(f, "--------------------------------------------------------\n");
    for (i = 0; i < count; i++) {
        fprintf(f, "%-8d %-24s %-5d %-8.2f %c\n",
                s[i].rollNo, s[i].name, s[i].age, s[i].marks,
                calculateGrade(s[i].marks));
        sum += s[i].marks;
    }
    fprintf(f, "\nTotal students: %d\nClass average : %.2f\n", count, sum / count);
    fclose(f);

    ok("Report saved to " REPORT_NAME);
}

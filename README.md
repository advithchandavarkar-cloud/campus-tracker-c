# Campus Tracker: Student Management System in C

Campus Tracker is an interactive, menu-driven student record manager that runs in the terminal. It is written in plain C and uses structs, file handling and input validation. It goes beyond basic CRUD with a colorful interface, visual performance bars and a built-in analytics dashboard.

## Features

| Feature | Description |
|---|---|
| Add student | Validated entry of roll number, name, age and marks, with duplicate roll-number detection |
| View all | Table of all records with grades and a visual performance bar |
| Search | Look up by roll number, or by part of a name (case-insensitive) |
| Update | Edit a record, pressing Enter to keep any current value |
| Delete | Removal with a y/n confirmation step |
| Sort | By roll number, name (A-Z) or marks (high to low) |
| Analytics dashboard | Class average, highest and lowest marks, pass rate, grade histogram and top-3 leaderboard |
| Export report | Generates a printable `report.txt` |
| Auto-save | Records persist in `students.txt` after every change |

## Grading scale

| Marks | Grade |
|---|---|
| 90 and above | A |
| 75 - 89 | B |
| 60 - 74 | C |
| 40 - 59 | D |
| Below 40 | F |

## Getting started

**Requirements:** a C compiler (GCC or Clang).

```bash
git clone https://github.com/YOUR-USERNAME/campus-tracker-c.git
cd campus-tracker-c
gcc -Wall -o student_manager student_manager.c
./student_manager
```

On Windows 10 and later the colors are enabled automatically.

## Concepts demonstrated

- Structures (`struct`) and arrays of structures
- File I/O (`fopen`, `fgets`, `fprintf`, `sscanf`)
- Sorting with `qsort` and custom comparator functions
- Robust input handling with `fgets`, with re-prompting on invalid input
- String processing, including trimming and case-insensitive substring search
- Modular design with one function per feature
- ANSI escape codes for terminal styling

## Project structure

```
campus-tracker-c/
├── student_manager.c   # complete source code
├── README.md
└── .gitignore          # ignores students.txt, report.txt, binaries
```

## Data format

Records are stored one per line in `students.txt`:

```
roll,name,age,marks
101,Asha Rao,19,92.00
```

## Possible improvements

- Login system with roles
- Subject-wise marks and GPA calculation
- CSV import
- Dynamic memory allocation to remove the 100-student limit

## License

MIT

# c-dataframe-cli: a small pandas-like CSV interpreter in C

[![CI](https://github.com/ortizserminio/c-dataframe-cli/actions/workflows/ci.yml/badge.svg)](https://github.com/ortizserminio/c-dataframe-cli/actions/workflows/ci.yml)

An interactive command-line program, written in plain C99 with no dependencies, that loads CSV files into in-memory **dataframes** and lets you inspect, sort, filter, transform and save them. It was my final practical for *Fundamentos de Programación* (Universidad Miguel Hernández, 2024/25), including the optional extensions. It builds on Linux, macOS and Windows.

```
$ ./dataframe
[?]:>load examples/data1
Archivo 'examples/data1' cargado correctamente como 'df0' con separador ','.
[df0: 10,4]:>sort Column3 des
[df0: 10,4]:>view 3
Column1             Column2             Column3             Column4
KJJKZ               2011/8/3            818.070000          WWFLW
NMXBO               2006/5/1            688.120000          GTJCH
CQLMU               2012/1/30           556.110000          RGDQB
[df0: 10,4]:>describe Column3
Column3  (n=10)
   min = -489.74   Q1  = -250.96   med = 397.77
   mean= 179.406   Q3  = 556.11   max = 818.07
   std = 463.446
```

(The interface messages are in Spanish, as required by the course.)

## Commands

| Command | Effect |
|---|---|
| `load <file> [sep]` | Load a CSV (`.csv` is added if missing). Column types are **inferred** from the first data row: date (`YYYY/MM/DD` or `YYYY-MM-DD`), number, or text. Empty cells are nulls. |
| `meta` | Column names, inferred types and null counts |
| `view [n]` | First `n` rows (default 10); `view -n` shows the last `n` in reverse |
| `sort <col> [asc\|des]` | Sort by a column (nulls go last in ascending order) |
| `filter <col> eq\|neq\|gt\|lt\|get\|let <value>` | Keep matching rows, compared by the column's type |
| `describe <col> ...` | count, mean, standard deviation, min, Q1, median, Q3, max |
| `delnull <col>` / `delcolum <col>` | Drop rows with a null in `col` / drop a column |
| `quarter <date col>` | Add a column with the quarter (Q1–Q4) of each date |
| `prefix <col> <n> <new col>` | Add a column with the first `n` characters of `col` |
| `add <file> [sep]` | Append the rows of another CSV with the same number of columns |
| `save [file] [sep]` | Write the active dataframe as CSV |
| `list` / `name <new>` / `switch <df>` | Manage several loaded dataframes |
| `quit` | Free everything and exit |

## Design

* **Columnar storage.** A `Dataframe` holds an array of `Columna`. Each column stores its values in one typed array (`double[]`, `struct tm[]` or `char*[]` behind a `void*`), plus a parallel `esNulo[]` array of null flags. Operations on one column touch contiguous memory, and nulls need no sentinel values.
* **Sorting by index.** `sort` does not move the data. It reorders an index array `indice[]` that maps logical rows to physical rows, so every column stays aligned and swapping two rows costs one `int` swap. The algorithm is bubble sort, which is **O(n²)**; fine for the course datasets of about 1 000 rows, but the first thing to replace with an O(n log n) sort (for example `qsort` over the index with the same comparator) for larger files.
* **Type inference on load.** Each column's type comes from its value in the first data row: a strict date validator (leap years, 30-day months) runs first, then a decimal validator; anything else is text.
* **Several dataframes.** Loaded dataframes are kept in a linked list. `list`, `switch` and `name` work on that list, and the prompt shows the active one as `[name: rows,cols]`.
* **Statistics (`describe`).** One pass computes count, min, max and mean. A second pass computes the variance with Welford's algorithm, which is numerically stable. Quartiles use nearest rank on a sorted copy (`qsort`, O(n log n)).

## Build, run and test

```bash
make                 # builds ./dataframe   (gcc or clang; MinGW on Windows)
./dataframe < script.txt   # it reads commands from stdin, so it can be scripted

make test            # unit tests + end-to-end tests
```

* **Unit tests** ([`tests/test_lib.c`](tests/test_lib.c), 158 checks, no framework):
  - validators (dates with leap years and invalid days, integers, decimals);
  - type inference, field splitting and trimming;
  - loading, and sorting by number, text and date;
  - null ordering, statistics and every filter operator;
  - a **save → load round trip** that must preserve every type, number and date.
* **End-to-end tests** ([`tests/test_cli.sh`](tests/test_cli.sh), 28 cases): drive the real binary through stdin and check its output, including error handling.
* **CI** runs everything on Ubuntu and macOS, plus an Ubuntu build with AddressSanitizer and UndefinedBehaviorSanitizer.

## What changed after the course

The first commit is the code as I submitted it (only my e-mail address was removed). Each later change has its own commit, and every bug fix has a test that failed before the fix:

* **Portability:** Windows console colours (`windows.h`) were replaced by a small `console.h` (Windows API on Windows, ANSI codes elsewhere). Under strict C, `strdup` was implicitly declared as returning `int`, which truncates pointers on 64-bit Linux.
* **`view` after `sort` showed scrambled rows:** the index permutation was applied twice, once in `view` and again inside `printElemento()`. The same code also leaked one allocation per printed cell.
* **`save` corrupted data:** numbers were written with one significant digit (`-339.39` → `-3e+02`), and dates as `DD/MM/(year+1900)`, so 2016 became **3916** in a format `load` could not read back.
* **`filter` with a mistyped operator** reported success and replaced the active dataframe with an empty one.
* **`prefix` after `sort`** filled the new column from the wrong rows (it mixed physical and display order).
* All `-Wall` warnings were cleaned up without changing behaviour.

The portability work, tests, CI, bug analysis and this README were prepared with the help of an AI assistant. The program itself is my own course work.

## License

[MIT](LICENSE)

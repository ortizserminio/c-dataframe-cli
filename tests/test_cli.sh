#!/usr/bin/env bash
# End-to-end tests: feed command scripts to the interpreter through stdin
# and check its output. Usage: bash tests/test_cli.sh ./dataframe
set -u
BIN="${1:-./dataframe}"
cd "$(dirname "$0")/.." || exit 1
mkdir -p tests/out

passed=0
failed=0
ok()   { passed=$((passed + 1)); echo "  ok   - $1"; }
bad()  { failed=$((failed + 1)); echo "  FAIL - $1"; }

# run "<commands>" -> output of the session in $out
run() { out=$(printf '%s\n' "$@" | "$BIN" 2>&1); }

# expect "<description>" "<fixed string that must appear in the output>"
expect() {
    if grep -qF -- "$2" <<< "$out"; then ok "$1"; else bad "$1"; echo "$out" | sed 's/^/      | /'; fi
}

echo "== end-to-end"

run "quit"
expect "starts with an empty prompt and exits cleanly" "[?]:>EXIT PROGRAM"

run "load examples/data1" "quit"
expect "load adds .csv and names the dataframe df0" "cargado correctamente como 'df0'"
expect "prompt shows name, rows and columns" "[df0: 10,4]:>"

run "load examples/Libro1 ;" "quit"
expect "load accepts a custom separator" "[df0: 10,3]:>"

run "load examples/does_not_exist" "quit"
expect "load reports missing files" "No se pudo cargar el archivo"

run "load examples/data1" "meta" "quit"
expect "meta shows inferred types (date)"    "Column2	Tipo: FECHA"
expect "meta shows inferred types (number)"  "Column3	Tipo: NUMERICO"
expect "meta shows null counts"              "Filas Nulas: 0"

run "load examples/data1" "view 2" "quit"
expect "view n prints the first n rows" "GQNTE               2005/6/30"

run "load examples/data1" "view -1" "quit"
expect "view -n prints the last rows" "QXULV               2012/2/5"

run "load examples/data1" "view 0" "quit"
expect "view 0 is rejected" "no puede ser 0"

run "load examples/data1" "filter Column3 gt 0" "quit"
expect "filter keeps matching rows" "Nuevo número de filas: 6"

run "load examples/data1" "filter Column3 gt" "quit"
expect "filter rejects wrong argument count" "filter necesita 3 argumentos"

run "load examples/data1" "describe Column3" "quit"
expect "describe prints count"  "Column3  (n=10)"
expect "describe prints min/median" "min = -489.74"
expect "describe prints max"    "max = 818.07"

run "load examples/data1" "sort Column9" "quit"
expect "sort reports unknown columns" "Columna no encontrada"

run "load examples/data1" "sort Column3 up" "quit"
expect "sort rejects an invalid order" "tiene que ser 'asc' o 'des'"

run "view" "quit"
expect "commands need an active dataframe" "No hay dataframe activo"

run "quit now"
expect "quit takes no arguments" "quit no acepta argumentos"

echo "== $passed passed, $failed failed"
[ "$failed" -eq 0 ]

# c-dataframe-cli: un pequeño intérprete de CSV al estilo pandas en C

[English](README.md) · **Español**

[![CI](https://github.com/ortizserminio/c-dataframe-cli/actions/workflows/ci.yml/badge.svg)](https://github.com/ortizserminio/c-dataframe-cli/actions/workflows/ci.yml)

Un programa interactivo de línea de comandos, escrito en C99 sin dependencias, que carga ficheros CSV en **dataframes** en memoria y permite inspeccionarlos, ordenarlos, filtrarlos, transformarlos y guardarlos. Fue mi práctica final de *Fundamentos de Programación* (Universidad Miguel Hernández, 2024/25), incluidas las ampliaciones opcionales. Compila en Linux, macOS y Windows.

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

## Comandos

| Comando | Efecto |
|---|---|
| `load <fichero> [sep]` | Carga un CSV (se añade `.csv` si falta). Los tipos de columna se **infieren** a partir de la primera fila de datos: fecha (`AAAA/MM/DD` o `AAAA-MM-DD`), número o texto. Las celdas vacías son nulos. |
| `meta` | Nombres de las columnas, tipos inferidos y número de nulos |
| `view [n]` | Primeras `n` filas (10 por defecto); `view -n` muestra las últimas `n` en orden inverso |
| `sort <col> [asc\|des]` | Ordena por una columna (los nulos van al final en orden ascendente) |
| `filter <col> eq\|neq\|gt\|lt\|get\|let <valor>` | Se queda con las filas que cumplen la condición, comparando según el tipo de la columna |
| `describe <col> ...` | número de valores, media, desviación típica, mínimo, Q1, mediana, Q3, máximo |
| `delnull <col>` / `delcolum <col>` | Elimina las filas con nulo en `col` / elimina una columna |
| `quarter <col fecha>` | Añade una columna con el trimestre (Q1–Q4) de cada fecha |
| `prefix <col> <n> <col nueva>` | Añade una columna con los primeros `n` caracteres de `col` |
| `add <fichero> [sep]` | Añade las filas de otro CSV con el mismo número de columnas |
| `save [fichero] [sep]` | Guarda el dataframe activo como CSV |
| `list` / `name <nuevo>` / `switch <df>` | Gestionan varios dataframes cargados |
| `quit` | Libera todo y sale |

## Diseño

* **Almacenamiento por columnas.** Un `Dataframe` contiene un array de `Columna`. Cada columna guarda sus valores en un array tipado (`double[]`, `struct tm[]` o `char*[]` detrás de un `void*`), más un array paralelo `esNulo[]` de indicadores de nulo. Las operaciones sobre una columna recorren memoria contigua, y los nulos no necesitan valores centinela.
* **Ordenación por índice.** `sort` no mueve los datos: reordena un array de índices `indice[]` que traduce filas lógicas a filas físicas, así que todas las columnas siguen alineadas e intercambiar dos filas cuesta intercambiar un `int`. El algoritmo es la burbuja, que es **O(n²)**. Basta para los datos de la asignatura (unas 1 000 filas), pero sería lo primero que cambiaría por un algoritmo O(n log n) (por ejemplo, `qsort` sobre el índice con el mismo comparador) para ficheros más grandes.
* **Inferencia de tipos al cargar.** El tipo de cada columna sale de su valor en la primera fila de datos: primero se aplica un validador estricto de fechas (años bisiestos, meses de 30 días) y después uno de decimales; lo demás es texto.
* **Varios dataframes.** Los dataframes cargados se guardan en una lista enlazada. `list`, `switch` y `name` trabajan sobre esa lista, y el prompt muestra el activo como `[nombre: filas,columnas]`.
* **Estadísticas (`describe`).** Una pasada calcula el número de valores, el mínimo, el máximo y la media. Una segunda pasada calcula la varianza con el algoritmo de Welford, que es numéricamente estable. Los cuartiles se toman por rango más cercano sobre una copia ordenada (`qsort`, O(n log n)).

## Compilar, ejecutar y probar

```bash
make                 # compila ./dataframe   (gcc o clang; MinGW en Windows)
./dataframe < script.txt   # lee comandos de stdin, así que se puede automatizar

make test            # tests unitarios + tests de punta a punta
```

* **Tests unitarios** ([`tests/test_lib.c`](tests/test_lib.c), 158 comprobaciones, sin framework):
  - validadores (fechas con años bisiestos y días no válidos, enteros, decimales);
  - inferencia de tipos, separación de campos y recorte de espacios;
  - carga, y ordenación por número, texto y fecha;
  - orden de los nulos, estadísticas y todos los operadores de filtro;
  - un **ciclo guardar → cargar** que debe conservar cada tipo, número y fecha.
* **Tests de punta a punta** ([`tests/test_cli.sh`](tests/test_cli.sh), 28 casos): manejan el ejecutable real a través de stdin y comprueban su salida, incluido el tratamiento de errores.
* El **CI** ejecuta todo en Ubuntu y macOS, más una build en Ubuntu con AddressSanitizer y UndefinedBehaviorSanitizer.

## Qué cambió después de la asignatura

El primer commit es el código tal como lo entregué (solo se quitó mi dirección de correo). Cada cambio posterior tiene su propio commit, y cada corrección de un bug tiene un test que fallaba antes de corregirlo:

* **Portabilidad:** los colores de consola de Windows (`windows.h`) se sustituyeron por un pequeño `console.h` (API de Windows en Windows, códigos ANSI en el resto). En C estricto, `strdup` quedaba declarado implícitamente como función que devuelve `int`, lo que trunca los punteros en Linux de 64 bits.
* **`view` después de `sort` mostraba las filas desordenadas:** la permutación del índice se aplicaba dos veces, una en `view` y otra dentro de `printElemento()`. Ese mismo código además perdía una reserva de memoria por cada celda impresa.
* **`save` corrompía los datos:** los números se escribían con una sola cifra significativa (`-339.39` → `-3e+02`) y las fechas como `DD/MM/(año+1900)`, así que 2016 pasaba a ser **3916**, en un formato que `load` no podía volver a leer.
* **`filter` con un operador mal escrito** decía que había funcionado y sustituía el dataframe activo por uno vacío.
* **`prefix` después de `sort`** rellenaba la columna nueva con valores de otras filas (mezclaba el orden físico con el de visualización).
* **Uso de memoria liberada en `quarter`** (encontrado por AddressSanitizer en el CI): al añadir la columna nueva se hace `realloc` del array de columnas, y la función seguía usando un puntero al bloque antiguo. En Windows el bloque no se movía por casualidad, así que parecía funcionar; en macOS daba trimestres incorrectos.
* Se eliminaron todos los avisos de `-Wall` sin cambiar el comportamiento.

La portabilidad, los tests, el CI, el análisis de bugs y este README se prepararon con ayuda de un asistente de IA. El programa en sí es mi trabajo de la asignatura.

## Licencia

[MIT](LICENSE)

// Unit tests for lib.c. Run from the repository root: make unit
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "lib.h"

static int checks = 0, failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++checks;                                                            \
        if (!(cond)) {                                                       \
            ++failures;                                                      \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                    #cond);                                                  \
        }                                                                    \
    } while (0)

#define CHECK_NEAR(a, b) CHECK(fabs((a) - (b)) < 1e-6)

static Dataframe* load_example(void) {
    Nodo* n = Load("examples/data1.csv", ',');
    return n ? n->df : NULL;
}

// Value of a numeric column at logical row i (i.e. after sorting).
static double num_at(Dataframe* df, Columna* c, int i) {
    return ((double*)c->datos)[df->indice[i]];
}

static void test_validators(void) {
    CHECK(Validar_entero("42"));
    CHECK(Validar_entero("-7"));
    CHECK(Validar_entero("+3"));
    CHECK(!Validar_entero(""));
    CHECK(!Validar_entero("+"));
    CHECK(!Validar_entero("4.2"));
    CHECK(!Validar_entero("12a"));

    CHECK(Validar_decimal("3.14"));
    CHECK(Validar_decimal("-0.5"));
    CHECK(Validar_decimal("10"));
    CHECK(Validar_decimal(".5"));
    CHECK(Validar_decimal("5."));
    CHECK(!Validar_decimal(""));
    CHECK(!Validar_decimal("+"));
    CHECK(!Validar_decimal("-."));
    CHECK(!Validar_decimal("."));
    CHECK(!Validar_decimal("1.2.3"));
    CHECK(!Validar_decimal("abc"));
}

static void test_dates(void) {
    CHECK(FechaValidarAMD("2021/12/31"));
    CHECK(FechaValidarAMD("2021-12-31"));
    CHECK(FechaValidarAMD("2024/02/29"));   // leap year
    CHECK(FechaValidarAMD("2000/02/29"));   // divisible by 400
    CHECK(!FechaValidarAMD("1900/02/29"));  // divisible by 100, not 400
    CHECK(!FechaValidarAMD("2023/02/29"));
    CHECK(!FechaValidarAMD("2021/04/31"));  // April has 30 days
    CHECK(!FechaValidarAMD("2021/13/01"));
    CHECK(!FechaValidarAMD("2021/00/10"));
    CHECK(!FechaValidarAMD("2021/12-31"));  // mixed separators
    CHECK(!FechaValidarAMD("31/12/2021"));  // day first is not accepted
    CHECK(!FechaValidarAMD("2021/1/5"));    // must be zero-padded

    Fecha* f = AFecha("2016/05/09");
    CHECK(f != NULL);
    if (f) {
        CHECK(f->tm_year == 2016 && f->tm_mon == 4 && f->tm_mday == 9);
        free(f);
    }
    CHECK(AFecha("not a date") == NULL);
}

static void test_type_inference(void) {
    CHECK(Tipo("2020/01/01") == FECHA);
    CHECK(Tipo("12.5") == NUMERICO);
    CHECK(Tipo("-3") == NUMERICO);
    CHECK(Tipo("hello") == TEXTO);
    CHECK(Tipo("") == TEXTO);
}

static void test_string_helpers(void) {
    char a[] = "  hola \t\n";
    Trim(a);
    CHECK(strcmp(a, "hola") == 0);
    char b[] = "   ";
    Trim(b);
    CHECK(strcmp(b, "") == 0);
    char c[] = "";
    Trim(c);
    CHECK(strcmp(c, "") == 0);

    CHECK(NumCampos("a,b,c") == 3);
    CHECK(NumCampos("abc") == 1);
    CHECK(NumCampos("a,,c") == 3);
    CHECK(NumCampos("") == 0);

    char buf[32];
    CHECK(strcmp(ObtenerCampo("a,bb,c", 1, buf), "bb") == 0);
    CHECK(strcmp(ObtenerCampo("a,bb,c", 2, buf), "c") == 0);
    CHECK(strcmp(ObtenerCampo("a,,c", 1, buf), "") == 0);
    CHECK(ObtenerCampo("", 0, buf) == NULL);
}

static void test_load(void) {
    Dataframe* df = load_example();
    CHECK(df != NULL);
    if (!df) return;
    CHECK(df->numFilas == 10);
    CHECK(df->numColumnas == 4);
    CHECK(df->columna[0].tipo == TEXTO);
    CHECK(df->columna[1].tipo == FECHA);
    CHECK(df->columna[2].tipo == NUMERICO);
    CHECK(df->columna[3].tipo == TEXTO);
    for (int j = 0; j < 4; j++) CHECK(numNulos(df->columna[j]) == 0);
    CHECK(strcmp(((char**)df->columna[0].datos)[0], "FLFOD") == 0);
    CHECK_NEAR(((double*)df->columna[2].datos)[0], -339.39);

    CHECK(BuscarColumna(df, "Column3") == &df->columna[2]);
    CHECK(BuscarColumna(df, "Nope") == NULL);

    CHECK(Load("examples/does_not_exist.csv", ',') == NULL);
    LiberarDataframe(df);
}

static void test_sort(void) {
    Dataframe* df = load_example();
    if (!df) { CHECK(0); return; }
    Columna* num = BuscarColumna(df, "Column3");

    SortCol(df, num, 1);
    for (int i = 0; i + 1 < df->numFilas; i++)
        CHECK(num_at(df, num, i) <= num_at(df, num, i + 1));
    CHECK_NEAR(num_at(df, num, 0), -489.74);
    CHECK_NEAR(num_at(df, num, 9), 818.07);

    SortCol(df, num, 0);
    for (int i = 0; i + 1 < df->numFilas; i++)
        CHECK(num_at(df, num, i) >= num_at(df, num, i + 1));

    // Text column: strcmp order
    Columna* txt = BuscarColumna(df, "Column1");
    SortCol(df, txt, 1);
    for (int i = 0; i + 1 < df->numFilas; i++)
        CHECK(strcmp(((char**)txt->datos)[df->indice[i]],
                     ((char**)txt->datos)[df->indice[i + 1]]) <= 0);

    // Date column: chronological order
    Columna* date = BuscarColumna(df, "Column2");
    SortCol(df, date, 1);
    for (int i = 0; i + 1 < df->numFilas; i++) {
        Fecha* a = &((Fecha*)date->datos)[df->indice[i]];
        Fecha* b = &((Fecha*)date->datos)[df->indice[i + 1]];
        CHECK(compararValores(a, 0, b, 0, FECHA) <= 0);
    }

    // Sorting only permutes the index: it stays a permutation of 0..n-1
    int seen[10] = {0};
    for (int i = 0; i < 10; i++) seen[df->indice[i]]++;
    for (int i = 0; i < 10; i++) CHECK(seen[i] == 1);
    LiberarDataframe(df);
}

static void test_nulls_compare_last(void) {
    double x = 1, y = 2;
    CHECK(compararValores(&x, 0, &y, 0, NUMERICO) < 0);
    CHECK(compararValores(&x, 1, &y, 0, NUMERICO) > 0);  // null goes last
    CHECK(compararValores(&x, 0, &y, 1, NUMERICO) < 0);
    CHECK(compararValores(&x, 1, &y, 1, NUMERICO) == 0);
}

static void test_stats(void) {
    Dataframe* df = load_example();
    if (!df) { CHECK(0); return; }
    Stats s = stats_col(BuscarColumna(df, "Column3"));
    CHECK(s.count == 10);
    CHECK_NEAR(s.min, -489.74);
    CHECK_NEAR(s.max, 818.07);
    CHECK_NEAR(s.mean, 179.406);
    CHECK_NEAR(s.med, 397.77);   // nearest-rank: index round(0.5 * 9) = 5

    Stats t = stats_col(BuscarColumna(df, "Column1"));  // text column
    CHECK(t.count == 0);
    LiberarDataframe(df);
}

static void test_filter(void) {
    Dataframe* df = load_example();
    if (!df) { CHECK(0); return; }
    Columna* num = BuscarColumna(df, "Column3");

    Dataframe* pos = Filtrar(df, num, "gt", "0", NUMERICO);
    CHECK(pos != NULL && pos->numFilas == 6);
    Dataframe* neg = Filtrar(df, num, "lt", "0", NUMERICO);
    CHECK(neg != NULL && neg->numFilas == 4);
    Dataframe* ge = Filtrar(df, num, "get", "51.58", NUMERICO);
    CHECK(ge != NULL && ge->numFilas == 6);   // 51.58 itself is included

    Columna* txt = BuscarColumna(df, "Column1");
    Dataframe* one = Filtrar(df, txt, "eq", "FLFOD", TEXTO);
    CHECK(one != NULL && one->numFilas == 1);
    Dataframe* rest = Filtrar(df, txt, "neq", "FLFOD", TEXTO);
    CHECK(rest != NULL && rest->numFilas == 9);

    if (pos) LiberarDataframe(pos);
    if (neg) LiberarDataframe(neg);
    if (ge) LiberarDataframe(ge);
    if (one) LiberarDataframe(one);
    if (rest) LiberarDataframe(rest);
    LiberarDataframe(df);
}

static void test_save_rejects_invalid_input(void) {
    CHECK(Save(NULL, "x.csv", ',') == -1);
}

// Saving and loading again must give back the same types and values.
static void test_save_round_trip(void) {
    Dataframe* df = load_example();
    if (!df) { CHECK(0); return; }
    const char* path = "tests/out/round_trip.csv";
    CHECK(Save(df, (char*)path, ';') == 0);

    Nodo* n = Load((char*)path, ';');
    CHECK(n != NULL);
    if (!n) { LiberarDataframe(df); return; }
    Dataframe* back = n->df;
    CHECK(back->numFilas == df->numFilas);
    CHECK(back->numColumnas == df->numColumnas);
    for (int j = 0; j < df->numColumnas && j < back->numColumnas; j++)
        CHECK(back->columna[j].tipo == df->columna[j].tipo);

    for (int i = 0; i < df->numFilas && i < back->numFilas; i++) {
        CHECK_NEAR(((double*)back->columna[2].datos)[i],
                   ((double*)df->columna[2].datos)[i]);
        Fecha* a = &((Fecha*)df->columna[1].datos)[i];
        Fecha* b = &((Fecha*)back->columna[1].datos)[i];
        CHECK(compararValores(a, 0, b, 0, FECHA) == 0);
    }
    LiberarDataframe(back);
    LiberarDataframe(df);
}

int main(void) {
    test_validators();
    test_dates();
    test_type_inference();
    test_string_helpers();
    test_load();
    test_sort();
    test_nulls_compare_last();
    test_stats();
    test_filter();
    test_save_rejects_invalid_input();
    test_save_round_trip();
    printf("%d/%d checks passed\n", checks - failures, checks);
    return failures == 0 ? 0 : 1;
}

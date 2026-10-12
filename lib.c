// strdup() is POSIX, not ISO C: request it explicitly so it is declared
// (otherwise it is implicitly int and truncates pointers on 64-bit).
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include "lib.h"

///////// Ampliaciones //////////
#include <math.h>     /* sqrt, ceil, floor … */



void Datos_alumno(){
    printf("Esterly Ortiz Serminio\n");
    

}

// 1 OK, 0 MAL
int Validar_decimal(char *cadena){

    int i, p=0;

    if(cadena[0] == '+' || cadena[0] == '-')
        i=1;
    else    
        i=0;

    if(strlen(cadena) == 1 && i==1){ // caso en el que solo escriben + o -
        return 0;
    }

    if(strlen(cadena) == 0){
        return 0;
    }

    if(i == 1 && strlen(cadena) == 2 && cadena[1] == '.' ){ //caso en el que nos escriben +. o -.
        return 0;
    }

    if(strlen(cadena) == 1 && i == 0 && cadena[0] == '.'){ // .
        return 0;
    }

    for(; i<strlen(cadena); i++){
        if(cadena[i] < '0' || cadena[i] > '9'){
            if(cadena[i] == '.'){
                p++;
            }
            else{
                return 0;
            }
        }

        if(p == 2){
            return 0;
        }
    }

    return 1;
}

// 1 OK, 0 MAL
// Esta sería la versión mejorada de Validar_entero para incluir negativos
int Validar_entero(char *cad) {
    int i = 0;

    if (cad == NULL || strlen(cad) == 0) {
        return 0; // Cadena nula o vacía no es un entero
    }

    // Permitir un signo opcional al principio (+ o -)
    if (cad[0] == '+' || cad[0] == '-') {
        i = 1; // Comenzar la verificación de dígitos desde el segundo carácter
        // Si la cadena solo tiene el signo, no es un número válido
        if (strlen(cad) == 1) {
            return 0;
        }
    }

    // El resto de los caracteres deben ser dígitos
    for (; i < strlen(cad); i++) {
        if (cad[i] < '0' || cad[i] > '9') {
            return 0; // Carácter no es un dígito
        }
    }
    return 1; // Todos los caracteres son válidos
}

// TU Validar_negativo ya no sería estrictamente necesaria si Validar_entero es así de robusta.
// int Validar_negativo(char *cad) { ... } // Puedes eliminarla si quieres.
//Esta funcion elimina los espacios en blanco del token
void Trim(char *cadena){

    int c, i, j, n=strlen(cadena);

    if(cadena == NULL || n<1){
        return;
    }

    n--;

    while(n>=0 && ((c=cadena[n]) == ' ' || c=='\t' || c=='\n' || c =='\r')){
        n--;
    }

    cadena[n+1] = '\0';

    i=0;

    while(i<=n && ((c=cadena[i]) ==' ' || c == '\t' || c=='\n' || c=='\r')){
        i++;
    }

    for(j=0;j <= n-i+1; j++){
        cadena[j] = cadena[j+i];
    }
}

// En lib.c
// ... otras inclusiones y funciones ...

/* --------------------------------------------------------------
 *  Load  : lee un CSV y devuelve un Nodo con su DataFrame
 *          – sin goto, con early-return y liberación ordenada –
 * -------------------------------------------------------------- */
Nodo* Load(char *nombre, char separator)
{
    /* ---------------- variables y buffers ---------------- */
    FILE      *f               = NULL;
    char       line[2048];
    char       delim[2]         = { separator, '\0' };

    int         numColumnas     = 0;
    char      **headers         = NULL;
    TipoDato   *tipos           = NULL;
    char      **data_lines      = NULL;
    int         rows            = 0;

    Dataframe  *df              = NULL;
    Nodo       *nodo            = NULL;

    /* ---------- abrir archivo ---------- */
    if (!(f = fopen(nombre, "r")))
        return NULL;

    /* ---------- 1. cabecera ------------ */
    if (!fgets(line, sizeof(line), f)) { fclose(f); return NULL; }
    Trim(line);
    if (!*line)                       { fclose(f); return NULL; }

    /* contar columnas */
    {
        char *aux = strdup(line);
        if (!aux)                     { fclose(f); return NULL; }
        for (char *tok = strtok(aux, delim); tok; tok = strtok(NULL, delim))
            ++numColumnas;
        free(aux);
    }
    if (!numColumnas)                 { fclose(f); return NULL; }

    /* copiar nombres */
    headers = calloc(numColumnas, sizeof(char*));
    if (!headers) { fclose(f); return NULL; }

    {
        char *aux = strdup(line);
        if (!aux) { fclose(f); for (int j=0;j<numColumnas;++j) free(headers[j]); free(headers); return NULL; }
        int i = 0;
        for (char *tok = strtok(aux, delim); tok && i < numColumnas;
             tok = strtok(NULL, delim), ++i)
        {
            Trim(tok);
            if (!(headers[i] = strdup(tok))) {
                fclose(f);
                for (int j=0;j<=i;++j) free(headers[j]);
                free(headers);
                free(aux);
                return NULL;
            }
        }
        free(aux);
    }

    /* ---------- 2. inferir tipos -------- */
    if (!fgets(line, sizeof(line), f)) {
        fclose(f); for (int j=0;j<numColumnas;++j) free(headers[j]); free(headers); return NULL;
    }
    Trim(line);  if (!*line) {
        fclose(f); for (int j=0;j<numColumnas;++j) free(headers[j]); free(headers); return NULL;
    }

    tipos = malloc(numColumnas * sizeof(TipoDato));
    if (!tipos) {
        fclose(f); for (int j=0;j<numColumnas;++j) free(headers[j]); free(headers); return NULL;
    }

    {
        char *aux = strdup(line);
        if (!aux) {
            fclose(f); free(tipos); for (int j=0;j<numColumnas;++j) free(headers[j]); free(headers); return NULL;
        }
        int i = 0;
        for (char *tok = strtok(aux, delim); tok && i < numColumnas;
             tok = strtok(NULL, delim), ++i)
        {
            Trim(tok);
            tipos[i] = Tipo(tok);
        }
        free(aux);
    }

    /* guarda la línea como primera fila */
    data_lines = malloc(sizeof(char*));
    if (!data_lines) {
        fclose(f); free(tipos); for (int j=0;j<numColumnas;++j) free(headers[j]); free(headers); return NULL;
    }
    if (!(data_lines[0] = strdup(line))) {
        fclose(f); free(data_lines); free(tipos); for (int j=0;j<numColumnas;++j) free(headers[j]); free(headers); return NULL;
    }
    rows = 1;

    /* ---------- 3. resto de filas -------- */
    while (fgets(line, sizeof(line), f)) {
        Trim(line);
        if (!*line) continue;

        char **tmp = realloc(data_lines, (rows + 1) * sizeof(char*));
        if (!tmp) break;
        data_lines = tmp;

        if (!(data_lines[rows] = strdup(line))) break;
        ++rows;
    }
    fclose(f);  /* ya no se necesita el fichero */

    /* ---------- 4. construir DataFrame --- */
    df = malloc(sizeof(Dataframe));
    if (!df) {                         /* liberar buffers y salir */
        for (int k=0;k<rows;++k) free(data_lines[k]);
        free(data_lines);  free(tipos);
        for (int j=0;j<numColumnas;++j) free(headers[j]); free(headers);
        return NULL;
    }

    df->numColumnas = numColumnas;
    df->numFilas    = rows;
    df->columna     = calloc(numColumnas, sizeof(Columna));
    df->indice      = rows ? malloc(rows * sizeof(int)) : NULL;
    df->nombre[0]   = '\0';

    if (!df->columna || (rows && !df->indice)) {
        LiberarDataframe(df);
        for (int k=0;k<rows;++k) free(data_lines[k]);
        free(data_lines);  free(tipos);
        for (int j=0;j<numColumnas;++j) free(headers[j]); free(headers);
        return NULL;
    }
    for (int r=0; r<rows; ++r) if (df->indice) df->indice[r] = r;

    /* reservar celdas columna por columna */
    for (int c=0; c<numColumnas; ++c) {
        Columna *col = &df->columna[c];
        col->tipo     = tipos[c];
        strncpy(col->nombre, headers[c], sizeof(col->nombre) - 1);
        col->nombre[ sizeof(col->nombre) - 1 ] = '\0'; 
        col->numFilas = rows;

        size_t n = rows;
        if (col->tipo == TEXTO)     col->datos  = calloc(n, sizeof(char*));
        else if (col->tipo == NUMERICO) col->datos = calloc(n, sizeof(double));
        else                          col->datos  = calloc(n, sizeof(Fecha));

        col->esNulo = calloc(n, sizeof(unsigned char));

        if (!col->nombre || !col->datos || !col->esNulo) {
            LiberarDataframe(df);
            for (int k=0;k<rows;++k) free(data_lines[k]);
            free(data_lines);  free(tipos);
            for (int j=0;j<numColumnas;++j) free(headers[j]); free(headers);
            return NULL;
        }
    }

    /* ---------- 5. volcar buffer -------- */
    for (int r=0; r<rows; ++r) {
        char *aux = strdup(data_lines[r]);
        if (!aux) {
            LiberarDataframe(df);
            for (int k=r;k<rows;++k) free(data_lines[k]);
            free(data_lines);  free(tipos);
            for (int j=0;j<numColumnas;++j) free(headers[j]); free(headers);
            return NULL;
        }

        int col = 0;
        for (char *tok = strtok(aux, delim); tok && col < numColumnas;
             tok = strtok(NULL, delim), ++col)
        {
            Trim(tok);
            unsigned char nul = 0;
            Columna *c = &df->columna[col];

            if (!*tok) {
                nul = 1;
            } else if (c->tipo == TEXTO) {
                ((char**)c->datos)[r] = strdup(tok);
                if (!((char**)c->datos)[r]) { free(aux); LiberarDataframe(df); /* …free… */ 
                    for (int k=r;k<rows;++k) free(data_lines[k]); free(data_lines);
                    free(tipos); for (int j=0;j<numColumnas;++j) free(headers[j]); free(headers);
                    return NULL; }
            } else if (c->tipo == NUMERICO) {
                char *e = NULL;
                double v = strtod(tok, &e);
                if (*e || !Validar_decimal(tok)) nul = 1;
                else ((double*)c->datos)[r] = v;
            } else { /* FECHA */
                Fecha *pf = AFecha(tok);
                if (!pf) nul = 1;
                else { ((Fecha*)c->datos)[r] = *pf; free(pf); }
            }
            c->esNulo[r] = nul;
        }
        for (; col < numColumnas; ++col)
            df->columna[col].esNulo[r] = 1;

        free(aux);
        free(data_lines[r]);
    }
    free(data_lines);
    data_lines = NULL;

    /* liberar auxiliares */
    free(tipos);
    for (int j=0;j<numColumnas;++j) free(headers[j]);
    free(headers);

    /* ---------- 6. empaquetar en Nodo ---- */
    nodo = malloc(sizeof(Nodo));
    if (!nodo) {
        LiberarDataframe(df);
        return NULL;
    }
    nodo->df = df;
    nodo->siguiente = NULL;
    return nodo;
}


Fecha* AFecha(char *cadena){

    Fecha *f = (Fecha *)malloc(sizeof(Fecha));
    if (!f) return NULL;

    // Intenta analizar la fecha en formato AAAA-MM-DD o AAAA/MM/DD
    if (sscanf(cadena, "%d-%d-%d", &f->tm_year, &f->tm_mon, &f->tm_mday) == 3 ||
        sscanf(cadena, "%d/%d/%d", &f->tm_year, &f->tm_mon, &f->tm_mday) == 3)
    {
        f->tm_mon -= 1; // el mes en struct tm es 0-11
        return f;
    }

    // Si el análisis falla, libera la memoria y devuelve NULL
    free(f);
    return NULL;
}

Dataframe* CrearDataFrame(int filas, int columnas, TipoDato *t, char* cab){
    
    char* val = (char *)malloc(sizeof(char)*40);
    Dataframe *df = (Dataframe *)malloc(sizeof(Dataframe));
    df->numColumnas=columnas;
    df->numFilas=filas;
    df->indice = (int *)malloc(sizeof(int)*filas);
    if(df->indice == NULL)
        return NULL;
    for (int i=0; i<filas; i++){
        df->indice[i] = i;
    }
    df->columna = (Columna *)malloc(sizeof(Columna)*columnas);
    for (int i=0; i<columnas; i++){
        ObtenerCampo(cab,i, val);
        df->columna[i] = *CrearColumna(val, t[i], filas);
    }
    return df;
}

Columna* CrearColumna(char *nombre, TipoDato t, int filas){
    Columna *c = (Columna *)malloc(sizeof(Columna));
    
    strcpy(c->nombre, nombre);
    c->esNulo = (unsigned char *) malloc(sizeof(unsigned char)*filas);

    c->tipo = t;
    if (t == TEXTO){
        c->datos = (char **)malloc(sizeof(char*)*filas);
    }
    else if(t == NUMERICO){
        c->datos = (double *)malloc(sizeof(double)*filas);
    }
    else if(t == FECHA){
        c->datos = (Fecha *)malloc(sizeof(Fecha)*filas);
    }
    c->numFilas=filas;
    return c;
}

int NumCampos (char *lin){
    char *sep = strdup(",");
	int x=1;
	int tamSep=strlen(sep);
	char *p1, *p2;
	
	if (lin==NULL || lin[0]=='\0')
		return 0;
	p2=strstr(lin, sep);
	while (p2!=NULL)
	{
		x++;
		p1=p2+tamSep;
		p2=strstr(p1, sep);
	}
	
	return x;
}
		
char* ObtenerCampo(char *lin, int id, char *val){
    char *sep = strdup(",");
	int i, tamSep=strlen(sep);
	char *p1, *p2;
	
	if (lin==NULL || lin[0]=='\0')
		return NULL;
	
	p1=lin;
	p2=strstr(lin, sep);
	
	for(i=0; i<id;i++)
	{
		p1=p2+tamSep;
		p2=strstr(p1, sep);
	}
	if(p2==NULL)
		strcpy(val,p1);
	else
	{
		strncpy(val, p1, p2-p1);
		val [p2-p1]='\0';
	}
	return val;
}

TipoDato Tipo(char *val){
    // printf("VAL: '%s' ->", val);
    if (FechaValidarAMD(val)){
        // printf("FECHA\n");
    	return FECHA;
    }
    else if (Validar_decimal(val)){
        // printf("NUMERICO\n");
        return NUMERICO;
    }
    else{
        // printf("TEXTO\n");
        return TEXTO;
    }
}

int FechaValidarAMD (char* fecha) {
    int d, m, a, i;

    // 1. Comprobar longitud
    if (strlen(fecha) != 10) {
        // printf("Fecha incorrecta - 1: Longitud incorrecta.\n"); // Para depuración
        return 0;
    }

    // 2. Comprobar separadores (que sean '-' o '/' y que sean iguales)
    char separator_char = fecha[4];
    if ((separator_char != '-' && separator_char != '/') || (separator_char != fecha[7])) {
        // printf("Fecha incorrecta - 2: Separadores incorrectos o inconsistentes.\n"); // Para depuración
        return 0;
    }

    // 3. Comprobar que los caracteres numéricos sean dígitos
    for (i = 0; i < 10; i++) {
        if (i == 4 || i == 7) { // Saltar los separadores
            continue;
        }
        if (fecha[i] < '0' || fecha[i] > '9') {
            // printf("Fecha incorrecta - 3: Caracter no numérico.\n"); // Para depuración
            return 0;
        }
    }

    // Si llegamos aquí, el formato general (AAAA/MM/DD o AAAA-MM-DD con dígitos) es correcto.
    // Extraer día, mes, año
    d = (fecha[8] - '0') * 10 + (fecha[9] - '0');
    m = (fecha[5] - '0') * 10 + (fecha[6] - '0');
    a = (fecha[0] - '0') * 1000 + (fecha[1] - '0') * 100 + (fecha[2] - '0') * 10 + (fecha[3] - '0');

    // printf("d=%d, m=%d, a=%d\n", d, m, a); // Para depuración

    // 4. Validar rangos básicos de día, mes, año
    if (a < 1 || m < 1 || m > 12 || d < 1 || d > 31) {
        // printf("Fecha incorrecta - 4: Rango de día/mes/año inválido.\n"); // Para depuración
        return 0;
    }

    // 5. Validar días para meses con 30 días
    if ((m == 4 || m == 6 || m == 9 || m == 11) && d > 30) {
        // printf("Fecha incorrecta - 5: Día inválido para el mes (max 30).\n"); // Para depuración
        return 0;
    }

    // 6. Validar febrero (28 o 29 días)
    if (m == 2) {
        // Comprobación de año bisiesto
        int is_leap = (a % 400 == 0) || (a % 4 == 0 && a % 100 != 0);

        if (is_leap) {
            if (d > 29) {
                // printf("Fecha incorrecta - 6: Día inválido para febrero en año bisiesto (max 29).\n"); // Para depuración
                return 0;
            } else {
                // printf("Fecha OK (febrero año bisiesto)\n"); // Para depuración
                return 1;
            }
        } else { // No es año bisiesto
            if (d > 28) {
                // printf("Fecha incorrecta - 7: Día inválido para febrero en año NO bisiesto (max 28).\n"); // Para depuración
                return 0;
            } else {
                // printf("Fecha OK (febrero año NO bisiesto)\n"); // Para depuración
                return 1;
            }
        }
    }

    // Si es un mes distinto de febrero y ha pasado las validaciones anteriores
    // printf("Fecha OK (mes distinto de febrero)\n"); // Para depuración
    return 1;
}

int anyadirNodo(Nodo *nuevo, Lista *laGranLista) {
    if (nuevo == NULL || laGranLista == NULL) {
        return 0; // Error: nodo o lista nula
    }

    if (laGranLista->primero == NULL) {
        laGranLista->primero = nuevo;
    } else {
        Nodo *temp = laGranLista->primero; // <-- Pequeño error de tipeo aquí, debería ser laGranLista->primero
        Nodo *temp_fixed = laGranLista->primero;
        while (temp_fixed->siguiente != NULL) {
            temp_fixed = temp_fixed->siguiente;
        }
        temp_fixed->siguiente = nuevo;
    }
    nuevo->siguiente = NULL; // Asegurar que el nuevo nodo apunta a NULL

    laGranLista->numDFs++; // ¡ESTE ES EL INCREMENTO CLAVE!
    return 1; // Éxito
}

int numNulos(Columna col){

    int i, contador=0;
    for(i=0; i<col.numFilas; i++){
        if(col.esNulo[i] == 1){
            contador++;
        }
    }
    return contador;
}

char* printElemento(Dataframe* df, int col, int fila) {
    char *resultado = (char *)malloc(100 * sizeof(char)); 
    if (!resultado)
        return NULL; 

    if (df->columna[col].esNulo[df->indice[fila]]==1)
        strcpy(resultado, "#N/A");
    else if (df->columna[col].tipo == TEXTO) {
        strcpy(resultado, ((char **)df->columna[col].datos)[df->indice[fila]]);
    }
    else if (df->columna[col].tipo == NUMERICO) {
        sprintf(resultado, "%f", ((double *)df->columna[col].datos)[df->indice[fila]]);
    }
    else if (df->columna[col].tipo == FECHA) {
        sprintf(resultado, "%d/%d/%d", 
            ((Fecha *)df->columna[col].datos)[df->indice[fila]].tm_year, 
            ((Fecha *)df->columna[col].datos)[df->indice[fila]].tm_mon+1, 
            ((Fecha *)df->columna[col].datos)[df->indice[fila]].tm_mday);
    }
    return resultado;
}

Columna* BuscarColumna(Dataframe *df, char *nombre){
    int i;
    for(i=0; i<df->numColumnas; i++){
        if(strcmp(df->columna[i].nombre, nombre) == 0){
            return &df->columna[i];
        }
    }
    return NULL;
}

void intercambiar(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

//Si dato1 > dato2 return + 
// Si dato1 < dato2 return - 
// Si dato1 = dato2 return 0
// En lib.c
// En lib.c

// Implementación de compararValores (con manejo de nulos)
int compararValores(void *dato1, unsigned char esNulo1, void *dato2, unsigned char esNulo2, TipoDato tipo) {
    // Reglas de comparación de nulos:
    // - Si ambos son nulos, se consideran iguales.
    // - Si solo dato1 es nulo, es "mayor" (va al final en orden ascendente).
    // - Si solo dato2 es nulo, es "menor" (va al principio en orden ascendente).
    // Esta política puede ajustarse según tus requisitos específicos para nulos.

    if (esNulo1 && esNulo2) {
        return 0; // Ambos nulos, se consideran iguales
    }
    if (esNulo1) {
        return 1; // dato1 es nulo, dato2 no. dato1 es "mayor".
    }
    if (esNulo2) {
        return -1; // dato2 es nulo, dato1 no. dato2 es "menor".
    }

    // Si ninguno es nulo, procedemos con la comparación normal según el tipo de dato
    switch (tipo) {
        case TEXTO: {
            // Asumimos que dato1 y dato2 son punteros a char (char*)
            return strcmp((char *)dato1, (char *)dato2);
        }
        case NUMERICO: {
            // Asumimos que dato1 y dato2 son punteros a double (double*)
            double d1 = *(double *)dato1;
            double d2 = *(double *)dato2;
            if (d1 < d2) return -1;
            if (d1 > d2) return 1;
            return 0;
        }
        case FECHA: {
            // Asumimos que dato1 y dato2 son punteros a struct tm (Fecha*)
            Fecha *f1 = (Fecha *)dato1;
            Fecha *f2 = (Fecha *)dato2;

            // Comparar por año, luego mes, luego día
            if (f1->tm_year != f2->tm_year)
                return f1->tm_year - f2->tm_year;
            if (f1->tm_mon != f2->tm_mon)
                return f1->tm_mon - f2->tm_mon;
            return f1->tm_mday - f2->tm_mday;
        }
        default:
            return 0; // Tipo desconocido, se consideran iguales
    }
}

// Implementación de SortCol (ya revisada, la incluyo para contexto)
void SortCol(Dataframe *df, Columna *col, int asc) {
    if (df == NULL || col == NULL || df->indice == NULL || df->numFilas == 0) {
        return; // No se puede ordenar si el DF o la columna son inválidos
    }

    int n = df->numFilas;
    int *indice = df->indice; // El array de índices del DataFrame

    // Algoritmo de burbuja (adaptado para usar el índice)
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            int current_idx = indice[j];      // Índice de la fila actual
            int next_idx = indice[j + 1];     // Índice de la siguiente fila

            // Obtener los datos reales de la columna usando los índices y los punteros void*
            void *dato1_ptr = NULL;
            void *dato2_ptr = NULL;
            
            // Acceder a los datos usando el índice actual de la fila
            if (col->tipo == NUMERICO) {
                dato1_ptr = &((double *)col->datos)[current_idx];
                dato2_ptr = &((double *)col->datos)[next_idx];
            } else if (col->tipo == FECHA) {
                dato1_ptr = &((Fecha *)col->datos)[current_idx];
                dato2_ptr = &((Fecha *)col->datos)[next_idx];
            } else if (col->tipo == TEXTO) {
                dato1_ptr = ((char **)col->datos)[current_idx]; // No se necesita &, ya es un char*
                dato2_ptr = ((char **)col->datos)[next_idx];
            }

            // Obtener el estado de nulidad de cada valor
            unsigned char esNulo1 = col->esNulo[current_idx];
            unsigned char esNulo2 = col->esNulo[next_idx];

            // Comparar los valores usando la función compararValores que ahora maneja los nulos
            int cmp = compararValores(dato1_ptr, esNulo1, dato2_ptr, esNulo2, col->tipo);

            // Determinar si se necesita un intercambio
            int should_swap = 0;
            if (asc) { // Orden ascendente
                if (cmp > 0) { // Si dato1 es mayor que dato2
                    should_swap = 1;
                }
            } else { // Orden descendente
                if (cmp < 0) { // Si dato1 es menor que dato2
                    should_swap = 1;
                }
            }

            if (should_swap) {
                intercambiar(&indice[j], &indice[j + 1]); // Intercambia los elementos en el array de índices
            }
        }
    }
}

// Implementación de Filtrar (nueva y corregida)
Dataframe* Filtrar(Dataframe *df, Columna *col, char *comp, char *val_str, TipoDato tipo_col) {
    if (df == NULL || col == NULL || df->numFilas == 0) {
        return NULL; // DataFrame o columna de filtrado inválidos
    }

    // Unknown operators used to match no row and silently replace the active
    // dataframe with an empty one; reject them instead.
    if (comp == NULL || (strcmp(comp, "eq") != 0 && strcmp(comp, "neq") != 0 &&
                         strcmp(comp, "gt") != 0 && strcmp(comp, "lt") != 0 &&
                         strcmp(comp, "get") != 0 && strcmp(comp, "let") != 0)) {
        return NULL;
    }

    // --- 1. Parsear el valor de comparación (val_str) al tipo de dato correcto ---
    void *val_parsed = NULL;
    // Un valor proporcionado por el usuario no se considera nulo para la comparación.
    unsigned char esNulo_val_parsed = 0; 

    if (tipo_col == TEXTO) {
        val_parsed = val_str; 
    } else if (tipo_col == NUMERICO) {
        double *d_val = (double *)malloc(sizeof(double));
        if (d_val == NULL) return NULL; // Error de asignación de memoria
        *d_val = atof(val_str); // Convertir cadena a double
        val_parsed = d_val;
    } else if (tipo_col == FECHA) {
        Fecha *f_val = AFecha(val_str); // Asume que AFecha convierte la cadena a struct Fecha*
        if (f_val == NULL) return NULL; // Error al parsear fecha o asignar memoria en AFecha
        val_parsed = f_val;
    } else {
        return NULL; // Tipo de dato desconocido para la comparación
    }

    // --- 2. Crear la estructura del nuevo DataFrame (filtrado) ---
    Dataframe *dfNuevo = malloc(sizeof(Dataframe));
    if (dfNuevo == NULL) {
        // Limpiar memoria si val_parsed fue asignado dinámicamente
        if (tipo_col == NUMERICO && val_parsed != NULL) free(val_parsed);
        if (tipo_col == FECHA && val_parsed != NULL) free(val_parsed);
        return NULL; // Error de asignación de memoria para el nuevo DataFrame
    }
    
    strcpy(dfNuevo->nombre, "filtered_df"); // Nombre genérico para el DF filtrado
    dfNuevo->numColumnas = df->numColumnas;
    dfNuevo->numFilas = 0; // Se inicializa a 0, se actualizará a medida que se añadan filas

    // Asignar capacidad inicial para el array de índices (máximo posible es el número de filas original)
    dfNuevo->indice = malloc(df->numFilas * sizeof(int));
    if (dfNuevo->indice == NULL) {
        if (tipo_col == NUMERICO && val_parsed != NULL) free(val_parsed);
        if (tipo_col == FECHA && val_parsed != NULL) free(val_parsed);
        free(dfNuevo);
        return NULL; // Error de asignación de memoria para índices
    }

    // Asignar memoria para las nuevas columnas y sus datos
    dfNuevo->columna = malloc(df->numColumnas * sizeof(Columna));
    if (dfNuevo->columna == NULL) {
        if (tipo_col == NUMERICO && val_parsed != NULL) free(val_parsed);
        if (tipo_col == FECHA && val_parsed != NULL) free(val_parsed);
        free(dfNuevo->indice);
        free(dfNuevo);
        return NULL; // Error de asignación de memoria para el array de columnas
    }

    for (int j = 0; j < df->numColumnas; j++) {
        // Copiar metadatos de la columna (nombre, tipo)
        strncpy(dfNuevo->columna[j].nombre, df->columna[j].nombre, sizeof(dfNuevo->columna[j].nombre) - 1);
        dfNuevo->columna[j].nombre[sizeof(dfNuevo->columna[j].nombre) - 1] = '\0';
        dfNuevo->columna[j].tipo = df->columna[j].tipo;
        
        // Asignar memoria inicial para los datos de la columna y sus banderas de nulos
        dfNuevo->columna[j].numFilas = df->numFilas; // Se establece a la capacidad máxima
        dfNuevo->columna[j].datos = malloc(df->numFilas * (
            df->columna[j].tipo == TEXTO ? sizeof(char*) :
            (df->columna[j].tipo == NUMERICO ? sizeof(double) : sizeof(Fecha))
        ));
        dfNuevo->columna[j].esNulo = malloc(df->numFilas * sizeof(unsigned char));

        if (dfNuevo->columna[j].datos == NULL || dfNuevo->columna[j].esNulo == NULL) {
            // Manejo de error: liberar la memoria ya asignada para dfNuevo y sus columnas
            for (int k = 0; k < j; k++) { 
                LiberarColumna(&dfNuevo->columna[k]); // Asume que LiberarColumna existe
            }
            free(dfNuevo->columna);
            free(dfNuevo->indice);
            free(dfNuevo);
            if (tipo_col == NUMERICO && val_parsed != NULL) free(val_parsed);
            if (tipo_col == FECHA && val_parsed != NULL) free(val_parsed);
            return NULL; // Error de asignación de memoria para datos/nulos de columna
        }
    }

    // --- 3. Iterar a través de las filas y aplicar la condición de filtro ---
    int new_row_idx = 0; // Índice para las filas del nuevo DataFrame

    for (int i = 0; i < df->numFilas; i++) {
        // Obtener el valor de la columna a filtrar para la fila actual
        void *dato_from_col = NULL;
        unsigned char esNulo_from_col = col->esNulo[df->indice[i]];

        // Obtener dato_from_col solo si no es nulo, de lo contrario se pasa NULL a compararValores
        if (!esNulo_from_col) {
            if (col->tipo == NUMERICO) {
                dato_from_col = &((double *)col->datos)[df->indice[i]];
            } else if (col->tipo == FECHA) {
                dato_from_col = &((Fecha *)col->datos)[df->indice[i]];
            } else if (col->tipo == TEXTO) {
                dato_from_col = ((char **)col->datos)[df->indice[i]];
            }
        }
        
        // --- Realizar la comparación basada en 'comp' (cadena del operador) ---
        int condition_met = 0;
        int cmp_result;

        // Llamada a compararValores con todos los argumentos, incluyendo los estados de nulidad
        cmp_result = compararValores(dato_from_col, esNulo_from_col, val_parsed, esNulo_val_parsed, tipo_col);

        if (strcmp(comp, "eq") == 0) {
            if (cmp_result == 0) {
                condition_met = 1;
            }
        } else if (strcmp(comp, "neq") == 0) {
            if (cmp_result != 0) {
                condition_met = 1;
            }
        } else if (strcmp(comp, "gt") == 0) { // Mayor que (Greater Than)
            if (cmp_result > 0) {
                condition_met = 1;
            }
        } else if (strcmp(comp, "lt") == 0) { // Menor que (Less Than)
            if (cmp_result < 0) {
                condition_met = 1;
            }
        } else if (strcmp(comp, "get") == 0) { // Mayor o igual que (Greater than or Equal to)
            if (cmp_result >= 0) {
                condition_met = 1;
            }
        } else if (strcmp(comp, "let") == 0) { // Menor o igual que (Less than or Equal to)
            if (cmp_result <= 0) {
                condition_met = 1;
            }
        }

        // Si se cumple la condición, copiar la fila completa al nuevo DataFrame
        if (condition_met) {
            dfNuevo->indice[new_row_idx] = new_row_idx; // Índice simple para el DF filtrado

            for (int j = 0; j < df->numColumnas; j++) {
                if (df->columna[j].esNulo[df->indice[i]]) { // Usar el índice del DF original para verificar nulos
                    dfNuevo->columna[j].esNulo[new_row_idx] = 1;
                } else {
                    dfNuevo->columna[j].esNulo[new_row_idx] = 0;
                    if (df->columna[j].tipo == TEXTO) {
                        // strdup asigna nueva memoria, que debe ser liberada posteriormente
                        ((char **)dfNuevo->columna[j].datos)[new_row_idx] = strdup(((char **)df->columna[j].datos)[df->indice[i]]);
                        // En un entorno de producción, se debe verificar si strdup devuelve NULL.
                    } else if (df->columna[j].tipo == NUMERICO) {
                        ((double *)dfNuevo->columna[j].datos)[new_row_idx] = ((double *)df->columna[j].datos)[df->indice[i]];
                    } else if (df->columna[j].tipo == FECHA) {
                        ((Fecha *)dfNuevo->columna[j].datos)[new_row_idx] = ((Fecha *)df->columna[j].datos)[df->indice[i]];
                    }
                }
            }
            new_row_idx++; // Incrementar el contador de filas del nuevo DataFrame
        }
    }

    // --- 4. Ajustes finales a dfNuevo y limpieza de memoria temporal ---
    dfNuevo->numFilas = new_row_idx; // Establecer el número real de filas encontradas

    // Reasignar la memoria a los tamaños exactos para eficiencia (opcional, pero buena práctica)
    if (dfNuevo->numFilas == 0) { // Si no hay filas después del filtro
        free(dfNuevo->indice);
        dfNuevo->indice = NULL; // Evitar puntero colgante
    } else {
        dfNuevo->indice = realloc(dfNuevo->indice, dfNuevo->numFilas * sizeof(int));
    }

    for (int j = 0; j < dfNuevo->numColumnas; j++) {
        dfNuevo->columna[j].numFilas = dfNuevo->numFilas; // Actualizar numFilas para cada columna
        if (dfNuevo->numFilas == 0) {
            free(dfNuevo->columna[j].datos);
            dfNuevo->columna[j].datos = NULL;
            free(dfNuevo->columna[j].esNulo);
            dfNuevo->columna[j].esNulo = NULL;
        } else {
            dfNuevo->columna[j].datos = realloc(dfNuevo->columna[j].datos, dfNuevo->numFilas * (
                dfNuevo->columna[j].tipo == TEXTO ? sizeof(char*) :
                (dfNuevo->columna[j].tipo == NUMERICO ? sizeof(double) : sizeof(Fecha))
            ));
            dfNuevo->columna[j].esNulo = realloc(dfNuevo->columna[j].esNulo, dfNuevo->numFilas * sizeof(unsigned char));
            // En un entorno de producción, se debería manejar si realloc devuelve NULL.
        }
    }

    // Liberar el valor de comparación parseado si fue asignado dinámicamente
    if (tipo_col == NUMERICO && val_parsed != NULL) {
        free(val_parsed);
    } else if (tipo_col == FECHA && val_parsed != NULL) {
        free(val_parsed);
    }

    return dfNuevo; // Devolver el nuevo DataFrame filtrado
}

Columna* CopiarColumna(const Columna *colOriginal) {
    Columna *nuevaCol = (Columna *)malloc(sizeof(Columna));
    if (!nuevaCol) {
        printf("Error al asignar memoria para la nueva columna.\n");
        return NULL;
    }

    strncpy(nuevaCol->nombre, colOriginal->nombre, sizeof(nuevaCol->nombre));
    nuevaCol->tipo = colOriginal->tipo;
    nuevaCol->numFilas = colOriginal->numFilas;

    // Asignar memoria para los datos y copiar según el tipo de dato
    if (colOriginal->tipo == TEXTO) {
        nuevaCol->datos = malloc(nuevaCol->numFilas * sizeof(char *));
        if (!nuevaCol->datos) {
            printf("Error al asignar memoria para los datos de la columna.\n");
            free(nuevaCol);
            return NULL;
        }
        for (int i = 0; i < nuevaCol->numFilas; i++) {
            ((char **)nuevaCol->datos)[i] = strdup(((char **)colOriginal->datos)[i]);  // Copia profunda de cada cadena
        }
    } else if (colOriginal->tipo == NUMERICO) {
        nuevaCol->datos = malloc(nuevaCol->numFilas * sizeof(double));
        if (!nuevaCol->datos) {
            printf("Error al asignar memoria para los datos de la columna.\n");
            free(nuevaCol);
            return NULL;
        }
        memcpy(nuevaCol->datos, colOriginal->datos, nuevaCol->numFilas * sizeof(double));
    } else if (colOriginal->tipo == FECHA) {
        nuevaCol->datos = malloc(nuevaCol->numFilas * sizeof(Fecha));
        if (!nuevaCol->datos) {
            printf("Error al asignar memoria para los datos de la columna.\n");
            free(nuevaCol);
            return NULL;
        }
        memcpy(nuevaCol->datos, colOriginal->datos, nuevaCol->numFilas * sizeof(Fecha));
    }

    // Asignar memoria para esNulo y copiar
    nuevaCol->esNulo = (unsigned char *)malloc(nuevaCol->numFilas * sizeof(unsigned char));
    if (!nuevaCol->esNulo) {
        printf("Error al asignar memoria para esNulo.\n");
        free(nuevaCol->datos);
        free(nuevaCol);
        return NULL;
    }
    memcpy(nuevaCol->esNulo, colOriginal->esNulo, nuevaCol->numFilas * sizeof(unsigned char));

    return nuevaCol;
}

int Save(Dataframe *df, char *nombre, char separator) {
    if (df == NULL || nombre == NULL || df->numColumnas == 0 || df->numFilas == 0) {
        return -1; // DataFrame inválido o nombre de archivo inválido
    }

    FILE *f = fopen(nombre, "w");
    if (f == NULL) {
        return -1; // No se pudo abrir el archivo para escritura
    }

    // 1. Escribir la cabecera (nombres de las columnas)
    for (int j = 0; j < df->numColumnas; j++) {
        fprintf(f, "%s", df->columna[j].nombre);
        if (j < df->numColumnas - 1) {
            fprintf(f, "%c", separator); // Usar el separador proporcionado
        }
    }
    fprintf(f, "\n"); // Nueva línea después de la cabecera

    // 2. Escribir los datos
    for (int i = 0; i < df->numFilas; i++) {
        int original_row_idx = df->indice[i]; // Usar el índice para acceder a la fila original
        for (int j = 0; j < df->numColumnas; j++) {
            if (df->columna[j].esNulo[original_row_idx]) {
                fprintf(f, ""); // Escribir cadena vacía para nulos (puedes usar "NULL" o una convención si lo deseas)
            } else {
                if (df->columna[j].tipo == TEXTO) {
                    fprintf(f, "%s", ((char **)df->columna[j].datos)[original_row_idx]);
                } else if (df->columna[j].tipo == NUMERICO) {
                    // %.15g keeps up to 15 significant digits (exact for
                    // values read from the CSV); "%.g" kept only one.
                    fprintf(f, "%.15g", ((double *)df->columna[j].datos)[original_row_idx]);
                } else if (df->columna[j].tipo == FECHA) {
                    // Asegúrate de que tu struct Fecha tenga los campos tm_mday, tm_mon, tm_year
                    // El formato "dd/mm/aaaa" es común, ajusta según tu definición de Fecha
                    Fecha *f_data = &((Fecha *)df->columna[j].datos)[original_row_idx];
                    // Same YYYY/MM/DD format that Load() accepts. AFecha() stores
                    // the full year in tm_year (not years since 1900).
                    fprintf(f, "%04d/%02d/%02d", f_data->tm_year, f_data->tm_mon + 1, f_data->tm_mday);
                }
            }
            if (j < df->numColumnas - 1) {
                fprintf(f, "%c", separator); // Usar el separador
            }
        }
        fprintf(f, "\n"); // Nueva línea después de cada fila
    }

    fclose(f);
    return 0; // Éxito
}

int AgregarColumna(Dataframe *df, char *nombre , TipoDato t) {
    Columna *nuevaMem = (Columna *)realloc(df->columna, (df->numColumnas + 1) * sizeof(Columna));
    
    if (nuevaMem == NULL) {
        return -1;  // Error al reasignar memoria
    }
    df->columna = nuevaMem;
    
    df->columna[df->numColumnas] = *CrearColumna(nombre, t, df->numFilas);
    
    df->numColumnas++;
    
    return 1;
}

int Quarter(Dataframe *df, Columna *col, char* nombre) {
    if (AgregarColumna(df, nombre, TEXTO) == -1) {
        return -1;
    }



    for (int i = 0; i < df->numFilas; i++) {

        df->columna[df->numColumnas - 1].esNulo[i] = 0;
        if (col->esNulo[df->indice[i]] == 1) {
            ((char **)df->columna[df->numColumnas - 1].datos)[df->indice[i]] = strdup("#N/A");
            continue;
        }
        int month = ((Fecha *)col->datos)[df->indice[i]].tm_mon + 1;
        char *quarter;

        if (month >= 1 && month <= 3) {
            quarter = "Q1";
        } else if (month >= 4 && month <= 6) {
            quarter = "Q2";
        } else if (month >= 7 && month <= 9) {
            quarter = "Q3";
        } else {
            quarter = "Q4";
        }

        ((char **)df->columna[df->numColumnas - 1].datos)[df->indice[i]] = strdup(quarter);
    }

    return 1;

}

void LiberarColumna(Columna *columna) {
    if (columna->tipo == TEXTO) {
        for (int i = 0; i < columna->numFilas; i++) {
            if (((char **)columna->datos)[i] != NULL && columna->esNulo[i]==0) {
                free(((char **)columna->datos)[i]);
            }
        }
    }
    if (columna->datos != NULL) {  
        free(columna->datos);
    }
    if (columna->esNulo != NULL) {  
        free(columna->esNulo);
    }
}

void LiberarDataframe(Dataframe *df) {
    if (df != NULL) {
        for (int i = 0; i < df->numColumnas; i++) {
            LiberarColumna(&df->columna[i]);
        }
        if (df->columna != NULL) {
            free(df->columna);
        }
        if (df->indice != NULL) {
            free(df->indice);
        }
        free(df);  
    }
}

int EliminarColumna(Dataframe *df, int indiceColumna) {

    LiberarColumna(&df->columna[indiceColumna]);
    
    // Mover las columnas
    for (int i = indiceColumna; i < df->numColumnas - 1; i++) {
        df->columna[i] = df->columna[i + 1];  // Copiar la siguiente columna
    }
    
    df->numColumnas--;
    
    // Reasignar memoria
    Columna *nueva_memoria = (Columna *)realloc(df->columna, df->numColumnas * sizeof(Columna));
    
    if (nueva_memoria == NULL && df->numColumnas > 0) {
        return -1;  // Error al reasignar memoria
    }
    
    df->columna = nueva_memoria;
    
    return 1;  // Columna eliminada con éxito
}

int EliminarNulos (Nodo* nodo, Dataframe *df, Columna *col) {
    Dataframe *dfNuevo;
    int numFilas = df->numFilas;
    int n=0;
    char cab[1000]="";
    TipoDato *t;
    t = (TipoDato *)malloc(df->numColumnas * sizeof(TipoDato));

    for (int i = 0; i < df->numFilas; i++) {
        if (col->esNulo[i] == 1) {
            numFilas--;
        }
    }

    if (numFilas == df->numFilas) {
        printf("No hay valores nulos\n");
        return 1;
    }

    for (int i = 0; i < df->numColumnas; i++) {
        t[i] = df->columna[i].tipo;
        strcat(cab, df->columna[i].nombre);
        if (i < df->numColumnas - 1)
            strcat(cab, ",");
    }

    dfNuevo = CrearDataFrame(numFilas, df->numColumnas, t, cab);

    if (dfNuevo == NULL)
        return -1;

    for (int i = 0; i < df->numFilas; i++) {

        if (col->esNulo[df->indice[i]] == 0) {

            for (int j = 0; j < df->numColumnas; j++) {

                if(df->columna[j].esNulo[df->indice[i]]==1) {
                    dfNuevo->columna[j].esNulo[n] = 1;
                }
                else {
                    dfNuevo->columna[j].esNulo[n] = 0;
                    if (df->columna[j].tipo == TEXTO) {
                        ((char **)dfNuevo->columna[j].datos)[n] = strdup(((char **)df->columna[j].datos)[df->indice[i]]);
                    }
                    else if (df->columna[j].tipo == NUMERICO) {
                        ((double *)dfNuevo->columna[j].datos)[n] = ((double *)df->columna[j].datos)[df->indice[i]];
                    }
                    else if (df->columna[j].tipo == FECHA) {
                        ((Fecha *)dfNuevo->columna[j].datos)[n] = ((Fecha *)df->columna[j].datos)[df->indice[i]];
                    }
                }
            }
            n++;
        }
    }
    nodo->df = dfNuevo;

    LiberarDataframe(df);

    return 1;
}


void cargarDataframe(Dataframe *df, int id) {
    sprintf(df->nombre, "df%d", id);  
    
}
Dataframe *dfActivo; // Declare dfActivo
Nodo *nodoActivo; // Declare nodoActivo

Nodo* BuscarDataFramePorNombre(Lista *laGranLista, const char *nombre) {
    if (laGranLista == NULL || nombre == NULL) {
        return NULL;
    }
    Nodo *actual = laGranLista->primero;
    while (actual != NULL) {
        // Comprobar si el nombre coincide
        if (actual->df != NULL && strcmp(actual->df->nombre, nombre) == 0) {
            return actual; // Se encontró el DataFrame
        }
        actual = actual->siguiente;
    }
    return NULL; // No se encontró ningún DataFrame con ese nombre
}

int AppendDataframe(Dataframe *dfDest, Dataframe *dfSrc) {
    if (dfDest == NULL || dfSrc == NULL) return -1;
    if (dfDest->numColumnas != dfSrc->numColumnas) return -1; // Should be pre-checked by caller

    int old_dest_rows = dfDest->numFilas;
    int src_rows = dfSrc->numFilas;
    if (src_rows == 0) return 0; // Nothing to append

    int new_total_rows = old_dest_rows + src_rows;

    // 1. Reallocate dfDest->indice
    int *new_indice_ptr = realloc(dfDest->indice, new_total_rows * sizeof(int));
    if (new_indice_ptr == NULL && new_total_rows > 0) return -1; // Allocation error
    dfDest->indice = new_indice_ptr;
    for (int i = 0; i < src_rows; i++) {
        // New rows in dfDest will have indices from old_dest_rows upwards
        dfDest->indice[old_dest_rows + i] = old_dest_rows + i; 
    }

    // 2. For each column, reallocate data and esNulo, then copy data
    for (int j = 0; j < dfDest->numColumnas; j++) {
        Columna *colDest = &dfDest->columna[j];
        Columna *colSrc = &dfSrc->columna[j];

        if (colDest->tipo != colSrc->tipo) return -1; // Should be pre-checked

        // Reallocate esNulo for dfDest column
        unsigned char *new_esNulo = realloc(colDest->esNulo, new_total_rows * sizeof(unsigned char));
        if (new_esNulo == NULL && new_total_rows > 0) { /* TODO: More robust error handling to undo previous reallocs */ return -1; }
        colDest->esNulo = new_esNulo;

        // Reallocate datos for dfDest column
        void *new_datos = NULL;
        size_t element_size = 0;
        if (colDest->tipo == TEXTO) element_size = sizeof(char*);
        else if (colDest->tipo == NUMERICO) element_size = sizeof(double);
        else if (colDest->tipo == FECHA) element_size = sizeof(Fecha);
        
        if (element_size > 0) {
            new_datos = realloc(colDest->datos, new_total_rows * element_size);
            if (new_datos == NULL && new_total_rows > 0) { /* TODO: More robust error handling */ return -1; }
            colDest->datos = new_datos;
        } else { return -1; /* Should not happen with valid types */ }

        // Copy data from colSrc to colDest
        for (int i = 0; i < src_rows; i++) {
            int dest_storage_idx = old_dest_rows + i; // Actual storage index in dfDest's column arrays
            int src_storage_idx = dfSrc->indice[i];   // Actual storage index in dfSrc's column arrays (respecting dfSrc's sort/filter)

            colDest->esNulo[dest_storage_idx] = colSrc->esNulo[src_storage_idx];
            if (!colSrc->esNulo[src_storage_idx]) {
                if (colDest->tipo == TEXTO) {
                    char *src_string = ((char**)colSrc->datos)[src_storage_idx];
                    if (src_string == NULL) { // Should not happen if esNulo is false, but defensive
                        ((char**)colDest->datos)[dest_storage_idx] = NULL;
                    } else {
                        ((char**)colDest->datos)[dest_storage_idx] = strdup(src_string);
                        if (((char**)colDest->datos)[dest_storage_idx] == NULL) {
                            /* strdup failed, critical memory error. Mark as N/A or return error */
                            colDest->esNulo[dest_storage_idx] = 1; // Mark as N/A if strdup fails
                            // Or return -1; for now, let's mark as N/A to avoid partial appends
                        }
                    }
                } else if (colDest->tipo == NUMERICO) {
                    ((double*)colDest->datos)[dest_storage_idx] = ((double*)colSrc->datos)[src_storage_idx];
                } else if (colDest->tipo == FECHA) {
                    ((Fecha*)colDest->datos)[dest_storage_idx] = ((Fecha*)colSrc->datos)[src_storage_idx];
                }
            } else { // Source value is NULL
                 if (colDest->tipo == TEXTO) {
                    ((char**)colDest->datos)[dest_storage_idx] = NULL;
                 }
                 // For NUMERICO/FECHA, esNulo flag is sufficient.
            }
        }
        colDest->numFilas = new_total_rows;
    }

    // 3. Update dfDest->numFilas
    dfDest->numFilas = new_total_rows;

    return 0; // Success
}

Dataframe* LoadDataframeFromFile(char *nombre, char separator) {
    FILE *f = fopen(nombre, "r");
    if (f == NULL) {
        return NULL; // Error opening file
    }

    char line[2048];
    char delimiter_str[2];
    delimiter_str[0] = separator;
    delimiter_str[1] = '\0';

    int numColumnas = 0;
    char **headers = NULL;
    TipoDato *tipos_columnas = NULL;
    Dataframe *df = NULL;
    int current_rows_in_buffer = 0;
    char **data_lines = NULL;

    // PASO 1: Read headers (same as in Load)
    if (fgets(line, sizeof(line), f) != NULL) {
        Trim(line);
        if (strlen(line) == 0) { fclose(f); return NULL; }
        char *temp_line_for_count = strdup(line);
        if (temp_line_for_count == NULL) { fclose(f); return NULL; }
        char *token = strtok(temp_line_for_count, delimiter_str);
        while (token != NULL) { numColumnas++; token = strtok(NULL, delimiter_str); }
        free(temp_line_for_count);
        if (numColumnas == 0) { fclose(f); return NULL; }
        headers = (char**)malloc(numColumnas * sizeof(char*));
        if (headers == NULL) { fclose(f); return NULL; }
        char *temp_line_for_headers = strdup(line);
        if (temp_line_for_headers == NULL) { free(headers); fclose(f); return NULL; }
        int i = 0;
        token = strtok(temp_line_for_headers, delimiter_str);
        while (token != NULL && i < numColumnas) {
            Trim(token);
            headers[i] = strdup(token);
            if (headers[i] == NULL) {
                for (int k = 0; k < i; k++) free(headers[k]);
                free(headers); free(temp_line_for_headers); fclose(f); return NULL;
            }
            i++;
            token = strtok(NULL, delimiter_str);
        }
        free(temp_line_for_headers);
    } else { fclose(f); return NULL; }

    // PASO 2: Read types (same as in Load)
    if (fgets(line, sizeof(line), f) != NULL) {
        Trim(line);
        if (strlen(line) == 0) { 
            for(int j=0; j<numColumnas; j++) free(headers[j]); free(headers);
            fclose(f); return NULL; 
        }
        tipos_columnas = (TipoDato*)malloc(numColumnas * sizeof(TipoDato));
        if (tipos_columnas == NULL) { 
            for(int j=0; j<numColumnas; j++) free(headers[j]); free(headers); fclose(f); return NULL; 
        }
        char *temp_line_for_types = strdup(line);
        if (temp_line_for_types == NULL) { 
            for(int j=0; j<numColumnas; j++) free(headers[j]); free(headers); free(tipos_columnas); fclose(f); return NULL; 
        }
        char *token = strtok(temp_line_for_types, delimiter_str);
        int i = 0;
        while (token != NULL && i < numColumnas) {
            Trim(token);
            tipos_columnas[i] = Tipo(token);
            i++;
            token = strtok(NULL, delimiter_str);
        }
        free(temp_line_for_types);
    } else { 
        for(int j=0; j<numColumnas; j++) free(headers[j]); free(headers);
        fclose(f); return NULL; 
    }
    
    // PASO 3: Read data lines into buffer (same as in Load)
    while (fgets(line, sizeof(line), f) != NULL) {
        Trim(line);
        if (strlen(line) == 0) continue;
        data_lines = (char**)realloc(data_lines, (current_rows_in_buffer + 1) * sizeof(char*));
        if (data_lines == NULL) { 
            for (int k = 0; k < current_rows_in_buffer; k++) free(data_lines[k]);
            for(int j=0; j<numColumnas; j++) free(headers[j]); free(headers);
            free(tipos_columnas); fclose(f); return NULL; 
        }
        data_lines[current_rows_in_buffer] = strdup(line);
        if (data_lines[current_rows_in_buffer] == NULL) {
            for (int k = 0; k < current_rows_in_buffer; k++) free(data_lines[k]); free(data_lines);
            for(int j=0; j<numColumnas; j++) free(headers[j]); free(headers);
            free(tipos_columnas); fclose(f); return NULL;
        }
        current_rows_in_buffer++;
    }

    // PASO 4: Create DataFrame structure (similar to Load, but no Nodo)
    df = (Dataframe*)malloc(sizeof(Dataframe));
    if (df == NULL) { 
        for (int k = 0; k < current_rows_in_buffer; k++) free(data_lines[k]); if(data_lines) free(data_lines);
        for(int j=0; j<numColumnas; j++) free(headers[j]); free(headers);
        free(tipos_columnas); fclose(f); return NULL; 
    }
    df->nombre[0] = '\0'; // Temporary DFs might not need a name from this function
    df->numColumnas = numColumnas;
    df->numFilas = current_rows_in_buffer;
    df->indice = NULL;
    df->columna = (Columna*)malloc(numColumnas * sizeof(Columna));
    if (df->columna == NULL) { 
        free(df); 
        for (int k = 0; k < current_rows_in_buffer; k++) free(data_lines[k]); if(data_lines) free(data_lines);
        for(int j_h=0; j_h<numColumnas; j_h++) free(headers[j_h]); free(headers);
        free(tipos_columnas); fclose(f); return NULL; 
    }

    for(int j = 0; j < numColumnas; j++) {
        Columna *temp_col = CrearColumna(headers[j], tipos_columnas[j], df->numFilas);
        if (temp_col == NULL) {
            for (int k = 0; k < j; k++) LiberarColumna(&df->columna[k]);
            free(df->columna); free(df);
            for (int k = 0; k < current_rows_in_buffer; k++) free(data_lines[k]); if(data_lines) free(data_lines);
            for(int k_h=0; k_h<numColumnas; k_h++) free(headers[k_h]); free(headers); 
            free(tipos_columnas); fclose(f); return NULL;
        }
        df->columna[j] = *temp_col;
        free(temp_col);
    }

    if (df->numFilas > 0) {
        df->indice = (int*)malloc(df->numFilas * sizeof(int));
        if (df->indice == NULL) { 
            LiberarDataframe(df);
            for (int k = 0; k < current_rows_in_buffer; k++) free(data_lines[k]); if(data_lines) free(data_lines);
            for(int j_h=0; j_h<numColumnas; j_h++) free(headers[j_h]); free(headers);
            free(tipos_columnas); fclose(f); return NULL; 
        }
        for(int k=0; k<df->numFilas; k++) df->indice[k] = k;
    } else {
        df->indice = NULL;
    }

    // PASO 5: Populate DataFrame with data (same as in Load)
    for (int k = 0; k < df->numFilas; k++) {
        char *current_data_line = data_lines[k];
        char *temp_data_line = strdup(current_data_line);
        if (temp_data_line == NULL) { 
            LiberarDataframe(df);
            for (int l = 0; l < current_rows_in_buffer; l++) free(data_lines[l]); if(data_lines) free(data_lines);
            for(int j_h=0; j_h<numColumnas; j_h++) free(headers[j_h]); free(headers);
            free(tipos_columnas); fclose(f); return NULL; 
        }
        char *token = strtok(temp_data_line, delimiter_str);
        int col_idx = 0;
        while (token != NULL && col_idx < numColumnas) {
            Trim(token);
            unsigned char is_null = 0;
            if (strlen(token) == 0) { is_null = 1; }
            else {
                if (df->columna[col_idx].tipo == TEXTO) {
                    ((char**)df->columna[col_idx].datos)[k] = strdup(token);
                    if (((char**)df->columna[col_idx].datos)[k] == NULL && strlen(token) > 0) { is_null = 1; }
                } else if (df->columna[col_idx].tipo == NUMERICO) {
                    char *endptr; double val_num = strtod(token, &endptr);
                    if (*endptr != '\0' || !Validar_decimal(token)) { is_null = 1; }
                    else { ((double*)df->columna[col_idx].datos)[k] = val_num; }
                } else if (df->columna[col_idx].tipo == FECHA) {
                    Fecha *val_fecha = AFecha(token);
                    if (val_fecha == NULL) { is_null = 1; }
                    else { ((Fecha*)df->columna[col_idx].datos)[k] = *val_fecha; free(val_fecha); }
                }
            }
            df->columna[col_idx].esNulo[k] = is_null;
            if (is_null && df->columna[col_idx].tipo == TEXTO) {
                ((char**)df->columna[col_idx].datos)[k] = NULL;
            }
            token = strtok(NULL, delimiter_str);
            col_idx++;
        }
        while(col_idx < numColumnas) {
            df->columna[col_idx].esNulo[k] = 1;
            if (df->columna[col_idx].tipo == TEXTO) {
                 ((char**)df->columna[col_idx].datos)[k] = NULL;
            }
            col_idx++;
        }
        free(temp_data_line);
    }

    // Free temporary buffers
    for (int k_dl = 0; k_dl < current_rows_in_buffer; k_dl++) free(data_lines[k_dl]); 
    if (data_lines) free(data_lines);
    for (int j = 0; j < numColumnas; j++) { if (headers[j]) free(headers[j]); }
    if (headers) free(headers);
    if (tipos_columnas) free(tipos_columnas);

    fclose(f);
    return df; // Return the DataFrame, not a Nodo
}

// Cosas nuevas 
// Funcion de describe 
/*-------------------------------------------------
 *  stats_col : resumen descriptivo
 *------------------------------------------------*/
static int cmp_double(const void *a, const void *b)
{
    double da = *(const double*)a;
    double db = *(const double*)b;
    return (da > db) - (da < db);
}

Stats stats_col(const Columna *c)
{
    Stats s = {0,0,0,0,0,0,0,0};
    if (!c || c->tipo != NUMERICO)         /* sólo para columnas numéricas */
        return s;

    const double *v = (const double*)c->datos;
    int nfilas = c->numFilas;

    /* 1-pasada: media, min, max y count */
    double sum = 0, minv = 0, maxv = 0;
    int k = 0;
    for (int i = 0; i < nfilas; ++i)
        if (!c->esNulo[i]) {
            double x = v[i];
            if (k == 0) { minv = maxv = x; }
            if (x < minv) minv = x;
            if (x > maxv) maxv = x;
            sum += x;
            ++k;
        }

    s.count = k;
    if (k == 0) return s;          /* todo nulo → struct vacío */

    s.mean = sum / k;

    /* 2-pasada: varianza con Welford */
    double mean = 0, M2 = 0;
    int idx = 0;
    for (int i = 0; i < nfilas; ++i)
        if (!c->esNulo[i]) {
            double x = v[i];
            ++idx;
            double delta = x - mean;
            mean += delta / idx;
            M2   += delta * (x - mean);
        }
    s.std = (idx > 1) ? sqrt(M2 / (idx - 1)) : 0.0;

    /* 3-Cuartiles: copiar valores válidos, ordenar y tomar percentiles */
    double *tmp = (double*)malloc(k * sizeof(double));
    if (!tmp) {                          /* sin memoria → al menos min/max */
        s.min = minv;  s.max = maxv;  s.q1 = s.med = s.q3 = s.mean;
        return s;
    }

    int j = 0;
    for (int i = 0; i < nfilas; ++i)
        if (!c->esNulo[i])
            tmp[j++] = v[i];

    qsort(tmp, k, sizeof(double), cmp_double);

    #define Q(p)  tmp[(int)round((p) * (k - 1))]
    s.min = tmp[0];
    s.q1  = Q(0.25);
    s.med = Q(0.50);
    s.q3  = Q(0.75);
    s.max = tmp[k - 1];

    free(tmp);
    return s;
}

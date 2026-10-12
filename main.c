#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include <dirent.h> 
#include "lib.h"

// gcc lib.c main.c -o main.exe

int main() {

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // Variables
    char prompt[100] = "?";
    char com[100];
    char aux[100];
    char * numeroDf;

    char * comtok[100];
    char separadores[10] = " \n\t\r";
   
    int numarg, n, asc, i, numDF = 0;

    Lista * laGranLista = NULL;
    Nodo * nodoActivo = NULL, * nodoAux = NULL;
    Dataframe * dfActivo = NULL;
    Columna * col1 = NULL, * col2 = NULL, * colRes = NULL;

    laGranLista = malloc(sizeof(Lista));
    laGranLista -> numDFs = 0;
    laGranLista -> primero = NULL;

    // Logica
    Datos_alumno();

    while (1) {
        // Establecer el color del prompt a blanco
        SetConsoleTextAttribute(hConsole, 7);
        
        // --- LÓGICA DE IMPRESIÓN DEL PROMPT ---
        // Construye el prompt con el formato [nombre:filas,columnas]:>
        // si hay un DataFrame activo, o [?]:> si no lo hay.
        if (dfActivo == NULL) {
            printf("[?]:>");
        } else {
            // Construye la cadena del prompt con el nombre y las dimensiones del DF activo
            sprintf(aux, "%s: %d,%d", dfActivo->nombre, dfActivo->numFilas, dfActivo->numColumnas);
            printf("[%s]:>", aux); // Imprime el prompt usando la cadena construida en 'aux'
        }
        
        // --- LECTURA DEL COMANDO (CORRECCIÓN CRÍTICA: USAR fgets) ---
        // fgets es seguro porque permite especificar el tamaño máximo a leer.
        if (fgets(com, sizeof(com), stdin) == NULL) {
            // Manejar error de lectura o fin de archivo (Ctrl+D en Linux/macOS, Ctrl+Z en Windows)
            printf("Error al leer el comando o fin de entrada.\n");
            break; // Salir del bucle principal
        }
        
        // Eliminar el salto de línea '\n' que fgets añade al final de la cadena
        // strcspn busca la primera ocurrencia de '\n' y devuelve su índice.
        // Asignar 0 (carácter nulo) en ese índice "corta" la cadena.
        com[strcspn(com, "\n")] = 0;

        // --- TOKENIZACIÓN DEL COMANDO (MÁS ROBUSTA) ---
        numarg = 0; // Reinicia el contador de argumentos para cada comando

        // Obtener el primer token
        comtok[numarg] = strtok(com, separadores);
        
        // Iterar para obtener los tokens restantes
        while (comtok[numarg] != NULL) {
            numarg++; // Incrementar el número de argumentos
            // Obtener el siguiente token
            comtok[numarg] = strtok(NULL, separadores);
        }

        // --- MANEJO DE COMANDO VACÍO O SOLO ESPACIOS ---
        if (numarg == 0) {
            continue; 
        }

        if (strcmp(comtok[0], "quit") == 0) {
            if (numarg != 1) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: quit no acepta argumentos\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            nodoAux = laGranLista -> primero;
            for (int i = 0; i < laGranLista -> numDFs; i++) {
                LiberarDataframe(nodoAux -> df);
                nodoAux = nodoAux -> siguiente;
            }

            SetConsoleTextAttribute(hConsole, 2);
            printf("EXIT PROGRAM\n");
            SetConsoleTextAttribute(hConsole, 7);

            return 0;
        }else if (strcmp(comtok[0], "load") == 0) {
            // 'load' necesita 1 o 2 argumentos adicionales: nombre_fichero [separador]
            if (numarg < 2 || numarg > 3) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: 'load' necesita el nombre de un archivo y opcionalmente un separador.\n");
                printf("Uso: load <nombre_fichero> [<sep>]\n");
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            char *originalFilename = comtok[1];
            char finalFilename[256]; // Buffer para el nombre de archivo final (con o sin .csv)
            char separator = ','; // Separador por defecto: coma

            // Copiar el nombre de archivo original
            strncpy(finalFilename, originalFilename, sizeof(finalFilename) - 1);
            finalFilename[sizeof(finalFilename) - 1] = '\0'; // Asegurar terminación nula

            // Comprobar si el nombre de archivo ya tiene una extensión
            // Buscamos el último '.' en la cadena
            char *dot = strrchr(finalFilename, '.');

            // Si no hay punto, o el punto está al principio (ej. ".hiddenfile") o al final (ej. "file."),
            // asumimos que no hay una extensión significativa.
            if (dot == NULL || dot == finalFilename || *(dot + 1) == '\0') {
                // Si no hay extensión o es inválida, añadimos .csv
                if (strlen(finalFilename) + strlen(".csv") < sizeof(finalFilename)) {
                    strcat(finalFilename, ".csv");
                } else {
                    SetConsoleTextAttribute(hConsole, 4); // Rojo
                    printf("Error: El nombre del archivo es demasiado largo para añadir '.csv'.\n");
                    SetConsoleTextAttribute(hConsole, 7); // Blanco
                    continue;
                }
            }
            // Si ya tiene una extensión (ej. "data.txt"), se usará tal cual.

            if (numarg == 3) { // Si se proporciona un separador
                if (strlen(comtok[2]) != 1) { // El separador debe ser un solo carácter
                    SetConsoleTextAttribute(hConsole, 4); // Rojo
                    printf("Error: El separador debe ser un solo caracter.\n");
                    SetConsoleTextAttribute(hConsole, 7); // Blanco
                    continue;
                }
                separator = comtok[2][0];
            }

            // Llama a la función Load con el nombre de archivo final y el separador
            nodoAux = Load(finalFilename, separator);

            if (nodoAux == NULL) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                // Usamos originalFilename en el mensaje de error si no se pudo cargar
                printf("Error: No se pudo cargar el archivo '%s'.\n", originalFilename);
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            } else {
                // Si nodoAux NO es NULL, entonces es seguro acceder a sus miembros
                dfActivo = nodoAux->df;
                nodoActivo = nodoAux; 

                snprintf(dfActivo->nombre, sizeof(dfActivo->nombre), "df%d", laGranLista->numDFs);
                anyadirNodo(nodoAux, laGranLista);

                SetConsoleTextAttribute(hConsole, 2); // Verde
                // Usamos originalFilename para el mensaje de éxito
                printf("Archivo '%s' cargado correctamente como '%s' con separador '%c'.\n", originalFilename, dfActivo->nombre, separator);
                SetConsoleTextAttribute(hConsole, 7); // Blanco
            }
            continue; 
        }else if (strcmp(comtok[0], "describe") == 0) {

            /* ----- SINTAXIS :  describe col1 [col2 ...]  ------ */
            if (dfActivo == NULL) {
                SetConsoleTextAttribute(hConsole, 4);  /* rojo */
                printf("Error: no hay dataframe activo.\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            if (numarg < 2) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: 'describe' requiere al menos un nombre de columna.\n");
                printf("Uso: describe <col1> [col2] ...\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            /* 1. Recorre cada columna solicitada */
            for (int i = 1; i < numarg; ++i) {
                Columna *c = BuscarColumna(dfActivo, comtok[i]);
                
                if (!c) {
                    SetConsoleTextAttribute(hConsole, 4);
                    printf("Error: columna '%s' no existe.\n", comtok[i]);
                    SetConsoleTextAttribute(hConsole, 7);
                    continue; // Salta a la siguiente columna de la lista de argumentos
                }

                if (c->tipo != NUMERICO) {
                    // Opcional: informar que la columna no es numérica
                    // printf("Info: La columna '%s' no es de tipo NUMERICO y será ignorada.\n", c->nombre);
                    continue; // Salta a la siguiente columna
                }

                Stats st = stats_col(c);

                /* 2. Salida en VERDE */
                SetConsoleTextAttribute(hConsole, 2);
                printf("%s  (n=%d)\n", c->nombre, st.count);
                printf("   min = %.6g   Q1  = %.6g   med = %.6g\n", st.min, st.q1, st.med);
                printf("   mean= %.6g   Q3  = %.6g   max = %.6g\n", st.mean, st.q3, st.max);
                printf("   std = %.6g\n",  st.std);
                SetConsoleTextAttribute(hConsole, 7);
            }
            continue;
        }else if (strcmp(comtok[0], "meta") == 0) {
            if (numarg != 1) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: meta no acepta argumentos\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            if (dfActivo == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: No hay dataframe activo\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            printf("Entrando en meta\n");
            SetConsoleTextAttribute(hConsole, 2);

            for (int i = 0; i < dfActivo -> numColumnas; i++) {
                printf("Columna %d: %s\t", i, dfActivo -> columna[i].nombre);
                if (dfActivo -> columna[i].tipo == TEXTO) {
                    printf("Tipo: TEXTO\t");
                } else if (dfActivo -> columna[i].tipo == NUMERICO) {
                    printf("Tipo: NUMERICO\t");
                } else if (dfActivo -> columna[i].tipo == FECHA) {
                    printf("Tipo: FECHA\t");
                }
                printf("Filas Nulas: %d\n", numNulos(dfActivo -> columna[i]));
            }

            SetConsoleTextAttribute(hConsole, 7);
        }else if (strcmp(comtok[0], "view") == 0) {
            if (dfActivo == NULL) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: No hay dataframe activo.\n");
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            int n_val = 10; // Valor por defecto: 10 primeras filas
            int start_idx;
            int end_idx;
            int step; // Para controlar el orden de iteración (1 para adelante, -1 para atrás)

            // 1. Validar el número de argumentos
            if (numarg > 2) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: El comando 'view' no acepta mas de un argumento.\n");
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            // 2. Procesar el argumento 'n' si se proporciona
            if (numarg == 2) {
                // Asumo que Validar_entero devuelve 1 si la cadena es un entero válido (positivo o negativo)
                // y 0 en caso contrario.
                if (Validar_entero(comtok[1]) == 0) {
                    SetConsoleTextAttribute(hConsole, 4); // Rojo
                    printf("Error: El parametro 'n' debe ser un numero entero.\n");
                    SetConsoleTextAttribute(hConsole, 7); // Blanco
                    continue;
                }
                n_val = atoi(comtok[1]); // n_val puede ser positivo o negativo

                if (n_val == 0) {
                    SetConsoleTextAttribute(hConsole, 4); // Rojo
                    printf("Error: El valor de 'n' no puede ser 0.\n");
                    SetConsoleTextAttribute(hConsole, 7); // Blanco
                    continue;
                }
            }

            // 3. Determinar el rango de filas a mostrar y el orden
            if (n_val > 0) // Mostrar las primeras 'n_val' filas (orden normal)
 {
                start_idx = 0;
                end_idx = n_val - 1;
                step = 1;

                // Ajustar si 'n_val' es mayor que el número total de filas
                if (end_idx >= dfActivo->numFilas) {
                    end_idx = dfActivo->numFilas - 1; // Mostrar hasta la última fila disponible
                }
            } else { // n_val < 0: Mostrar las últimas abs(n_val) filas en orden inverso
                int abs_n = abs(n_val); // Obtenemos el valor absoluto de n

                // Calcular el índice de inicio para la iteración inversa.
                // Queremos mostrar las últimas 'abs_n' filas.
                // Si el DF tiene 10 filas (0-9) y n_val es -3, queremos las filas 7, 8, 9.
                // Para orden inverso: 9, 8, 7.
                // start_idx será el índice de la última fila que queremos mostrar (la más grande).
                start_idx = dfActivo->numFilas - 1; 

                // end_idx será el índice de la primera fila que queremos mostrar (la más pequeña).
                // Si abs_n es 3, numFilas 10, entonces 10 - 3 = 7. La fila con índice 7 es la primera de las últimas 3.
                end_idx = dfActivo->numFilas - abs_n; 
                
                step = -1; // Esto indica que el bucle debe ir hacia atrás (decrementar)

                // Asegurarse de que 'end_idx' no sea negativo (es decir, si abs_n es mayor que numFilas,
                // empezamos desde la fila 0)
                if (end_idx < 0) {
                    end_idx = 0; 
                }
            }

            SetConsoleTextAttribute(hConsole, 2); // Color verde para la salida

            // Mostrar nombres de las columnas
            for (int i = 0; i < dfActivo->numColumnas; i++) {
                printf("%-20s", dfActivo->columna[i].nombre);
            }
            printf("\n");

            // 4. Imprimir las filas según el rango y el orden calculado
            if (step == 1) { // Iterar hacia adelante (para n > 0 o default)
                for (int i = start_idx; i <= end_idx; i++) {
                    for (int j = 0; j < dfActivo->numColumnas; j++) {
                        int data_row_idx = dfActivo->indice[i]; // Obtener el índice real de la fila en el array de datos
                        if (dfActivo->columna[j].esNulo[data_row_idx]) {
                            printf("%-20s", "#N/A");
                        } else {
                            printf("%-20s", printElemento(dfActivo, j, data_row_idx));
                        }
                    }
                    printf("\n");
                }
            } else { // step == -1: Iterar hacia atrás (para n < 0)
                for (int i = start_idx; i >= end_idx; i--) { // Bucle descendente
                    for (int j = 0; j < dfActivo->numColumnas; j++) {
                        int data_row_idx = dfActivo->indice[i]; // Obtener el índice real de la fila en el array de datos
                        if (dfActivo->columna[j].esNulo[data_row_idx]) {
                            printf("%-20s", "#N/A");
                        } else {
                            printf("%-20s", printElemento(dfActivo, j, data_row_idx));
                        }
                    }
                    printf("\n");
                }
            }

            SetConsoleTextAttribute(hConsole, 7); // Restablecer color a blanco
            continue; 
        }else if (strcmp(comtok[0], "sort") == 0){
            if (numarg < 2 || numarg > 3) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: sort necesita 2 o 3 argumentos\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            if (dfActivo == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: No hay dataframe activo\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            col1 = BuscarColumna(dfActivo, comtok[1]);
            if (col1 == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: Columna no encontrada\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            int asc = 1; // Orden ascendente por defecto
            if (numarg == 3) {
                if (strcmp(comtok[2], "asc") == 0) {
                    asc = 1;
                } else if (strcmp(comtok[2], "des") == 0) {
                    asc = 0;
                } else {
                    SetConsoleTextAttribute(hConsole, 4);
                    printf("Error: el tercer argumento tiene que ser 'asc' o 'des'\n");
                    SetConsoleTextAttribute(hConsole, 7);
                    continue;
                }
            }

            SortCol(dfActivo, col1, asc);
            SetConsoleTextAttribute(hConsole, 2);
            printf("Dataframe ordenado por la columna '%s' en orden %s\n", comtok[1], asc ? "ascendente" : "descendente");
            SetConsoleTextAttribute(hConsole, 7);

        }else if (strcmp(comtok[0], "save") == 0) {
            // save [<nombre_fichero>] [<sep>]
            // numarg puede ser 1 (solo 'save'), 2 ('save filename') o 3 ('save filename sep')

            if (dfActivo == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: No hay dataframe activo para guardar.\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            char filename[100]; // Buffer para el nombre del archivo
            char separator = ';'; // Separador por defecto, por ejemplo, ';'

            // Determinar el nombre del archivo
            if (numarg == 1) { // Solo "save"
                // Si no se proporciona nombre, usar el nombre del dataframe + ".csv"
                if (strlen(dfActivo->nombre) == 0) {
                    SetConsoleTextAttribute(hConsole, 4);
                    printf("Error: El dataframe activo no tiene un nombre asignado. Debe proporcionar un nombre de fichero.\n");
                    SetConsoleTextAttribute(hConsole, 7);
                    continue;
                }
                sprintf(filename, "%s.csv", dfActivo->nombre);
            } else if (numarg == 2) { // "save <nombre_fichero>"
                strcpy(filename, comtok[1]);
                // Asegurarse de que tenga la extensión .csv si no la tiene
                if (strlen(filename) < 4 || strcmp(filename + strlen(filename) - 4, ".csv") != 0) {
                    strcat(filename, ".csv");
                }
            } else if (numarg == 3) { // "save <nombre_fichero> <sep>"
                strcpy(filename, comtok[1]);
                // Asegurarse de que tenga la extensión .csv si no la tiene
                if (strlen(filename) < 4 || strcmp(filename + strlen(filename) - 4, ".csv") != 0) {
                    strcat(filename, ".csv");
                }

                // Verificar el separador
                if (strlen(comtok[2]) != 1) {
                    SetConsoleTextAttribute(hConsole, 4);
                    printf("Error: El separador debe ser un solo caracter.\n");
                    SetConsoleTextAttribute(hConsole, 7);
                    continue;
                }
                separator = comtok[2][0];
            } else { // Más de 3 argumentos o menos de 1 (nunca debería pasar menos de 1 si ya entró al if "save")
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: Uso incorrecto del comando save.\n");
                printf("Uso: save [<nombre_fichero>] [<sep>]\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            // Llamar a la función Save con el DataFrame activo, el nombre del archivo y el separador.
            // La función Save en lib.c necesita ser actualizada para aceptar el separador.
            if (Save(dfActivo, filename, separator) == -1) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: No se ha podido guardar el archivo '%s'.\n", filename);
                SetConsoleTextAttribute(hConsole, 7);
            } else {
                SetConsoleTextAttribute(hConsole, 2);
                printf("DataFrame guardado exitosamente en '%s' con separador '%c'.\n", filename, separator);
                SetConsoleTextAttribute(hConsole, 7);
            }
            continue;

        }else if (strcmp(comtok[0], "filter") == 0) {
            // filter <nombre_columna> eq/neq/gt/lt/get/let <valor>
            if (numarg != 4) { // filter (0), nombre_columna (1), operador (2), valor (3)
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: filter necesita 3 argumentos: <nombre_columna> <operador> <valor>\n");
                printf("Uso: filter <nombre_columna> eq/neq/gt/lt/get/let <valor>\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }
            if (dfActivo == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: No hay dataframe activo para filtrar.\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            Columna *columna_a_filtrar = BuscarColumna(dfActivo, comtok[1]);
            if (columna_a_filtrar == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: Columna '%s' no encontrada en el dataframe activo.\n", comtok[1]);
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            // Asegúrate de que los operadores "get" y "let" sean manejados correctamente
            // y de que la función Filtrar sea capaz de comparar los valores según el tipo de la columna.
            // La función Tipo() puede ser útil aquí para el valor 'comtok[3]' si Filtrar lo necesita internamente,
            // pero la firma de Filtrar ya espera el tipo de la columna a filtrar.

            Dataframe *dfFiltrado = Filtrar(dfActivo, columna_a_filtrar, comtok[2], comtok[3], columna_a_filtrar->tipo);

            // Si Filtrar devuelve un nuevo Dataframe (o NULL en caso de error)
            if (dfFiltrado == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: No se pudo aplicar el filtro o no se encontraron filas que cumplan la condición.\n");
                SetConsoleTextAttribute(hConsole, 7);
            } else {
                // Reemplazar el dataframe activo con el dataframe filtrado
                if (nodoActivo != NULL) {
                    LiberarDataframe(nodoActivo->df); // Liberar el dataframe antiguo
                    nodoActivo->df = dfFiltrado;      // Asignar el nuevo dataframe filtrado
                    dfActivo = dfFiltrado;            // Actualizar el puntero dfActivo
                    SetConsoleTextAttribute(hConsole, 2);
                    printf("DataFrame filtrado exitosamente. Nuevo número de filas: %d\n", dfActivo->numFilas);
                    SetConsoleTextAttribute(hConsole, 7);
                } else {
                    SetConsoleTextAttribute(hConsole, 4);
                    printf("Error interno: nodoActivo es NULL al intentar actualizar el dataframe filtrado.\n");
                    SetConsoleTextAttribute(hConsole, 7);
                    LiberarDataframe(dfFiltrado); // Liberar el nuevo dataframe si no se puede asignar
                }
            }
            continue;
        }else if (strcmp(comtok[0], "delnull") == 0) {

            if (numarg != 2) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: delnull no necesita argumentos\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }
            if (dfActivo == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: No hay dataframe activo\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }
            if (BuscarColumna(dfActivo, comtok[1]) == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: Columna no encontrada\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            if (EliminarNulos(nodoActivo, dfActivo, BuscarColumna(dfActivo, comtok[1])) == -1) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: No se ha podido eliminar los nulos\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            } else {
                dfActivo = nodoActivo -> df;
                sprintf(aux, "DF%d: %d,%d", numDF, dfActivo -> numFilas, dfActivo -> numColumnas);
                strcpy(prompt, aux);
            }
        }else if (strcmp(comtok[0], "delcolum") == 0) {

            if (numarg != 2) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: delcolum necesita un argumento\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }
            if (dfActivo == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: No hay dataframe activo\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }
            if (BuscarColumna(dfActivo, comtok[1]) == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: Columna no encontrada\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            for (i = 0; i < dfActivo -> numColumnas; i++) {
                if (strcmp(dfActivo -> columna[i].nombre, comtok[1]) == 0) {
                    break;
                }
            }

            if (EliminarColumna(dfActivo, i) == -1) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: No se ha podido eliminar la columna\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            } else {
                sprintf(aux, "DF%d: %d,%d", numDF, dfActivo -> numFilas, dfActivo -> numColumnas);
                strcpy(prompt, aux);
            }
        }else if (strcmp(comtok[0], "quarter") == 0) {
            if (numarg != 3) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: quarter necesita un argumento\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }
            if (dfActivo == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: No hay dataframe activo\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }
            if (BuscarColumna(dfActivo, comtok[1]) == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: Columna no encontrada\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }
            if (BuscarColumna(dfActivo, comtok[2]) != NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: Columna ya existente\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            Quarter(dfActivo, BuscarColumna(dfActivo, comtok[1]), comtok[2]);
        }else if (strcmp(comtok[0], "name") == 0) {
            if (dfActivo == NULL) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: No hay un dataframe activo para cambiarle el nombre.\n");
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            if (numarg != 2) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: Uso incorrecto del comando 'name'. Sintaxis: name <nuevo_nombre>\n");
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            char *nuevoNombre = comtok[1];

            // Validar la longitud del nombre
            if (strlen(nuevoNombre) > 50) { // Nombre[51] permite 50 + \0
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: El nombre del dataframe no debe exceder de 50 caracteres.\n");
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            // Verificar si el nuevo nombre ya está en uso por OTRO dataframe
            // !!! CORRECCIÓN CRÍTICA: Asegurarse de no comparar con el propio DataFrame activo.
            Nodo *actual = laGranLista->primero;
            int nombreDuplicado = 0;
            while (actual != NULL) {
                // Solo si el DataFrame actual no es el DataFrame activo Y los nombres son iguales
                if (actual->df != dfActivo && strcmp(actual->df->nombre, nuevoNombre) == 0) {
                    nombreDuplicado = 1;
                    break;
                }
                actual = actual->siguiente;
            }

            if (nombreDuplicado) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: Ya existe otro dataframe con el nombre '%s'. Los nombres deben ser unicos.\n", nuevoNombre);
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            // Cambiar el nombre del DataFrame activo
            // strncpy es más seguro para evitar desbordamientos
            strncpy(dfActivo->nombre, nuevoNombre, sizeof(dfActivo->nombre) - 1);
            dfActivo->nombre[sizeof(dfActivo->nombre) - 1] = '\0'; // Asegurar null-termination

            // Confirmar el cambio
            SetConsoleTextAttribute(hConsole, 2); // Verde
            printf("Nombre del dataframe activo cambiado a: '%s'\n", dfActivo->nombre);

            // Actualizar el prompt con SOLO el nuevo nombre del DataFrame activo
            // Esto es crucial para cumplir la especificación del Ejercicio 2.
            strcpy(prompt, dfActivo->nombre);
            
            SetConsoleTextAttribute(hConsole, 7); // Blanco
        }else if (strcmp(comtok[0], "list") == 0) {
            // Verificar si el comando 'list' tiene argumentos (no debería tener)
            if (numarg != 1) { // 'list' es un comando de un solo token, por lo que numarg debe ser 1
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: 'list' no acepta argumentos.\n");
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            // Verificar si no hay dataframes cargados en memoria
            if (laGranLista->primero == NULL) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("No hay dataframes cargados en memoria.\n");
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            // Si hay dataframes, listarlos
            SetConsoleTextAttribute(hConsole, 2); // Verde
            printf("Dataframes cargados:\n"); // Un encabezado simple es bueno aquí

            Nodo *actual = laGranLista->primero;
            while (actual != NULL) {
                // El nombre del DataFrame ya se asigna por defecto o por 'name'
                // No necesitamos la comprobación de strlen(actual->df->nombre) > 0
                // ya que siempre se asignará un nombre (dfX o personalizado).
                
                // Formato de salida según el enunciado: "nombre_1: 87 filas, 5 columnas"
                printf("%s: %d filas, %d columnas\n",
                       actual->df->nombre,
                       actual->df->numFilas,
                       actual->df->numColumnas);

                actual = actual->siguiente;
            }

            SetConsoleTextAttribute(hConsole, 7); // Volver al color blanco
        }else if (strcmp(comtok[0], "switch") == 0) {
            if (numarg != 2) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: switch necesita exactamente un argumento\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            char *nombreDf = comtok[1];
            Nodo *actual = laGranLista->primero;
            int encontrado = 0;
            while (actual != NULL) {
                if (strcmp(actual->df->nombre, nombreDf) == 0) {
                    dfActivo = actual->df;
                    nodoActivo = actual; // Actualiza nodoActivo también
                    encontrado = 1;
                    break;
                }
                actual = actual->siguiente;
            }

            if (encontrado) {
                SetConsoleTextAttribute(hConsole, 2);
                printf("Cambiado al dataframe: %s\n", dfActivo->nombre);

                // Cambia el prompt para mostrar el nombre del nuevo dataframe activo
                sprintf(aux, "%s: %d,%d", dfActivo->nombre, dfActivo->numFilas, dfActivo->numColumnas);
                strcpy(prompt, aux);

                SetConsoleTextAttribute(hConsole, 7);
            } else {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: no se encontró un dataframe con ese nombre\n");
                SetConsoleTextAttribute(hConsole, 7);
            if (numarg != 2) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: switch necesita exactamente un argumento\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            nodoAux = BuscarDataFramePorNombre(laGranLista, comtok[1]);
            if (nodoAux == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: no se encontró un dataframe con ese nombre\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            dfActivo = nodoAux->df;
            nodoActivo = nodoAux;

            SetConsoleTextAttribute(hConsole, 2);
            printf("Cambiado al dataframe: %s\n", dfActivo->nombre);

            // Cambia el prompt para mostrar el nombre del nuevo dataframe activo
            snprintf(aux, sizeof(aux), "%s: %d,%d", dfActivo->nombre, dfActivo->numFilas, dfActivo->numColumnas);
            strcpy(prompt, aux);

            SetConsoleTextAttribute(hConsole, 7);
            }

        }else if (strcmp(comtok[0], "prefix") == 0) {
            // 1. Verificar si hay un dataframe activo
            if (dfActivo == NULL) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: No hay un dataframe activo. Cargue uno con 'load' primero.\n");
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            // 2. Validar el número de argumentos
            // Sintaxis: prefix <columna_original> <n> <nueva_columna>
            // Tokens esperados: "prefix", "columna_original", "n", "nueva_columna" = 4 tokens
            if (numarg != 4) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: 'prefix' necesita 3 argumentos. Sintaxis: prefix <col_origen> <num_caracteres> <nueva_col_nombre>\n");
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            char *nombreColumnaOriginal = comtok[1];
            char *strNumCaracteres = comtok[2];
            char *nuevoNombreColumna = comtok[3];

            // 3. Validar que 'n' sea un número entero
            if (Validar_entero(strNumCaracteres) == 0) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: El segundo argumento (numero de caracteres 'n') debe ser un numero entero.\n");
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            int n = atoi(strNumCaracteres);

            // 4. Validar que 'n' sea mayor que 0
            if (n <= 0) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: El numero de caracteres 'n' debe ser mayor que 0.\n");
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            // 5. Validar que el nuevo nombre de columna no exista
            if (BuscarColumna(dfActivo, nuevoNombreColumna) != NULL) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: Ya existe una columna con el nombre '%s' en el dataframe activo.\n", nuevoNombreColumna);
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            // 6. Buscar la columna original
            Columna *colOriginal = BuscarColumna(dfActivo, nombreColumnaOriginal);
            if (colOriginal == NULL) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: La columna original '%s' no fue encontrada en el dataframe activo.\n", nombreColumnaOriginal);
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            // 7. Validar que la columna original sea de tipo TEXTO
            if (colOriginal->tipo != TEXTO) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: La columna original '%s' debe ser de tipo TEXTO para usar 'prefix'.\n", nombreColumnaOriginal);
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            // --- Toda la validación ha pasado, proceder a crear la nueva columna ---

            Columna nuevaColumna;
            strncpy(nuevaColumna.nombre, nuevoNombreColumna, sizeof(nuevaColumna.nombre) - 1);
            nuevaColumna.nombre[sizeof(nuevaColumna.nombre) - 1] = '\0'; // Asegurar terminador nulo
            nuevaColumna.tipo = TEXTO;
            nuevaColumna.numFilas = colOriginal->numFilas; // La nueva columna tendrá el mismo número de filas

            // Asignar memoria para los datos y el array de nulos de la nueva columna
            nuevaColumna.datos = malloc(nuevaColumna.numFilas * sizeof(char *));
            nuevaColumna.esNulo = malloc(nuevaColumna.numFilas * sizeof(unsigned char));

            if (nuevaColumna.datos == NULL || nuevaColumna.esNulo == NULL) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: No se pudo asignar memoria para la nueva columna.\n");
                // Es crucial liberar la memoria si una de las asignaciones falla
                free(nuevaColumna.datos);
                free(nuevaColumna.esNulo);
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }

            // Rellenar la nueva columna con los prefijos
            for (int i = 0; i < nuevaColumna.numFilas; i++) {
                // Si el valor original es nulo, el prefijo también lo será
                if (colOriginal->esNulo[dfActivo->indice[i]]) { // Usar dfActivo->indice[i]
                    nuevaColumna.esNulo[i] = 1;
                    ((char **)nuevaColumna.datos)[i] = NULL; // Asegurarse de que el puntero sea NULL
                } else {
                    char *valorOriginal = ((char **)colOriginal->datos)[dfActivo->indice[i]]; // Usar dfActivo->indice[i]
                    size_t len_original = strlen(valorOriginal);
                    size_t chars_to_copy = (n < len_original) ? n : len_original; // Copiar hasta 'n' o el largo real

                    char *prefijo = malloc((chars_to_copy + 1) * sizeof(char));
                    if (prefijo == NULL) {
                        // Manejo de error si malloc falla para una cadena individual
                        // Esto es más complejo, pero para el examen, una salida simple podría bastar
                        SetConsoleTextAttribute(hConsole, 4);
                        printf("Error: Memoria insuficiente al crear prefijos para la fila %d. Operacion abortada.\n", i);
                        // Liberar lo que ya se asignó en nuevaColumna
                        for (int j = 0; j < i; j++) {
                            if (!nuevaColumna.esNulo[j]) {
                                free(((char **)nuevaColumna.datos)[j]);
                            }
                        }
                        free(nuevaColumna.datos);
                        free(nuevaColumna.esNulo);
                        SetConsoleTextAttribute(hConsole, 7);
                        return 1; // Podrías querer salir del main o continuar y manejar esto como error grave
                                 // Para un 'continue' aquí, necesitarías deshacer realloc si ya se hizo.
                                 // Lo más simple es salir.
                    }
                    
                    strncpy(prefijo, valorOriginal, chars_to_copy);
                    prefijo[chars_to_copy] = '\0'; // Asegurar terminador nulo

                    ((char **)nuevaColumna.datos)[i] = prefijo;
                    nuevaColumna.esNulo[i] = 0; // No es nulo
                }
            }

            // Añadir la nueva columna al dataframe activo
            // Primero, reasignar espacio para el array de Columnas
            Columna *temp_columna_ptr = realloc(dfActivo->columna, (dfActivo->numColumnas + 1) * sizeof(Columna));
            if (temp_columna_ptr == NULL) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: No se pudo reasignar memoria para anadir la nueva columna al dataframe.\n");
                // Si realloc falla, debemos liberar la nuevaColumna que ya habíamos creado
                LiberarColumna(&nuevaColumna); // Asume que tienes una función para liberar una Columna
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue;
            }
            dfActivo->columna = temp_columna_ptr;
            
            // Ahora sí, añadir la nueva columna al final del array
            dfActivo->columna[dfActivo->numColumnas] = nuevaColumna;
            dfActivo->numColumnas++; // Incrementar el contador de columnas del DataFrame

            SetConsoleTextAttribute(hConsole, 2); // Verde
            printf("Nueva columna '%s' creada con los primeros %d caracteres de la columna '%s'.\n", nuevoNombreColumna, n, nombreColumnaOriginal);
            SetConsoleTextAttribute(hConsole, 7); // Blanco
        }else if (strcmp(comtok[0], "add") == 0) {
            if (dfActivo == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: No hay dataframe activo. Cargue o seleccione uno primero.\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            // add <nombre_fichero> [<sep>]
            if (numarg < 2 || numarg > 3) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: 'add' necesita el nombre de un archivo y opcionalmente un separador.\n");
                printf("Uso: add <nombre_fichero> [<sep>]\n");
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            char *originalFilename = comtok[1];
            char finalFilename[256];
            char separator = ','; // Default separator

            strncpy(finalFilename, originalFilename, sizeof(finalFilename) - 1);
            finalFilename[sizeof(finalFilename) - 1] = '\0';

            char *dot = strrchr(finalFilename, '.');
            if (dot == NULL || dot == finalFilename || *(dot + 1) == '\0') {
                if (strlen(finalFilename) + strlen(".csv") < sizeof(finalFilename)) {
                    strcat(finalFilename, ".csv");
                } else {
                    SetConsoleTextAttribute(hConsole, 4);
                    printf("Error: El nombre del archivo es demasiado largo para anadir '.csv'.\n");
                    SetConsoleTextAttribute(hConsole, 7);
                    continue;
                }
            }

            if (numarg == 3) {
                if (strlen(comtok[2]) != 1) {
                    SetConsoleTextAttribute(hConsole, 4);
                    printf("Error: El separador debe ser un solo caracter.\n");
                    SetConsoleTextAttribute(hConsole, 7);
                    continue;
                }
                separator = comtok[2][0];
            }

            Dataframe *dfTemp = LoadDataframeFromFile(finalFilename, separator);

            if (dfTemp == NULL) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: No se pudo cargar el archivo '%s' para anadir.\n", originalFilename);
                SetConsoleTextAttribute(hConsole, 7);
                continue;
            }

            // Check if column count and types match dfActivo
            if (dfActivo->numColumnas != dfTemp->numColumnas) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: El numero de columnas no coincide con el dataframe activo (%d vs %d).\n", dfActivo->numColumnas, dfTemp->numColumnas);
                SetConsoleTextAttribute(hConsole, 7);
                LiberarDataframe(dfTemp);
                continue;
            }
            for (int i = 0; i < dfActivo->numColumnas; i++) {
                if (dfActivo->columna[i].tipo != dfTemp->columna[i].tipo) {
                    SetConsoleTextAttribute(hConsole, 4);
                    printf("Error: El tipo de la columna '%s' (indice %d) no coincide (activo: %d, nuevo: %d).\n", dfActivo->columna[i].nombre, i, dfActivo->columna[i].tipo, dfTemp->columna[i].tipo);
                    SetConsoleTextAttribute(hConsole, 7);
                    LiberarDataframe(dfTemp);
                    continue; // Exit the loop and the 'add' command processing
                }
            }

            if (AppendDataframe(dfActivo, dfTemp) != 0) {
                SetConsoleTextAttribute(hConsole, 4);
                printf("Error: No se pudo anadir el dataframe.\n");
                SetConsoleTextAttribute(hConsole, 7);
            } else {
                SetConsoleTextAttribute(hConsole, 2);
                printf("Dataframe '%s' anadido correctamente. Nuevas dimensiones: %d filas, %d columnas.\n", originalFilename, dfActivo->numFilas, dfActivo->numColumnas);
                SetConsoleTextAttribute(hConsole, 7);
                // Prompt will be updated automatically at the start of the next loop
            }
            LiberarDataframe(dfTemp); // Free the temporary dataframe
            continue;
        } else if (numarg == 1) { // Solo un token, que se asume es el nombre del DF
            char *nombreBuscado = comtok[0]; // El primer y único token es el nombre a buscar

            // No se permiten argumentos adicionales si se intenta usar solo el nombre del DF
            // (esta comprobación ya se hace implícitamente con numarg == 1)

            // Buscar el DataFrame por su nombre (dfX o nombre personalizado)
            nodoAux = BuscarDataFramePorNombre(laGranLista, nombreBuscado);

            if (nodoAux == NULL) {
                SetConsoleTextAttribute(hConsole, 4); // Rojo
                printf("Error: No se encontro un dataframe con el nombre '%s'.\n", nombreBuscado);
                SetConsoleTextAttribute(hConsole, 7); // Blanco
                continue; // El dataframe activo permanece sin cambios.
            }

            // Si se encontró el DataFrame, establecerlo como activo
            dfActivo = nodoAux->df;
            nodoActivo = nodoAux; // Asegurarse de que el nodo activo también se actualice

            SetConsoleTextAttribute(hConsole, 2); // Verde
            printf("Cambiado al dataframe: '%s'.\n", dfActivo->nombre);
            SetConsoleTextAttribute(hConsole, 7); // Blanco

            // No es necesario actualizar la variable `prompt` global aquí.
            // La lógica al inicio del bucle `while(1)` se encarga de construir y
            // mostrar el prompt con el nombre y las dimensiones del `dfActivo` actualizado.
        }else {

            SetConsoleTextAttribute(hConsole, 4);
            printf("Error: Comando no reconocido\n");
            SetConsoleTextAttribute(hConsole, 7);
        }
    }
    return 0;
}


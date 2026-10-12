// Console text colours that work on Windows, Linux and macOS.
//
// The original program called SetConsoleTextAttribute() from <windows.h>
// directly, so it only built on Windows. set_color() keeps that behaviour
// on Windows and uses ANSI escape codes elsewhere. Colours are skipped when
// stdout is not a terminal, so redirected output (logs, tests) stays plain.
//
// Colour codes are the Windows console attributes used by the program:
//   2 = green (success output), 4 = red (errors), 7 = default (white).
#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>

static void set_color(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)color);
}

#else
#include <unistd.h>

static void set_color(int color) {
    if (!isatty(STDOUT_FILENO)) {
        return;
    }
    fflush(stdout);
    switch (color) {
        case 2:  fputs("\033[32m", stdout); break;  // green
        case 4:  fputs("\033[31m", stdout); break;  // red
        default: fputs("\033[0m", stdout);  break;  // reset
    }
}
#endif

#endif

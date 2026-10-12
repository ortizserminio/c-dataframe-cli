# Build the interpreter and run the tests (Linux, macOS, or MinGW on Windows).
#   make          -> builds ./dataframe
#   make test     -> unit tests (tests/test_lib.c) + end-to-end tests
CC      = gcc
CFLAGS  ?= -std=gnu99 -Wall -O2
LDLIBS  = -lm

.PHONY: all test unit e2e clean
all: dataframe

dataframe: main.c lib.c lib.h console.h
	$(CC) $(CFLAGS) main.c lib.c -o $@ $(LDLIBS)

build/test_lib: tests/test_lib.c lib.c lib.h
	@mkdir -p build
	$(CC) $(CFLAGS) -I. tests/test_lib.c lib.c -o $@ $(LDLIBS)

unit: build/test_lib
	./build/test_lib

e2e: dataframe
	bash tests/test_cli.sh ./dataframe

test: unit e2e

clean:
	rm -rf build dataframe dataframe.exe

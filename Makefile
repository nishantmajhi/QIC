CC      = gcc
CFLAGS  = -Wall -Wextra -I include
FPIC    = -fPIC

# Internal implementation objects (compiled with -fPIC for shared lib)
LIB_SRC = src/parser.c src/utils.c src/normalize.c src/solver.c src/qic.c
LIB_OBJ = $(LIB_SRC:.c=.o)

ifeq ($(OS),Windows_NT)
	# ── Windows / MinGW ───────────────────────────────────────────────
	# On Windows a DLL needs a companion import library (libqic.dll.a)
	# so the linker can resolve -lqic.  We generate both in one shot
	# using --out-implib.  -rpath is not supported; the DLL must live
	# next to the executable (or on PATH) at runtime.
	EXE        = .exe
	LIB_FILE   = libqic.dll
	IMPLIB     = libqic.dll.a
	LIB_FLAGS  = -shared -o $(LIB_FILE) -Wl,--out-implib,$(IMPLIB)
	LINK_FLAGS = -L. -lqic -lm
	# `make` here may be driven by cmd.exe, or by sh.exe from MSYS or Git
	# for Windows when one is on PATH.  The two need different remove
	# commands, and running cmd's under sh does not fail loudly — it
	# silently creates a file called `nul` holding the error message.
    ifeq ($(findstring sh.exe,$(SHELL)),)
	CLEAN_OBJ  = -del /Q src\*.o tests\*.o 2>nul
	CLEAN_BIN  = -del /Q main.exe test.exe libqic.dll libqic.dll.a 2>nul
    else
	CLEAN_OBJ  = -rm -f src/*.o tests/*.o
	CLEAN_BIN  = -rm -f main.exe test.exe libqic.dll libqic.dll.a
    endif
else
	# ── Linux / macOS ─────────────────────────────────────────────────
	EXE        =
	LIB_FILE   = libqic.so
	IMPLIB     =
	LIB_FLAGS  = -shared -o $(LIB_FILE)
	LINK_FLAGS = -L. -lqic -lm -Wl,-rpath,.
	CLEAN_OBJ  = -rm -f src/*.o tests/*.o
	CLEAN_BIN  = -rm -f main test libqic.so
endif

# ── default target ────────────────────────────────────────────────
all: main

# ── shared library ────────────────────────────────────────────────
#   Linux  → libqic.so
#   Windows → libqic.dll + libqic.dll.a (import library for -lqic)
$(LIB_FILE): $(LIB_OBJ)
	$(CC) $(LIB_FLAGS) $^ -lm

# ── main executable (links against the shared library) ────────────
main: src/main.o $(LIB_FILE)
	$(CC) $(CFLAGS) -o $@$(EXE) src/main.o $(LINK_FLAGS)

# ── test executable (links against the shared library) ────────────
# depends on `main` so the suite can also drive the CLI end to end;
# without it those checks report SKIP rather than failing.
test: tests/test.o $(LIB_FILE) main
	$(CC) $(CFLAGS) -o $@$(EXE) tests/test.o $(LINK_FLAGS)
	./$@$(EXE)

# ── compile rule for library objects (needs -fPIC) ────────────────
src/%.o: src/%.c
	$(CC) $(CFLAGS) $(FPIC) -c -o $@ $<

# ── compile rule for test objects ─────────────────────────────────
tests/%.o: tests/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# ── clean ─────────────────────────────────────────────────────────
clean:
	$(CLEAN_OBJ)
	$(CLEAN_BIN)

.PHONY: all test clean
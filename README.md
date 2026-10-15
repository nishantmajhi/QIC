# QIC

A small C library that turns a typed quadratic equation into its integer
coefficients, and optionally into its roots.

You hand it a string — `2x^2 + 3x - 5 = 0`, or `3x + x² + 2`, or anything in
between — and it gives you back `a`, `b` and `c` for the equivalent equation in
standard form `ax² + bx + c = 0`. It is built as a shared library so that a
program in another language can link against it; the companion Python GUI in
`../QIC - Frontend` does exactly that through `ctypes`.

---

## Building

Requires GCC and Make. There are no other dependencies.

```sh
make            # Linux and macOS
mingw32-make    # Windows, MinGW / TDM-GCC
```

| Platform        | Produces                                   |
| --------------- | ------------------------------------------ |
| Linux / macOS   | `libqic.so`, `main`                        |
| Windows (MinGW) | `libqic.dll`, `libqic.dll.a`, `main.exe`   |

`make test` builds the library, the CLI and the test suite, then runs it.
`make clean` removes everything that was built.

Nothing built is committed to the repository, so a fresh clone has to be built
once before either the CLI or the Python frontend can find the library.

---

## The API

`qic.h` is the only header a caller needs to include.

```c
#include "qic.h"

QuadraticExpression qic_parse(const char *raw_input);
QuadraticSolution   qic_solve(const char *raw_input);
```

```c
typedef enum {
    NONE,              /* 0 — success */
    INVALID_INPUT,     /* 1 — a character outside [0-9 x ^ = + -] */
    INVALID_DEGREE,    /* 2 — no x² term, or an exponent that is not 2 */
    INVALID_EQUATION,  /* 3 — structurally malformed */
    NOT_QUADRATIC,     /* 4 — parsed cleanly, but a == 0 */
    COEFFICIENT_RANGE, /* 5 — a coefficient does not fit in an int */
    ALLOC_FAILED       /* 6 — out of memory */
} ParseError;

typedef struct { int a, b, c; ParseError error; } QuadraticExpression;

typedef struct { double real, imag; } ComplexNumber;
typedef struct { ComplexNumber root1, root2; } QuadraticSolution;
```

**Always check `.error` before reading `a`, `b` and `c`.** When it is anything
but `NONE` the coefficients are not meaningful.

`qic_solve` returns `NAN` for both roots if the input cannot be parsed or is
not a quadratic. It gives no reason for the failure; call `qic_parse` first
when you need to tell a user what was wrong.

`root1` is the root taken with `+√D`, `root2` the one taken with `−√D`. When
the discriminant is negative the two are a conjugate pair and `.imag` is
non-zero.

### Example

```c
#include "qic.h"
#include <stdio.h>

int main(void) {
    QuadraticExpression e = qic_parse("2X^2 + 3X - 5 = 0");
    if (e.error != NONE) return 1;
    printf("a=%d b=%d c=%d\n", e.a, e.b, e.c);      /* a=2 b=3 c=-5 */

    QuadraticSolution s = qic_solve("2X^2 + 3X - 5 = 0");
    printf("x1=%g x2=%g\n", s.root1.real, s.root2.real);  /* x1=1 x2=-2.5 */
    return 0;
}
```

```sh
# Linux — rpath so the loader finds the .so
gcc myprogram.c -I path/to/QIC/include -L path/to/QIC -lqic -lm \
    -Wl,-rpath,path/to/QIC -o myprogram

# Windows — links against the import library; libqic.dll must sit next to
# the .exe at runtime, or somewhere on PATH
gcc myprogram.c -I path/to/QIC/include -L path/to/QIC -lqic -lm -o myprogram.exe
```

---

## What it accepts

Whitespace is stripped and letters are folded to lowercase before anything
else happens, so `  X^2 + 3X + 2  ` and `x^2+3x+2` are the same input.

A side of an equation is a run of terms, each of which is an optional run of
signs, an optional integer, and an optional `x` with an optional `^2`:

| Input               | Read as              | Why it is worth saying |
| ------------------- | -------------------- | ---------------------- |
| `x^2+3x+2`          | x²+3x+2 = 0          | a bare side is set equal to zero |
| `x^2+3x+2=0`        | x²+3x+2 = 0          | |
| `2x^2+3x=x+5`       | 2x²+2x−5 = 0         | both sides are parsed and subtracted |
| `2x^2+3x=x^2+5`     | x²+3x−5 = 0          | x² may appear on both sides |
| `3x+x^2+2`          | x²+3x+2 = 0          | terms may be in any order |
| `x^2+2x+3x+1`       | x²+5x+1 = 0          | repeated terms accumulate |
| `1+2+x^2+3x`        | x²+3x+3 = 0          | there is no limit on the number of terms |
| `x^2`               | x² = 0               | missing terms are zero |
| `-x^2+2x+1`         | −x²+2x+1 = 0         | an implicit coefficient is ±1 |
| `x^2+-3x+2`         | x²−3x+2 = 0          | runs of signs fold to their net sign |
| `x^2+1=`            | x²+1 = 0             | an empty side is zero |

### What it rejects

| Input              | Error               |
| ------------------ | ------------------- |
| `2.5x^2+1`         | `INVALID_INPUT` — no decimals or fractions |
| `ax^2+bx+c=0`      | `INVALID_INPUT` — one variable, spelled `x` |
| `x+1=0`            | `INVALID_DEGREE` — no x² term |
| `x^3+2x+1`         | `INVALID_DEGREE` — the exponent must be 2 |
| `x^2=1=2`          | `INVALID_EQUATION` — more than one `=` |
| `x^2+`             | `INVALID_EQUATION` — a sign with no term after it |
| `0x^2+3x+1`        | `NOT_QUADRATIC` — a is zero |
| `x^2+3x=x^2+5`     | `NOT_QUADRATIC` — the x² terms cancel |
| `99999999999x^2+1` | `COEFFICIENT_RANGE` — will not fit in an int |

`NOT_QUADRATIC` is reported rather than `NONE` because a caller that trusts
`.error` and then divides by `2a` would divide by zero. The coefficients are
still parsed; a caller that wants them can read them and decide for itself.

---

## CLI

`main` is an ordinary consumer of the library and a convenient way to try it.

```sh
./main                        # prompts for an equation
./main "2x^2+3x-5=0"          # prints the standard form
./main -s "2x^2+3x-5=0"       # and solves it; -s may go either side
```

```
$ ./main -s "2x^2+3x-5=0"
The standard form of your equation is: 2x^2+3x-5=0
x1 = 1
x2 = -2.5

$ ./main -s "x^2+1"
The standard form of your equation is: 1x^2+0x+1=0
x1 = (0, 1i)
x2 = (0, -1i)
```

Complex roots print as `(real, imaginary i)`. The exit status is 0 on success
and 1 on any parse error or on being asked to solve something that is not a
quadratic.

---

## Layout

```
QIC/
├── include/
│   ├── qic.h          ← the public API; the only header callers need
│   ├── parser.h       ← QuadraticExpression and ParseError
│   ├── solver.h       ← QuadraticSolution
│   ├── normalize.h
│   └── utils.h
├── src/
│   ├── qic.c          ← façade: normalise → validate → parse → solve
│   ├── normalize.c    ← whitespace and case
│   ├── utils.c        ← input validation, printing, error messages
│   ├── parser.c       ← the term scanner
│   ├── solver.c       ← the quadratic formula
│   └── main.c         ← CLI, a consumer of the library like any other
├── tests/
│   └── test.c         ← 368 checks; see below
└── Makefile
```

`parser.c` and `solver.c` contain no I/O and `solver.c` does not know that
strings exist, which is what makes the suite below possible.

---

## Tests

```sh
make test
```

368 checks, no framework, linked against the built library exactly as an
external caller would be. They cover parsing, the error codes, both root
formulas, and the CLI — the last by running the built `main` under `popen` and
matching its output, which is how the one output regression during this
project's cleanup was caught.

Roots are asserted with a *relative* tolerance. That is not fussiness: one of
the equations in the suite has roots of 2×10⁹ and 5×10⁻¹⁰, and an absolute
tolerance would cheerfully accept an answer of zero for the second — which is
precisely the bug those checks exist to catch.

---

## Limitations

- Integer coefficients only, each of which must fit in an `int`.
- One variable, spelled `x`, of degree 2.
- The roots are `double`; the parse is exact, the arithmetic is not.

## Licence

Free for personal, non-commercial use. You may use, study and modify this
software for yourself, but you may not use it commercially, and you may not
share or publish it, in its original form or modified. See [LICENSE](LICENSE)
for the full terms.

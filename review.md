# Code Review — QIC

**Remediated 2026-09-18 on branch `cleanup/2026-09-18`. This file is now a
record rather than a to-do list.** The original review of 2026-09-04 follows
the corrections below, annotated with what was actually found.

Suite: `mingw32-make test` (or `make test`), expecting **368 passed, 0 failed**.

---

## Corrections to the first review

Three claims in the original review were wrong, and one was reachable only by
a different route than the one described. They are corrected here because a
review that sends the next reader to fix the wrong thing is worse than one
that misses something.

**"There are no tests" is wrong.** `tests/test.c` existed and held 152 checks,
all of which passed against the original code. It covered parsing only — it
never touched the solver, which is where the arithmetic defects were, and
several of its expectations encoded the parser's bugs as correct behaviour.
That is why the review's author probably did not notice it: `make test` passes
cleanly on the unfixed code. It has been rewritten and extended rather than
replaced.

**Critical issue 1's reproducer does not reproduce.** The review said
`x^2 + 3x = x^2 + 5` reaches the solver with `a = 0`. It does not: that input
was rejected outright with `INVALID_EQUATION`, because `parse_input` required
`count_char('^', expression) == 1` and this string has two. Every equation
with a quadratic term on both sides was rejected the same way — which is a
worse bug than the one the review was describing, and it is the reason the
described one could not happen.

The underlying defect is real, by another route. `0x^2+3x+1` parses to
`a = 0` with `error = NONE`, and that exact input was in the project's own
test suite asserting exactly that. The review was also wrong that the divide
happens through `qic_solve`: that function did guard `exp.a == 0` and returned
`NAN`, and `main.c` guarded it too. The unguarded path is `solve()` called
directly — which is what any caller doing `qic_parse` then `solve` does, and
what `main.c` itself does two lines later. Against the original, that returns
`nan` and `-inf` as the two "roots".

**"Five compiled binaries are committed" undercounts.** `git ls-files` showed
eleven: the five named, plus `src/main.o`, `src/normalize.o`, `src/parser.o`,
`src/qic.o`, `src/solver.o` and `src/utils.o`.

**The allocation list is incomplete.** The review lists five unchecked
allocations in `parser.c`. It misses `normalize.c`'s `malloc`, which is the
one on the path every single call takes.

---

## What was fixed

Every item below was reproduced against the original code before being
changed. The suite as written kills the original process with a segmentation
fault after **74 failures**; with the NULL check removed so the run can finish,
the original scores **290 passed, 76 failed**. The fixed code scores 368/0.

### From the original review

**Division by zero when `a` is zero.** `parse_input` now reports a new
`NOT_QUADRATIC` error when the combined `a` is zero, and `solve()` — which is
exported and so cannot assume its caller checked — returns `NAN` for both
roots instead of dividing by `2a`. The coefficients are still filled in, so a
caller that wants them can have them. `main.c` treats `NOT_QUADRATIC` as "print
the standard form, refuse to solve", which is byte-for-byte what it already
did when it checked `exp.a` itself.

**The discriminant overflowed in int arithmetic.** `b*b - 4*a*c` was evaluated
entirely in `int` before being widened. The coefficients are now widened
first. `x^2+100000x+1` returned −31224.58 and −68775.42 against the original;
the true roots are −1.0×10⁻⁵ and −99999.99999. A second case,
`x^2+x-1000000000`, is worse than wrong: `4*a*c` wrapped to a positive number,
which flipped the sign of the discriminant, so the original reported two
complex roots for an equation with two real ones.

**Catastrophic cancellation.** Replaced with the stable form
`q = -(b + sign(b)·√D)/2`, `x₁ = q/a`, `x₂ = c/q`. For `x^2+2000000000x+1` the
naive formula computes the small root as the difference of two numbers that
are equal to the last bit, and returns exactly `0` — which is not a root of
that equation at all. The suite asserts roots with a *relative* tolerance for
this reason; an absolute tolerance passes the wrong answer.

**Unchecked allocations.** Mostly deleted rather than checked. The rewritten
scanner does not mutate what it reads, so the two `strdup`s in
`parse_both_sided` are gone and the right-hand side is now a pointer into the
existing buffer instead of a copy. One `malloc` remains, for the left-hand
side, and it is checked; `normalize`'s is checked and reported as
`ALLOC_FAILED`.

**The duplicated `print_complex` branch.** Collapsed. The review guessed the
intent was a `+` sign for the positive case, but the format is a
comma-separated pair — `(0, 1i)` — so a sign was never needed and the output
is unchanged.

**The character-counting validation.** Replaced with an actual term scanner,
which is what fixed four of the bugs below at once.

**`INT_BUFFER_SIZE`.** Gone. It existed to round-trip a coefficient through
`atoi` and `snprintf` in order to work out how many characters to skip; the
scanner tracks its own position and has no need of it.

**`.gitignore` and the committed binaries.** Added, and all eleven artefacts
untracked with `git rm --cached`, which leaves them on disk. `libqic.dll` and
`main.exe` have since been rebuilt from the fixed source and are present, so
the Python frontend next door still finds a library. The Linux pair
(`libqic.so` and `main`) are gone from the working tree — they were stale ELF
binaries that nothing here could run, and they remain in git history.
`LICENSE` added (MIT, matching the other projects in this archive).

**`make clean` created a file called `nul`.** Not in the original review, and
not really a code bug, but it left rubbish in the tree. The Makefile's Windows
branch assumed cmd.exe, and `make` on this machine picks up `sh.exe` from Git
for Windows instead, under which `2>nul` is a redirect into a file rather than
a discard. The branch now detects which shell it got.

### Bugs the first review did not find

**The exponent was never checked.** `parse_input` only tested for the
*presence* of `^`, and the parser then consumed a fixed three characters for
`x^2`. So `x^3+2x+1` parsed as a=1, b=2, c=1 with no error — a cubic silently
solved as a quadratic. `x^9`, `x^1`, `x^0` and `x^10` all did the same. The
scanner now reads the exponent and requires it to be 2.

**Like terms overwrote each other instead of accumulating.** The parser
assigned `result->b = num` rather than `+=`, so in `x^2+2x+3x+1` the `3x`
discarded the `2x` and the answer was b=3 where it should be 5. It bit
two-sided equations too: `x^2+2x+1=x+x` gave b=1 instead of 0.

**Terms beyond the third were dropped.** The parse loop ran exactly three
times. `1+2+x^2+3x` — a constant written before the x terms is enough — came
back as a=1, b=0, c=2 instead of a=1, b=3, c=3.

**`x^2` on both sides was rejected.** Described under the corrections above.
`2x^2+3x=x^2+5` is now read as x²+3x−5=0.

**Coefficients outside int range produced garbage with no error.** `atoi` has
undefined behaviour on overflow. `2147483648x^2+x+1` came back as
c=−2147483648, and `99999999999x^2+1` as a=9. Parsing now accumulates in
`long long` and reports `COEFFICIENT_RANGE`. Both sides can individually be in
range and still leave it once subtracted, so that is checked too.

**Runs of mixed signs corrupted the expression.** The sign-collapsing code
wrote a `-` into `expression[sign_count]`, which is not the sign run at all
but the character after it — the first digit of the coefficient.
`x^2+-3x+2` came back as b=1, c=0 instead of b=−3, c=2, and `x^2---3x+2` the
same. Signs are now folded by counting, without writing to the string.

**Leading zeros mis-parsed.** `007x^2+08` gave c=7 and dropped the 8;
`x^2+007x+1` gave b=0 and c=7. A single leading zero happened to come out
right by consuming a spare iteration of the three-term loop, which is why it
was easy to miss.

**`qic_parse(NULL)` segfaulted** in `strlen`, inside `normalize`. Now
`INVALID_INPUT`.

**`-0.0`, introduced during this pass and caught by the new CLI test.**
Widening the coefficients to `double` made `-b` produce negative zero when b
is 0, so `x^2+1` began printing `x1 = (-0, 1i)` where it had printed `(0, 1i)`.
Normalised in `solve()`. Worth recording because it is the one defect here
that the 350-odd library assertions could not see — `fabs(0.0 - -0.0)` is 0 —
and only the check that runs the real binary and matches its output caught it.

---

## Deliberately not done

**The struct layout is unchanged.** `QuadraticExpression` is still four
`int`s in the same order, because `../QIC - Frontend/qic_api.py` declares it
to `ctypes` field by field. The three new `ParseError` values are appended, so
0–3 still mean what they always meant.

**`left_trim` was removed** from `utils.c` and `utils.h`. It was dead once the
scanner replaced the trim-as-you-go parsing, and it was exported from the
shared library. `qic.h` documents itself as the only header a caller needs, and
the only known consumer uses only `qic_parse`, so this is judged safe — but it
is the one deletion here that is visible outside the library.

**`temp/draft.c` was deleted.** An earlier draft of `parse_single_sided`,
tracked in git and superseded years ago. It is in the history if anyone wants
it.

**The UI was not touched.** The CLI's output is unchanged for every input that
worked before, which the CLI checks confirm: run them against the original
binary and eight of the ten pass. The two that fail are the cubic and the
two-sided equation, both of which are the bug fixes.

---

## For whoever is next

**`../QIC - Frontend` needs a small follow-up and is a separate project, so
this pass did not touch it.** Two things:

- `qic_api.py`'s `_PARSE_ERRORS` table and its docstring map error codes 0–3.
  Codes 4, 5 and 6 now exist. `app.py` reads the table with
  `.get(result.error, <generic message>)`, so nothing breaks — an unknown code
  falls through to "Parse error — check the equation and retry." But a user who
  types `0x^2+3x+1` or `x^3+1` now gets the generic message where a specific
  one is available. Three table entries.
- It searched for `libqic.dll` in `../QIC/`, relying on the binary being
  committed. It still finds the one on disk, but a fresh clone will not have
  one until `make` is run. Its README should say so.

**The Python GUI has not been run.** It needs Python with tkinter and is a
different project; the library it links against is covered by 368 checks here,
but nobody has confirmed the two still talk to each other since the enum grew.
Loading `qic_api.py` and parsing one equation would settle it in two minutes.

**The library builds clean under `-Wall -Wextra`** with TDM-GCC 10.3, with no
warnings. It has not been built on Linux; nothing in the changes is
platform-specific, and the Makefile's Linux branch is untouched, but `libqic.so`
is now untracked and no one here can produce one.

---

---

# The original review, as written on 2026-09-04

Reviewed 2026-09-04 against the working tree. Scope: `src/parser.c`, `src/solver.c`, `src/qic.c`,
`src/normalize.c`, `src/utils.c`, `src/main.c`, `include/`, `Makefile`, `README.md`.

## What this project is

A small C shared library that parses a quadratic equation from a string in various formats, reduces
it to standard form, extracts integer coefficients, and optionally solves for the roots. Built as
`libqic.so` / `libqic.dll` so other processes can link against it — the companion Python GUI does
exactly that through ctypes.

## Verdict

The library boundary is a good design choice and the API surface (`qic_parse`, `qic_solve`, one
public header) is appropriately small. The parser handles both single-sided and two-sided equations
by parsing each side and subtracting, which is the right decomposition.

But the solver has three defects that produce wrong or undefined answers for inputs a user can
plausibly type, and the repository has five compiled binaries committed to it.

## Critical issues

**1. Division by zero when the quadratic term cancels.** `solve()` computes
`-b / (2.0*a)` with no check that `a != 0`. That is reachable: `parse_both_sided` sets
`result->a = left.a - right.a`, so an input like `x^2 + 3x = x^2 + 5` parses to `a = 0` and the solver
divides by zero, returning infinity or NaN as the "roots" with `error` still set to `NONE`.

The parser's `INVALID_DEGREE` check only looks for the presence of `^` in the raw string, so it never
catches this. Add a post-normalisation check: if `a == 0` after the sides are combined, set an error
(a linear or degenerate equation is not a quadratic) and let the caller decide what to do.

**2. The discriminant is computed in integer arithmetic and overflows.**
`double discriminant = b*b - 4*a*c;` — `b`, `a` and `c` are all `int`, so the entire right-hand side
is evaluated as `int` *before* being converted to `double`. With `b = 100000` that is `10^10`, well
past the range of a 32-bit `int`: signed overflow, which is undefined behaviour, and in practice a
wrong discriminant and wrong roots with no error reported. Cast first:
`(double)b * b - 4.0 * a * c`.

**3. Catastrophic cancellation in the root formula.** When `b*b` greatly exceeds `4ac`,
`sqrt(discriminant)` is very close to `|b|`, so one of the two roots is computed as the difference of
two nearly equal numbers and loses most of its significant digits. For a library whose job is
correct roots, use the numerically stable form: compute
`q = -(b + copysign(sqrt(D), b)) / 2`, then `x1 = q/a` and `x2 = c/q`. This is a well-known fix and
costs three lines.

## Other bugs

- **Unchecked allocations.** `parse_input` calls `malloc` twice (`:26`, `:34`) and `strdup` once
  (`:43`), and `parse_both_sided` calls `strdup` twice more — none are checked. A NULL from any of
  them is dereferenced by the following `strncpy`.
- **`print_complex` has two identical branches.** The `n.imag > 0` case and the `else` case contain
  exactly the same `printf`. One of them is dead; presumably the intent was a `+` sign for the
  positive case. Fix or collapse it.
- **The input validation heuristic is opaque.** `count_char('x', expression) < 1 || > 4` and
  `count_char('^') != 1` reject by counting characters rather than by parsing structure, so
  `x^2 + x + x + x + x = 0` is rejected as malformed even though it is a valid quadratic, and other
  malformed inputs slip through to produce nonsense coefficients. The magic number 4 has no comment
  explaining it.
- **`INT_BUFFER_SIZE 12` is defined in `parser.c`** and sized for a 32-bit integer's decimal digits
  plus sign and terminator. Correct, but it deserves the one-line comment saying so, and a coefficient
  longer than that is silently truncated rather than reported as an error.

## Cleanup

**Five compiled binaries are committed to git**, confirmed with `git ls-files`: `libqic.dll`,
`libqic.dll.a`, `libqic.so`, `main` and `main.exe`. The working tree additionally holds six `.o`
files. There is no `.gitignore`.

For a library that builds in under a second from a `Makefile`, none of that belongs in version
control: it bloats history, the `.so` and `.dll` will drift out of step with the source, and a Linux
user gets a Windows DLL they cannot use. Add a `.gitignore` covering `*.o`, `*.so`, `*.dll`, `*.a`,
`main`, `main.exe`, then `git rm --cached` the five tracked files.

Worth noting the knock-on effect: the Python frontend's `qic_api.py` searches for the shared library
next to itself and in sibling directories, so it has been relying on those committed binaries being
present. Once you untrack them, document the build step in the frontend's README.

No `LICENSE`. No debug output, no secrets, no dead code beyond the duplicated `print_complex` branch.

## Action checklist

- [x] **P1** Reject or flag `a == 0` after combining both sides, before dividing.
- [x] **P1** Cast the discriminant to `double` before multiplying; fix the overflow.
- [x] **P2** Use the numerically stable root formula.
- [x] **P2** Add a `.gitignore` and untrack the five committed binaries. *(Eleven, not five.)*
- [x] **P3** Check every `malloc` and `strdup`; fix the duplicated `print_complex` branch.
- [x] **P3** Replace the character-count validation with structural parsing; add a `LICENSE`.

## Notes for a future session

Build with `make`. There are no tests, and this library is the ideal candidate for them: `qic_parse`
is a pure string-to-struct function and `solve` is pure arithmetic. Write cases for each defect above
before fixing it — `x^2 + 3x = x^2 + 5` for the zero-`a` crash, a large `b` for the overflow, and
`x^2 + 1e8·x + 1` for the cancellation. Remember the Python GUI links against this library, so any
change to the struct layout in `qic.h` breaks `qic_api.py`'s ctypes definitions.

## Licence, 2026-10-02

The author chose one licence for all of their projects: free for personal,
non-commercial use, modifiable for the user's own use, and not to be
distributed, original or modified. No standard licence says that, so `LICENSE`
is a short custom text, the same in every project apart from the year and what
it excludes. The README's licence section says the same. Anything above that
calls the licence an open question, or calls the project MIT, is superseded.

## CI, 2026-10-02

`.github/workflows/ci.yml` runs `make test` on `ubuntu-latest` and
`mingw32-make test` on `windows-latest`. Nothing has been pushed, so it has
not run on GitHub yet. Every job was run before committing, from a fresh clone
of this branch, the way a runner would run it: Linux jobs in a throwaway
Ubuntu 24.04 (the image `ubuntu-latest` uses), Windows jobs here, each with
only the tools the job sets up. 368 on each.

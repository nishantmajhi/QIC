/*
 * test.c — QIC library test suite.
 *
 * An ordinary consumer of the library: it includes only qic.h and links
 * against libqic, exactly as an external caller would.
 *
 * Sections marked "regression" cover a defect that was present in the
 * original code. Each was written before its fix and confirmed to fail
 * against the unfixed library; see review.md for how that run was set up.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "qic.h"

#ifdef _WIN32
#  define POPEN    _popen
#  define PCLOSE   _pclose
#  define CLI_FILE "main.exe"
/* Explicitly relative: cmd.exe does not always resolve a bare name against
   the working directory, and this suite runs from the project root. */
#  define CLI_PATH ".\\main.exe"
#else
#  define POPEN    popen
#  define PCLOSE   pclose
#  define CLI_FILE "main"
#  define CLI_PATH "./main"
#endif

static int passed = 0, failed = 0;

#define ASSERT_EQ(label, expected, actual) do {             \
    if ((expected) == (actual)) {                           \
        printf("  [PASS] %s\n", label);                     \
        passed++;                                           \
    } else {                                                \
        printf("  [FAIL] %s  expected=%d  got=%d\n",        \
               label, (int)(expected), (int)(actual));      \
        failed++;                                           \
    }                                                       \
} while (0)

/* Absolute tolerance — for roots of order 1. */
#define ASSERT_NEAR(label, expected, actual, tol) do {              \
    double _e = (expected), _a = (actual);                          \
    if (fabs(_e - _a) <= (tol)) {                                   \
        printf("  [PASS] %s\n", label);                             \
        passed++;                                                   \
    } else {                                                        \
        printf("  [FAIL] %s  expected=%.17g  got=%.17g\n",          \
               label, _e, _a);                                      \
        failed++;                                                   \
    }                                                               \
} while (0)

/*
 * Relative tolerance — the only honest way to assert on a root whose
 * magnitude is 1e-10 sitting next to one whose magnitude is 1e9. An
 * absolute tolerance would happily accept the naive formula's answer of
 * exactly zero, which is the very bug being tested for.
 */
#define ASSERT_REL(label, expected, actual, rel) do {               \
    double _e = (expected), _a = (actual);                          \
    double _d = fabs(_e - _a);                                      \
    double _s = fabs(_e) > 0.0 ? fabs(_e) : 1.0;                    \
    if (_d / _s <= (rel)) {                                         \
        printf("  [PASS] %s\n", label);                             \
        passed++;                                                   \
    } else {                                                        \
        printf("  [FAIL] %s  expected=%.17g  got=%.17g"             \
               "  (relative error %.3g)\n",                         \
               label, _e, _a, _d / _s);                             \
        failed++;                                                   \
    }                                                               \
} while (0)

#define ASSERT_TRUE(label, cond) do {                       \
    if (cond) {                                             \
        printf("  [PASS] %s\n", label);                     \
        passed++;                                           \
    } else {                                                \
        printf("  [FAIL] %s\n", label);                     \
        failed++;                                           \
    }                                                       \
} while (0)

static void check(const char *expr, int ea, int eb, int ec, ParseError eerr) {
    QuadraticExpression r = qic_parse(expr);
    printf("qic_parse(\"%s\")\n", expr);
    if (eerr != NONE) {
        ASSERT_EQ("error", eerr, r.error);
        return;
    }
    ASSERT_EQ("a", ea, r.a);
    ASSERT_EQ("b", eb, r.b);
    ASSERT_EQ("c", ec, r.c);
    ASSERT_EQ("no error", NONE, r.error);
}

/* Solve through the public one-shot entry point and assert on both roots. */
static void check_roots(const char *expr,
                        double r1re, double r1im,
                        double r2re, double r2im, double tol) {
    QuadraticSolution s = qic_solve(expr);
    printf("qic_solve(\"%s\")\n", expr);
    ASSERT_NEAR("root1.real", r1re, s.root1.real, tol);
    ASSERT_NEAR("root1.imag", r1im, s.root1.imag, tol);
    ASSERT_NEAR("root2.real", r2re, s.root2.real, tol);
    ASSERT_NEAR("root2.imag", r2im, s.root2.imag, tol);
}

/* ===================================================================== */
/* Parsing                                                               */
/* ===================================================================== */

static void test_basic_single_digit(void) {
    printf("\n=== Basic single-digit coefficients ===\n");
    check("x^2+2x+1",    1,  2,  1, NONE);
    check("x^2-3x+2",    1, -3,  2, NONE);
    check("x^2+x+1",     1,  1,  1, NONE);
    check("x^2-x-1",     1, -1, -1, NONE);
    check("2x^2+3x+4",   2,  3,  4, NONE);
}

static void test_multi_digit(void) {
    printf("\n=== Multi-digit coefficients ===\n");
    check("10x^2+20x+30",    10,  20,  30, NONE);
    check("100x^2+200x+300", 100, 200, 300, NONE);
    check("3x^2+15x+2",       3,  15,   2, NONE);
    check("2x^2-10x+50",      2, -10,  50, NONE);
    check("5x^2-3x-7",        5,  -3,  -7, NONE);
}

static void test_missing_terms(void) {
    printf("\n=== Missing b or c term ===\n");
    check("x^2+5",    1,  0,   5, NONE);
    check("x^2-9",    1,  0,  -9, NONE);
    check("x^2+3x",   1,  3,   0, NONE);
    check("5x^2+10",  5,  0,  10, NONE);
    check("10x^2-20", 10, 0, -20, NONE);
    check("x^2",      1,  0,   0, NONE);
}

static void test_both_sided(void) {
    printf("\n=== Both-sided equations (LHS = RHS) ===\n");
    check("x^2+2x+1=0",    1,  2,  1, NONE);
    check("2x^2+3x=x+5",   2,  2, -5, NONE);
    check("x^2+1=",        1,  0,  1, NONE);   /* an empty side is zero */
    check("=x^2+1",       -1,  0, -1, NONE);
}

static void test_random_order(void) {
    printf("\n=== Random term order ===\n");
    check("1+x^2+2x",     1,  2,  1, NONE);
    check("4+2x^2+3x",    2,  3,  4, NONE);
    check("3x+x^2+2",     1,  3,  2, NONE);
    check("-3x+x^2+2",    1, -3,  2, NONE);
    check("2x+2x^2-5",    2,  2, -5, NONE);
    check("5x+3x^2+7",    3,  5,  7, NONE);
    check("-1+x^2+x",     1,  1, -1, NONE);
    check("-7+2x^2-3x",   2, -3, -7, NONE);
    check("3x+x^2=2",     1,  3, -2, NONE);
    check("5x+2x^2=-7",   2,  5,  7, NONE);
}

static void test_explicit_zero_coefficients(void) {
    printf("\n=== Explicit zero coefficients ===\n");
    check("2x^2+0x+5",    2,  0,  5, NONE);
    check("x^2+0x-4",     1,  0, -4, NONE);
    check("3x^2+2x+0",    3,  2,  0, NONE);
    check("x^2-x+0",      1, -1,  0, NONE);
    check("x^2+3x+5=5",   1,  3,  0, NONE);
    check("2x^2+x+3=3",   2,  1,  0, NONE);
}

static void test_sign_folding(void) {
    printf("\n=== Repeated and leading signs ===\n");
    check("x^2--3x+2",    1,  3,  2, NONE);   /* '--' folds to '+' */
    check("x^2+-3x+2",    1, -3,  2, NONE);
    check("x^2---3x+2",   1, -3,  2, NONE);
    check("+x^2+2x+1",    1,  2,  1, NONE);
    check("-x^2+2x+1",   -1,  2,  1, NONE);
    check("-x^2-x-1",    -1, -1, -1, NONE);
}

static void test_whitespace_and_case(void) {
    printf("\n=== Whitespace and case normalisation ===\n");
    check("  X^2 + 3X + 2  ", 1, 3, 2, NONE);
    check("X^2-3X+2",         1,-3, 2, NONE);
    check("\tx^2\n+\t3x+2",   1, 3, 2, NONE);
}

static void test_errors(void) {
    printf("\n=== Error handling ===\n");
    check("x+1=0",         0, 0, 0, INVALID_DEGREE);   /* no '^' anywhere */
    check("",              0, 0, 0, INVALID_DEGREE);
    check("2.5x^2+3x+1",   0, 0, 0, INVALID_INPUT);
    check("ax^2+bx+c=0",   0, 0, 0, INVALID_INPUT);
    check("x^2=1=2",       0, 0, 0, INVALID_EQUATION); /* two '=' signs */
    check("^2+1",          0, 0, 0, INVALID_EQUATION); /* term starts with '^' */
    check("x^2+",          0, 0, 0, INVALID_EQUATION); /* trailing sign */
    check("x^+1",          0, 0, 0, INVALID_EQUATION); /* '^' with no exponent */
}

/* ===================================================================== */
/* Regressions — every one of these failed against the original code     */
/* ===================================================================== */

static void test_regression_not_quadratic(void) {
    printf("\n=== Regression: a == 0 is not a quadratic ===\n");
    /*
     * The original returned error=NONE with a=0, so a caller that trusted
     * .error went straight on to divide by 2a.
     */
    check("0x^2+3x+1",    0, 0, 0, NOT_QUADRATIC);
    check("0x^2-5x+2",    0, 0, 0, NOT_QUADRATIC);
    check("x^2=x^2+5",    0, 0, 0, NOT_QUADRATIC);
    check("x^2+3x=x^2+5", 0, 0, 0, NOT_QUADRATIC);
    check("2x^2=2x^2",    0, 0, 0, NOT_QUADRATIC);

    /*
     * solve() is exported and declared in a public header, so it has to
     * defend itself: a=0 must not divide by zero and must not hand back an
     * infinity dressed up as a root.
     */
    QuadraticExpression degenerate = { .a = 0, .b = 3, .c = 1, .error = NONE };
    QuadraticSolution s = solve(degenerate);
    printf("solve({a=0,b=3,c=1})\n");
    ASSERT_TRUE("root1.real is NaN, not +/-inf", isnan(s.root1.real));
    ASSERT_TRUE("root2.real is NaN, not +/-inf", isnan(s.root2.real));

    QuadraticSolution t = qic_solve("0x^2+3x+1");
    printf("qic_solve(\"0x^2+3x+1\")\n");
    ASSERT_TRUE("root1.real is NaN", isnan(t.root1.real));
    ASSERT_TRUE("root2.real is NaN", isnan(t.root2.real));
}

static void test_regression_exponent_must_be_two(void) {
    printf("\n=== Regression: the exponent was never checked ===\n");
    /*
     * The original only looked for the presence of '^' and then consumed a
     * fixed three characters, so a cubic parsed as a quadratic and nobody
     * was told.
     */
    check("x^3+2x+1",    0, 0, 0, INVALID_DEGREE);
    check("x^9+1",       0, 0, 0, INVALID_DEGREE);
    check("2x^0+1",      0, 0, 0, INVALID_DEGREE);
    check("x^1+1",       0, 0, 0, INVALID_DEGREE);
    check("x^10+1",      0, 0, 0, INVALID_DEGREE);
    check("3x^4-2x+7",   0, 0, 0, INVALID_DEGREE);
    check("x^2=x^3",     0, 0, 0, INVALID_DEGREE); /* checked on both sides */
    check("x^02+1",      1, 0, 1, NONE);           /* a padded 2 is still 2 */
}

static void test_regression_like_terms_accumulate(void) {
    printf("\n=== Regression: like terms must accumulate, not overwrite ===\n");
    /*
     * The original assigned (result->b = num) instead of accumulating, so
     * the last occurrence of a term silently discarded every earlier one.
     */
    check("x^2+x+x",        1,  2,  0, NONE);
    check("x^2+2x+3x+1",    1,  5,  1, NONE);
    check("x^2+x^2+1",      2,  0,  1, NONE);
    check("x^2+1+1",        1,  0,  2, NONE);
    check("x^2+5x-3x+2",    1,  2,  2, NONE);
    check("x^2+2x+1=x+x",   1,  0,  1, NONE);
}

static void test_regression_more_than_three_terms(void) {
    printf("\n=== Regression: terms beyond the third were dropped ===\n");
    /*
     * The original loop ran exactly three times, so a fourth term — which
     * a constant written before the x terms easily produces — was ignored.
     */
    check("1+2+x^2+3x",       1,  3,  3, NONE);
    check("x^2+x+x+x+x",      1,  4,  0, NONE);
    check("x^2+x+x+x+x=0",    1,  4,  0, NONE);
    check("1+1+1+1+x^2",      1,  0,  4, NONE);
    check("x^2+2x+3+4x+5",    1,  6,  8, NONE);
}

static void test_regression_two_sided_quadratic(void) {
    printf("\n=== Regression: x^2 on both sides was rejected ===\n");
    /*
     * The original required exactly one '^' in the whole string, so every
     * equation carrying a quadratic term on each side was reported
     * malformed even when it reduced to a perfectly ordinary quadratic.
     */
    check("2x^2+3x=x^2+5",   1,  3, -5, NONE);
    check("3x^2=x^2+4",      2,  0, -4, NONE);
    check("5x^2+2x=2x^2-x",  3,  3,  0, NONE);
    check("x^2+1=-x^2-1",    2,  0,  2, NONE);
}

static void test_regression_coefficient_range(void) {
    printf("\n=== Regression: out-of-range coefficients were silently wrong ===\n");
    /*
     * The original used atoi, whose behaviour on overflow is undefined. In
     * practice "2147483648x^2+x+1" came back as c = -2147483648 with the
     * error field still set to NONE, and "99999999999x^2+1" as a = 9.
     */
    check("2147483648x^2+x+1",  0, 0, 0, COEFFICIENT_RANGE);
    check("99999999999x^2+1",   0, 0, 0, COEFFICIENT_RANGE);
    check("x^2+9999999999x+1",  0, 0, 0, COEFFICIENT_RANGE);
    check("x^2+x+99999999999",  0, 0, 0, COEFFICIENT_RANGE);
    /* combining two in-range sides can still leave the range */
    check("2147483647x^2=-2x^2", 0, 0, 0, COEFFICIENT_RANGE);
    /* the boundaries themselves must still parse */
    check("2147483647x^2+1",    INT_MAX, 0, 1, NONE);
    check("x^2-2147483648",     1, 0, INT_MIN, NONE);
}

static void test_regression_leading_zeros(void) {
    printf("\n=== Regression: leading zeros ===\n");
    check("07x^2+1",     7, 0,  1, NONE);
    check("007x^2+08",   7, 0,  8, NONE);
    check("x^2+007x+1",  1, 7,  1, NONE);
}

/* ===================================================================== */
/* Solving                                                               */
/* ===================================================================== */

static void test_solve_real_roots(void) {
    printf("\n=== Real roots ===\n");
    /* root1 is the (-b + sqrt(D)) root, root2 the (-b - sqrt(D)) one */
    check_roots("2x^2+3x-5=0",  1.0, 0.0, -2.5, 0.0, 1e-12);
    check_roots("x^2-3x+2",     2.0, 0.0,  1.0, 0.0, 1e-12);
    check_roots("x^2-5x+6",     3.0, 0.0,  2.0, 0.0, 1e-12);
    check_roots("-x^2+3x-2",    1.0, 0.0,  2.0, 0.0, 1e-12);
    check_roots("x^2-4",        2.0, 0.0, -2.0, 0.0, 1e-12);
}

static void test_solve_repeated_root(void) {
    printf("\n=== Repeated root (discriminant zero) ===\n");
    check_roots("x^2+2x+1",  -1.0, 0.0, -1.0, 0.0, 1e-12);
    check_roots("x^2-2x+1",   1.0, 0.0,  1.0, 0.0, 1e-12);
    check_roots("4x^2+4x+1", -0.5, 0.0, -0.5, 0.0, 1e-12);
    check_roots("x^2",        0.0, 0.0,  0.0, 0.0, 1e-12);
}

static void test_solve_complex_roots(void) {
    printf("\n=== Complex roots (discriminant negative) ===\n");
    check_roots("x^2+1",     0.0,  1.0,  0.0, -1.0, 1e-12);
    check_roots("x^2+2x+5", -1.0,  2.0, -1.0, -2.0, 1e-12);
    check_roots("x^2-2x+10", 1.0,  3.0,  1.0, -3.0, 1e-12);
}

static void test_solve_parse_failure_is_nan(void) {
    printf("\n=== qic_solve on input it cannot use ===\n");
    QuadraticSolution s = qic_solve("2.5x^2+1");
    ASSERT_TRUE("unparseable root1.real is NaN", isnan(s.root1.real));
    ASSERT_TRUE("unparseable root2.real is NaN", isnan(s.root2.real));
    QuadraticSolution t = qic_solve("x^3+1");
    ASSERT_TRUE("cubic root1.real is NaN", isnan(t.root1.real));
    ASSERT_TRUE("cubic root2.real is NaN", isnan(t.root2.real));
}

static void test_regression_discriminant_overflow(void) {
    printf("\n=== Regression: the discriminant overflowed in int arithmetic ===\n");
    /*
     * b*b - 4*a*c was evaluated entirely in int before being converted to
     * double. With b = 100000 that is 1e10, well past INT_MAX, so it
     * wrapped and both roots came back wrong with no error reported: the
     * original answered -31224.58 and -68775.42 for this equation.
     *
     * root2 is asserted tightly because it is unaffected by the separate
     * cancellation defect below; root1 only loosely, so that this test
     * fails for the overflow and not for something else.
     */
    QuadraticSolution s = qic_solve("x^2+100000x+1");
    printf("qic_solve(\"x^2+100000x+1\")\n");
    ASSERT_REL("root2 (large root)", -99999.99999, s.root2.real, 1e-12);
    ASSERT_REL("root1 (small root)", -1.0000000001e-05, s.root1.real, 1e-4);

    /*
     * Here 4*a*c alone overflows: it wrapped to a positive number, which
     * flipped the sign of the discriminant, so the original reported two
     * complex roots for an equation with two real ones.
     */
    QuadraticSolution t = qic_solve("x^2+x-1000000000");
    printf("qic_solve(\"x^2+x-1000000000\")\n");
    ASSERT_NEAR("root1.imag is zero — the roots are real", 0.0, t.root1.imag, 0.0);
    ASSERT_REL("root1",  31622.276601683792, t.root1.real, 1e-9);
    ASSERT_REL("root2", -31623.276601683792, t.root2.real, 1e-9);
}

static void test_regression_catastrophic_cancellation(void) {
    printf("\n=== Regression: catastrophic cancellation in the root formula ===\n");
    /*
     * With b = 2e9, sqrt(D) rounds to exactly |b| in double precision, so
     * the naive (-b + sqrt(D)) / 2a evaluates to exactly 0 — which is not
     * a root of this equation at all. The stable form computes the small
     * root as c/q and keeps every digit.
     */
    QuadraticSolution s = qic_solve("x^2+2000000000x+1");
    printf("qic_solve(\"x^2+2000000000x+1\")\n");
    ASSERT_TRUE("root1 is not exactly zero", s.root1.real != 0.0);
    ASSERT_REL("root1 (small root)", -5.0e-10, s.root1.real, 1e-9);
    ASSERT_REL("root2 (large root)", -2.0e9,   s.root2.real, 1e-12);

    /* Substituting a root back in must give zero to near machine precision. */
    double res1 = (1.0 * s.root1.real + 2000000000.0) * s.root1.real + 1.0;
    ASSERT_NEAR("root1 substituted back into the equation", 0.0, res1, 1e-6);

    /* A milder case, where the naive form loses about six digits. */
    QuadraticSolution t = qic_solve("x^2+200000x+1");
    printf("qic_solve(\"x^2+200000x+1\")\n");
    ASSERT_REL("root1 (small root)", -5.000000000125e-06, t.root1.real, 1e-12);
    ASSERT_REL("root2 (large root)", -199999.999995,      t.root2.real, 1e-12);
}

static void test_null_input(void) {
    printf("\n=== NULL input is rejected, not dereferenced ===\n");
    QuadraticExpression r = qic_parse(NULL);
    ASSERT_EQ("qic_parse(NULL) reports INVALID_INPUT", INVALID_INPUT, r.error);
    QuadraticSolution s = qic_solve(NULL);
    ASSERT_TRUE("qic_solve(NULL) root1 is NaN", isnan(s.root1.real));
}

/* ===================================================================== */
/* CLI — main is itself a consumer of the library                        */
/* ===================================================================== */

static int cli_available = 0;

static void cli_check(const char *label, const char *args,
                      const char *expect_substr) {
    char cmd[512];
    char out[2048];
    size_t used = 0;
    FILE *p;

    out[0] = '\0';
    snprintf(cmd, sizeof(cmd), "%s %s 2>&1", CLI_PATH, args);
    p = POPEN(cmd, "r");
    if (p == NULL) {
        printf("  [FAIL] %s  (could not run %s)\n", label, CLI_PATH);
        failed++;
        return;
    }
    while (used + 1 < sizeof(out) &&
           fgets(out + used, (int)(sizeof(out) - used), p) != NULL) {
        used = strlen(out);
    }
    PCLOSE(p);

    if (strstr(out, expect_substr) != NULL) {
        printf("  [PASS] %s\n", label);
        passed++;
    } else {
        printf("  [FAIL] %s\n         expected to contain: %s\n         got: %s\n",
               label, expect_substr, out);
        failed++;
    }
}

static void test_cli(void) {
    printf("\n=== CLI ===\n");
    if (!cli_available) {
        printf("  [SKIP] %s is not built — run 'make' to include these checks\n",
               CLI_FILE);
        return;
    }
    cli_check("standard form",          "\"2x^2+3x-5=0\"",    "2x^2+3x-5=0");
    cli_check("-s before the equation", "-s \"2x^2+3x-5=0\"", "x1 = 1");
    cli_check("-s after the equation",  "\"2x^2+3x-5=0\" -s", "x1 = 1");
    cli_check("complex roots",          "-s \"x^2+1\"",       "x1 = (0, 1i)");
    cli_check("two-sided equation",     "\"2x^2+3x=x^2+5\"",  "1x^2+3x-5=0");
    cli_check("unparseable input",      "\"2.5x^2+1\"",       "Parse error");
    cli_check("a cubic is refused",     "\"x^3+1\"",          "only quadratic");
    cli_check("a == 0 still prints the standard form",
              "\"0x^2+3x+1\"", "0x^2+3x+1=0");
    cli_check("a == 0 refuses to solve",
              "-s \"0x^2+3x+1\"", "not a quadratic");
    cli_check("too many parameters",    "a b c",              "Invalid parameters");
}

int main(void) {
    /*
     * Unbuffered, because running this suite against unfixed code is
     * exactly when the process dies partway through — and which check it
     * reached is the interesting part.
     */
    setvbuf(stdout, NULL, _IONBF, 0);

    FILE *f = fopen(CLI_FILE, "rb");
    if (f != NULL) { cli_available = 1; fclose(f); }

    test_basic_single_digit();
    test_multi_digit();
    test_missing_terms();
    test_both_sided();
    test_random_order();
    test_explicit_zero_coefficients();
    test_sign_folding();
    test_whitespace_and_case();
    test_errors();

    test_regression_not_quadratic();
    test_regression_exponent_must_be_two();
    test_regression_like_terms_accumulate();
    test_regression_more_than_three_terms();
    test_regression_two_sided_quadratic();
    test_regression_coefficient_range();
    test_regression_leading_zeros();

    test_solve_real_roots();
    test_solve_repeated_root();
    test_solve_complex_roots();
    test_solve_parse_failure_is_nan();
    test_regression_discriminant_overflow();
    test_regression_catastrophic_cancellation();
    test_null_input();

    test_cli();

    printf("\n--------------------------------\n");
    printf("Results: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}

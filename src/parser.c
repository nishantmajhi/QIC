#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "utils.h"

/*
 * Terms are accumulated in long long and range-checked once at the end, so
 * that an intermediate sum which leaves int range is reported rather than
 * silently wrapping.
 */
static int in_int_range(long long v) {
    return v >= INT_MIN && v <= INT_MAX;
}

/*
 * Parse one side of an equation and accumulate its terms into *result.
 *
 * `expression` is already normalised — lowercase, no whitespace — and
 * contains no '='. An empty side is legal and contributes nothing, which
 * is what makes "x^2+1=" and "=x^2+1" work.
 *
 * The grammar for a term is:  [+-]* digits? ( 'x' ( '^' digits )? )?
 * and the whole side has to be consumed by a whole number of them. Parsing
 * structurally rather than counting characters is what lets an equation
 * carry any number of terms, repeat a term, or put a quadratic term on
 * both sides.
 */
void parse_single_sided(const char *expression, QuadraticExpression *result) {
    long long a = 0, b = 0, c = 0;
    int i = 0;

    while (expression[i] != '\0') {
        int sign = 1;
        long long value = 0;
        int have_digits = 0;

        while (expression[i] == '+' || expression[i] == '-') {
            if (expression[i] == '-') sign = -sign;
            i++;
        }

        while (isdigit((unsigned char)expression[i])) {
            value = value * 10 + (expression[i] - '0');
            /*
             * Bail as soon as the magnitude passes INT_MAX+1 — the +1 so
             * that INT_MIN is still reachable as a negative coefficient.
             * Stopping here also keeps `value * 10` far from overflowing
             * long long on the next digit.
             */
            if (value > (long long)INT_MAX + 1) {
                result->error = COEFFICIENT_RANGE;
                return;
            }
            have_digits = 1;
            i++;
        }

        if (expression[i] == 'x') {
            int degree = 1;
            i++;

            if (expression[i] == '^') {
                long exponent = 0;
                i++;
                if (!isdigit((unsigned char)expression[i])) {
                    result->error = INVALID_EQUATION;
                    return;
                }
                while (isdigit((unsigned char)expression[i])) {
                    /* Clamped: once past 100 it can never equal 2 again,
                       so there is nothing to gain by overflowing. */
                    if (exponent < 100) exponent = exponent * 10 + (expression[i] - '0');
                    i++;
                }
                if (exponent != 2) {
                    result->error = INVALID_DEGREE;
                    return;
                }
                degree = 2;
            }

            /* A bare 'x' means a coefficient of 1, not of 0. */
            if (degree == 2) a += (have_digits ? value : 1) * sign;
            else             b += (have_digits ? value : 1) * sign;
        } else if (have_digits) {
            c += value * sign;
        } else {
            /* A sign with nothing after it, or a character no term can
               begin with — '^' on its own, for instance. */
            result->error = INVALID_EQUATION;
            return;
        }
    }

    if (!in_int_range(a) || !in_int_range(b) || !in_int_range(c)) {
        result->error = COEFFICIENT_RANGE;
        return;
    }

    result->a = (int)a;
    result->b = (int)b;
    result->c = (int)c;
}

void parse_both_sided(const char *LHS, const char *RHS, QuadraticExpression *result) {
    QuadraticExpression left  = { .a = 0, .b = 0, .c = 0, .error = NONE };
    QuadraticExpression right = { .a = 0, .b = 0, .c = 0, .error = NONE };
    long long a, b, c;

    parse_single_sided(LHS, &left);
    if (left.error != NONE) {
        result->error = left.error;
        return;
    }

    parse_single_sided(RHS, &right);
    if (right.error != NONE) {
        result->error = right.error;
        return;
    }

    /* Two in-range sides can still subtract to something out of range. */
    a = (long long)left.a - right.a;
    b = (long long)left.b - right.b;
    c = (long long)left.c - right.c;

    if (!in_int_range(a) || !in_int_range(b) || !in_int_range(c)) {
        result->error = COEFFICIENT_RANGE;
        return;
    }

    result->a = (int)a;
    result->b = (int)b;
    result->c = (int)c;
}

QuadraticExpression parse_input(const char *expression) {
    QuadraticExpression result = { .a = 0, .b = 0, .c = 0, .error = NONE };
    size_t expression_length, equals_index;

    /*
     * A quadratic has to have an x^2 term somewhere. The exponent itself
     * is checked per term by the scanner; this only rejects input with no
     * exponent at all, which is where "x+1=0" is caught.
     */
    if (strchr(expression, '^') == NULL) {
        result.error = INVALID_DEGREE;
        return result;
    }
    if (count_char('=', expression) > 1) {
        result.error = INVALID_EQUATION;
        return result;
    }

    expression_length = strlen(expression);
    equals_index = strcspn(expression, "=");

    if (equals_index < expression_length) {
        /*
         * Only the left side needs copying — the right side is a suffix of
         * the same buffer, and the scanner no longer mutates what it reads.
         */
        char *LHS = malloc(equals_index + 1);
        if (LHS == NULL) {
            result.error = ALLOC_FAILED;
            return result;
        }
        memcpy(LHS, expression, equals_index);
        LHS[equals_index] = '\0';

        parse_both_sided(LHS, expression + equals_index + 1, &result);
        free(LHS);
    } else {
        parse_single_sided(expression, &result);
    }

    /*
     * Reported last, so that a genuinely malformed input keeps its more
     * specific error. An equation whose quadratic terms cancel parses
     * perfectly well; it just is not a quadratic, and saying so here is
     * what stops a caller dividing by 2a == 0.
     */
    if (result.error == NONE && result.a == 0) {
        result.error = NOT_QUADRATIC;
    }

    return result;
}

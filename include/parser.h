#ifndef PARSER_H
#define PARSER_H

/*
 * Values are appended, never reordered: the Python frontend maps these
 * numbers by value through ctypes, so 0..3 have to keep meaning what they
 * always meant.
 */
typedef enum {
    NONE,
    INVALID_INPUT,        /* a character outside [0-9 x ^ = + -] */
    INVALID_DEGREE,       /* no x^2 term, or an exponent that is not 2 */
    INVALID_EQUATION,     /* structurally malformed */
    NOT_QUADRATIC,        /* parsed cleanly, but a == 0 */
    COEFFICIENT_RANGE,    /* a coefficient does not fit in an int */
    ALLOC_FAILED          /* out of memory */
} ParseError;

typedef struct {
    int a;
    int b;
    int c;
    ParseError error;
} QuadraticExpression;

QuadraticExpression parse_input(const char *expression);
void parse_both_sided(const char *LHS, const char *RHS, QuadraticExpression *result);
void parse_single_sided(const char *expression, QuadraticExpression *result);

#endif

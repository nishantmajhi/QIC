#include <stdlib.h>
#include <math.h>
#include "qic.h"
#include "normalize.h"
#include "parser.h"
#include "solver.h"
#include "utils.h"

QuadraticExpression qic_parse(const char *raw_input) {
    QuadraticExpression result = { .a = 0, .b = 0, .c = 0, .error = NONE };
    char *normalized;

    if (raw_input == NULL) {
        result.error = INVALID_INPUT;
        return result;
    }

    normalized = normalize(raw_input);
    if (normalized == NULL) {
        result.error = ALLOC_FAILED;
        return result;
    }

    if (!validate_input(normalized)) {
        free(normalized);
        result.error = INVALID_INPUT;
        return result;
    }

    result = parse_input(normalized);
    free(normalized);
    return result;
}

QuadraticSolution qic_solve(const char *raw_input) {
    QuadraticSolution sol;
    QuadraticExpression exp;

    sol.root1.real = sol.root2.real = NAN;
    sol.root1.imag = sol.root2.imag = 0.0;

    exp = qic_parse(raw_input);
    /* NOT_QUADRATIC now covers the a == 0 case this used to check for. */
    if (exp.error != NONE) {
        return sol;
    }

    return solve(exp);
}

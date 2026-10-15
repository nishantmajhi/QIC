#ifndef QIC_H
#define QIC_H

/*
 * qic.h — Public API for the QIC quadratic parser/solver library.
 *
 * External callers only need to include this header.
 * All normalization and parsing details are handled internally.
 *
 * Usage:
 *   QuadraticExpression exp = qic_parse("2x^2 + 3x - 5 = 0");
 *   if (exp.error == NONE) { ... }
 *
 *   QuadraticSolution sol = qic_solve("2x^2 + 3x - 5 = 0");
 */

#include "parser.h"
#include "solver.h"

/*
 * qic_parse — Normalize and parse a raw equation string.
 *
 * Handles whitespace stripping, case folding, and full parsing.
 * Returns a QuadraticExpression with .error set to NONE on success,
 * or INVALID_INPUT / INVALID_DEGREE / INVALID_EQUATION on failure.
 */
QuadraticExpression qic_parse(const char *raw_input);

/*
 * qic_solve — Normalize, parse, and solve a raw equation string.
 *
 * Convenience wrapper that calls qic_parse internally.
 * If parsing fails, root1 and root2 will both be NAN.
 * Always check the .error field on qic_parse first when you need
 * error details; use qic_solve when you just want the roots.
 */
QuadraticSolution qic_solve(const char *raw_input);

#endif

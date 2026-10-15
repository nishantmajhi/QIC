#ifndef SOLVER_H
#define SOLVER_H

#include "parser.h"

typedef struct {
    double real;
    double imag;
} ComplexNumber;

typedef struct {
    ComplexNumber root1;
    ComplexNumber root2;
} QuadraticSolution;

QuadraticSolution solve(QuadraticExpression exp);
void print_solution(QuadraticSolution sol);

#endif

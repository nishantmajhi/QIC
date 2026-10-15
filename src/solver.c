#include <math.h>
#include <stdio.h>
#include "solver.h"

/*
 * IEEE 754 distinguishes -0.0 from 0.0. Widening the coefficients to double
 * means -b is now -0.0 when b is 0, which is arithmetically identical but
 * prints as "-0". Adding positive zero maps -0.0 to 0.0 and leaves every
 * other value untouched.
 */
static double without_negative_zero(double v) {
    return v + 0.0;
}

QuadraticSolution solve(QuadraticExpression exp) {
    QuadraticSolution sol;
    /*
     * Widened before any arithmetic happens. In the original these stayed
     * int, so `b*b - 4*a*c` was evaluated entirely in int and converted to
     * double only afterwards — b = 100000 alone is enough to overflow that
     * and hand back two confidently wrong roots.
     */
    double a = exp.a, b = exp.b, c = exp.c;
    double discriminant;

    sol.root1.imag = sol.root2.imag = 0.0;

    /*
     * solve() is exported, so it cannot assume its caller checked. With
     * a == 0 there is no pair of roots to return, and dividing by 2a would
     * produce an infinity that looks like an answer.
     */
    if (exp.a == 0) {
        sol.root1.real = sol.root2.real = NAN;
        return sol;
    }

    discriminant = b * b - 4.0 * a * c;

    if (discriminant >= 0) {
        /*
         * The stable form. Taking -b - sign(b)*sqrt(D) adds two quantities
         * of the same sign, so nothing cancels; the other root then comes
         * from the product of the roots, c/a, rather than from a second
         * subtraction. The textbook formula computes one of the two roots
         * as the difference of two nearly equal numbers, and for b = 2e9
         * that difference rounds to exactly zero.
         */
        double root = sqrt(discriminant);
        double q = -0.5 * (b + (b >= 0 ? root : -root));

        if (q == 0.0) {
            /* Only when b == 0 and D == 0, which forces c == 0 too. */
            sol.root1.real = sol.root2.real = 0.0;
        } else if (b >= 0) {
            /* q carries the -sqrt(D) root; root1 is the +sqrt(D) one. */
            sol.root1.real = c / q;
            sol.root2.real = q / a;
        } else {
            sol.root1.real = q / a;
            sol.root2.real = c / q;
        }
    } else {
        sol.root1.real = sol.root2.real = -b / (2.0 * a);
        sol.root1.imag =  sqrt(-discriminant) / (2.0 * a);
        sol.root2.imag = -sqrt(-discriminant) / (2.0 * a);
    }

    sol.root1.real = without_negative_zero(sol.root1.real);
    sol.root2.real = without_negative_zero(sol.root2.real);
    sol.root1.imag = without_negative_zero(sol.root1.imag);
    sol.root2.imag = without_negative_zero(sol.root2.imag);

    return sol;
}

static void print_complex(ComplexNumber n) {
    if (n.imag == 0.0) {
        printf("%.4g", n.real);
    } else {
        printf("(%.4g, %.4gi)", n.real, n.imag);
    }
}

void print_solution(QuadraticSolution sol) {
    printf("x1 = ");
    print_complex(sol.root1);
    printf("\n");
    printf("x2 = ");
    print_complex(sol.root2);
    printf("\n");
}

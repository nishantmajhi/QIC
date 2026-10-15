#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "qic.h"
#include "solver.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    const char *raw_input;
    char *interactive_input = NULL;
    bool solve_flag = false;

    if (argc == 1) {
        interactive_input = ask_input();
        raw_input = interactive_input;
    } else if (argc == 2) {
        raw_input = argv[1];
    } else if (argc == 3) {
        if (strcmp(argv[1], "-s") == 0) {
            solve_flag = true;
            raw_input = argv[2];
        } else if (strcmp(argv[2], "-s") == 0) {
            solve_flag = true;
            raw_input = argv[1];
        } else {
            fprintf(stderr, "Invalid parameters!\n");
            printf("Usage: ./main [-s] \"ax^2+bx+c=0\"\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Invalid parameters!\n");
        printf("Usage: ./main [-s] \"ax^2+bx+c=0\"\n");
        return 1;
    }

    /* Parse via the public API — normalization and validation are handled inside */
    QuadraticExpression exp = qic_parse(raw_input);

    if (interactive_input) {
        free(interactive_input);
        interactive_input = NULL;
    }

    /*
     * NOT_QUADRATIC is not a parse failure from the CLI's point of view:
     * the coefficients came out fine and are worth printing. All it does
     * is stop the equation being solved, which is exactly what the a == 0
     * check below this used to do on its own.
     */
    if (exp.error != NONE && exp.error != NOT_QUADRATIC) {
        handleParseError(exp.error);
        return 1;
    }

    print_standard_form(exp);

    if (argc == 1 && !solve_flag) {
        printf("Print solutions? (y/n): ");
        char answer[4];
        if (fgets(answer, sizeof(answer), stdin) && (answer[0] == 'y' || answer[0] == 'Y')) {
            solve_flag = true;
        }
    }

    if (solve_flag) {
        if (exp.error == NOT_QUADRATIC) {
            fprintf(stderr, "Cannot solve: coefficient 'a' is zero (not a quadratic).\n");
            return 1;
        }
        /*
         * Call solve(exp) directly — exp is already parsed and valid.
         * Do NOT call qic_solve(raw_input): in interactive mode raw_input
         * points to freed memory at this point, which caused NAN on Windows.
         */
        QuadraticSolution sol = solve(exp);
        print_solution(sol);
    }

    return 0;
}
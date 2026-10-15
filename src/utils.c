#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "utils.h"
#include "parser.h"

char *ask_input() {
    char *input = malloc(256 * sizeof(char));
    if (input == NULL) {
        printf("Memory allocation failed!\n");
        return NULL;
    }

    printf("Enter the quadratic equation: ");
    if (fgets(input, 256, stdin) == NULL) {
        printf("Failed to read input!\n");
        free(input);
        return NULL;
    }

    input[strcspn(input, "\n")] = '\0';

    return input;
}

bool validate_input(const char *str) {
    if (str == NULL) return false;

    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        if (!(c >= '0' && c <= '9') &&
              c != 'x'  &&
              c != '^'  &&
              c != '='  &&
              c != '+'  &&
              c != '-'  &&
              c != ' '  &&
              c != '\t' &&
              c != '\n') {
            return false;
        }
    }

    return true;
}

int count_char(char c, const char *str) {
    int count = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == c) count++;
    }
    return count;
}

void print_standard_form(QuadraticExpression exp) {
    printf("The standard form of your equation is: ");
    printf("%dx^2", exp.a);
    printf(exp.b < 0 ? "%dx" : "+%dx", exp.b);
    printf(exp.c < 0 ? "%d" : "+%d", exp.c);
    printf("=0\n");
}

void handleParseError(ParseError error) {
    switch (error) {
        case INVALID_INPUT:
            fprintf(stderr, "Parse error: integer coefficients and variable 'x' are only allowed.\n");
            printf("e.g. 2x^2+3x-5=0\n");
            break;
        case INVALID_DEGREE:
            fprintf(stderr, "Parse error: only quadratic equations (x^2) are allowed!\n");
            break;
        case INVALID_EQUATION:
            fprintf(stderr, "Parse error: invalid equation!\n");
            break;
        case NOT_QUADRATIC:
            fprintf(stderr, "Parse error: coefficient 'a' is zero (not a quadratic).\n");
            break;
        case COEFFICIENT_RANGE:
            fprintf(stderr, "Parse error: a coefficient is too large to fit in an int.\n");
            break;
        case ALLOC_FAILED:
            fprintf(stderr, "Out of memory.\n");
            break;
        default:
            fprintf(stderr, "Unknown parse error.\n");
            break;
    }
}

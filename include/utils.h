#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include "parser.h"

char *ask_input();
bool validate_input(const char *str);
void print_standard_form(QuadraticExpression exp);
void handleParseError(ParseError error);
int count_char(char c, const char *str);

#endif

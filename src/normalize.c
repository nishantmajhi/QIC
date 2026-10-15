#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * Strip whitespace and fold to lowercase.
 * Returns a freshly allocated string the caller owns, or NULL if the
 * allocation fails — which the original never reported, so the following
 * write went straight through a null pointer.
 */
char *normalize(const char *input) {
    char *output;
    int j = 0;

    if (input == NULL) return NULL;

    output = malloc(strlen(input) + 1);
    if (output == NULL) return NULL;

    for (int i = 0; input[i]; i++) {
        if (input[i] != ' ' && input[i] != '\t' && input[i] != '\n') {
            output[j++] = (char)tolower((unsigned char)input[i]);
        }
    }
    output[j] = '\0';
    return output;
}

#include <stddef.h>
#include <malloc.h>

#include "missing_handler.h"
#include "err.h"

extern int *missing;
extern size_t missing_size;
extern size_t num_missing;

void addNewValueToMissing(int value) {
    if (missing_size == num_missing) {
        missing = (int *) realloc(missing, 25);

        if (missing == NULL) {
            syserr("realloc");
        }
    }

    missing[num_missing] = value;
    num_missing++;
}
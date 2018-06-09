#ifndef BUFFER_HANDLER_H
#define BUFFER_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "transmitter.h"

extern ssize_t read_to_buffer(uint64_t *offset);
extern void increment_buffer_idx(void);

#endif

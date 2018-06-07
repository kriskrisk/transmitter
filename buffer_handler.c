#include <zconf.h>

#include "buffer_handler.h"
#include "transmitter.h"
#include "err.h"

extern audio_data **cyclic_buffer;
extern int current_buffer_idx;
extern size_t psize;
extern size_t buffer_size;

static void increment_buffer_idx(void) {
    if (current_buffer_idx == buffer_size - 1) {
        current_buffer_idx = 0;
    } else {
        current_buffer_idx++;
    }
}

ssize_t read_to_buffer(uint64_t *offset) {
    char *audio = malloc(psize);

    // Check if it was possible to read whole block
    if (read(STDIN_FILENO, audio, psize) != psize)
        syserr("read from stdin");

    audio_data *new_audio = (audio_data *)malloc(sizeof(audio_data));

    new_audio->offset = *offset;
    new_audio->audio = audio;
    cyclic_buffer[current_buffer_idx] = new_audio;

    *offset = *offset + psize;
    increment_buffer_idx();
}

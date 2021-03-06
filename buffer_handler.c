#include <zconf.h>

#include "buffer_handler.h"
#include "transmitter.h"
#include "err.h"

extern audio_data **cyclic_buffer;
extern int current_buffer_idx;
extern size_t psize;
extern size_t cyclic_buffer_size;

void increment_buffer_idx(void) {
    if (current_buffer_idx == cyclic_buffer_size - 1) {
        current_buffer_idx = 0;
    } else {
        current_buffer_idx++;
    }
}

ssize_t read_to_buffer(uint64_t *offset) {
    char *audio = malloc(psize);
    if (audio == NULL)
        syserr("read_to_buffer: malloc");

    if (fread((void *) audio, sizeof(char), psize, stdin) != psize) {
        if (feof(stdin))
            return 0;

        syserr("read from stdin");
    }

    audio_data *new_audio = (audio_data *) malloc(sizeof(audio_data));
    if (new_audio == NULL)
        syserr("read_to_buffer: malloc");

    new_audio->offset = *offset;
    new_audio->audio = audio;
    cyclic_buffer[current_buffer_idx] = new_audio;

    *offset = *offset + psize;

    return psize;
}

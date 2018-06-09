#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

#include "err.h"
#include "radio.h"
#include "transmitter.h"
#include "buffer_handler.h"
#include "missing_handler.h"
#include "validate.h"

#define RETMIX_PREFIX_LEN 14

/* Parameters of transmitter */
char *mcast_addr;
uint16_t data_port = DATA_PORT;
uint16_t ctrl_port = CTRL_PORT;
size_t psize = PSIZE;
int fsize = FSIZE;
uint rtime = RTIME;
char *name = NAME;
uint64_t session_id;    // Stored in net order

/* Size of one UDP packet with audio */
size_t audio_packet_size;

audio_data **cyclic_buffer;
size_t cyclic_buffer_size;
int current_buffer_idx; // Index in cyclic_buffer that was read from stdin last time

/* Data about missing datagrams */
int *missing;
size_t missing_size = 2;
size_t num_missing = 0;

pthread_mutex_t missing_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t audio_data_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Receives LOOKUP and REXMIT messages, and sends REPLY */
static void *handle_control_receiver(void *args) {
    char buffer[MAX_UDP_SIZE];
    char reply_buffer[REPLY_SIZE];
    struct sockaddr_in client_address, server_address;
    socklen_t client_address_len = sizeof(struct sockaddr_in);
    int new_int, sent_len;
    char *beginning;
    //int sock_reply = setup_sender(mcast_addr, ctrl_port);

    /////////////////// Receiver

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        syserr("socket");

    int enable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        syserr("SO_REUSEADDR failed");

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(ctrl_port);

    if (bind(sock, (struct sockaddr *) &server_address, (socklen_t) sizeof(server_address)) < 0)
        syserr("bind");

    //////////////////// End receiver

    while (true) {
        if (recvfrom(sock, buffer, MAX_UDP_SIZE, 0, (struct sockaddr *)&client_address, &client_address_len) < 0) {
            syserr("read");
        }

        switch (validate_control_protocol(buffer)) {
            case LOOKUP:
                sent_len = snprintf(reply_buffer, REPLY_SIZE, "BOREWICZ_HERE %s %" PRIu16 " %s\n",
                                    mcast_addr,
                                    data_port,
                                    name);

                assert(sent_len <= REPLY_SIZE);

                if (sendto(sock, reply_buffer, (size_t)sent_len, 0, (struct sockaddr *)&client_address, client_address_len) != sent_len)
                    syserr("sendto");

                memset(reply_buffer, 0, REPLY_SIZE);
                break;
            case REXMIT:
                beginning = buffer + RETMIX_PREFIX_LEN;

                /* Parse list of offsets */
                while ((new_int = (int)strtol(beginning, &beginning, 10)) != 0) {
                    pthread_mutex_lock(&missing_mutex);
                    addNewValueToMissing(new_int);
                    pthread_mutex_unlock(&missing_mutex);

                    if (*beginning != '\n')
                        beginning++;
                    else
                        break;
                }

                break;
        }
    }
}

static void *handle_audio(void *args) {
    int sock;
    uint64_t offset = 0;
    uint64_t first_byte;
    ssize_t read_len;

    /* Buffer for sending audio */
    char audio_buffer[audio_packet_size];

    sock = setup_sender(mcast_addr, data_port);

    while (true) {
        pthread_mutex_lock(&audio_data_mutex);
        read_len = read_to_buffer(&offset);
        pthread_mutex_unlock(&audio_data_mutex);

        if (read_len == 0) {
            /* End of stdin */
            break;
        }

        first_byte = htobe64(cyclic_buffer[current_buffer_idx]->offset);

        memcpy(audio_buffer, (void *) &session_id, sizeof(uint64_t));
        memcpy(audio_buffer + sizeof(uint64_t), (void *) &first_byte, sizeof(uint64_t));
        memcpy(audio_buffer, cyclic_buffer[current_buffer_idx]->audio, psize);

        if (write(sock, audio_buffer, audio_packet_size) != audio_packet_size)
            syserr("write");

    }
}

int cmp_func(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

static void *handle_retransmission(void *args) {
    int sock = setup_sender(mcast_addr, data_port);
    char audio_buffer[audio_packet_size];

    while (true) {
        sleep(rtime);

        pthread_mutex_lock(&missing_mutex);
        uint64_t tmp[num_missing];
        size_t number_of_missing;
        memcpy(tmp, missing, num_missing * sizeof(int));
        number_of_missing = num_missing;
        num_missing = 0;
        pthread_mutex_unlock(&missing_mutex);

        qsort(missing, number_of_missing, sizeof(int), cmp_func);

        for (int i = 0; i < number_of_missing; i++) {
            if (i == 0 || tmp[i] != tmp[i - 1]) {
                /* Data should be at this index in array */
                size_t idx = (tmp[i] / psize) % cyclic_buffer_size;

                pthread_mutex_lock(&audio_data_mutex);

                if (cyclic_buffer[idx]->offset == tmp[i]) {
                    uint64_t first_byte = htobe64(tmp[i]);

                    memcpy(audio_buffer, (void *) &session_id, sizeof(uint64_t));
                    memcpy(audio_buffer + sizeof(uint64_t), (void *) &first_byte, sizeof(uint64_t));
                    memcpy(audio_buffer, cyclic_buffer[idx]->audio, psize);

                    if (write(sock, audio_buffer, audio_packet_size) != audio_packet_size)
                        syserr("write");
                }

                pthread_mutex_unlock(&audio_data_mutex);
            }
        }
    }
}

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "a:P:C:p:f:n:R:")) != -1) {
        switch (opt) {
            case 'a':
                mcast_addr = optarg;
                break;
            case 'P':
                data_port = (uint16_t) atoi(optarg);
                break;
            case 'C':
                ctrl_port = (uint16_t) atoi(optarg);
                break;
            case 'p':
                psize = (size_t) atoi(optarg);
                break;
            case 'f':
                fsize = atoi(optarg);
                break;
            case 'n':
                name = optarg;
                break;
            case 'R':
                rtime = (uint)atoi(optarg);
                break;
        }
    }

    /* Setup global values */
    session_id = htobe64((uint64_t) time(NULL));
    cyclic_buffer_size = (size_t) ((fsize / psize) + 1) * psize;
    audio_packet_size = psize + 2 * sizeof(uint64_t);
    missing = (int *)malloc(2 * sizeof(int));

    int control_protocol, audio, retransmission;

    if ((cyclic_buffer = (audio_data **) malloc(cyclic_buffer_size * sizeof(audio_data *))) == NULL) {
        syserr("cyclic buffer allocation");
    }

    pthread_t control_protocol_thread, audio_data_thread, retransmission_thread;

    /* pthread create */
    control_protocol = pthread_create(&control_protocol_thread, 0, handle_control_receiver, NULL);
    if (control_protocol == -1) {
        syserr("control protocol: pthread_create");
    }

    audio = pthread_create(&audio_data_thread, 0, handle_audio, NULL);
    if (audio == -1) {
        syserr("audio: pthread_create");
    }
/*
    retransmission = pthread_create(&retransmission_thread, 0, handle_retransmission, NULL);
    if (retransmission == -1) {
        syserr("retransmission: pthread_create");
    }
*/

    /* pthread join */
    int err = pthread_join(audio_data_thread, NULL);
    if (err != 0) {
        fprintf(stderr, "Error in pthread_join: %d (%s)\n", err, strerror(err));
        exit(EXIT_FAILURE);
    }

    exit(0);
}

/*EOF*/

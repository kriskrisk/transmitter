#ifndef RADIO_H
#define RADIO_H

#include <stdint.h>
#include <netinet/in.h>

#define DATA_PORT 21240
#define CTRL_PORT 31240
#define PSIZE 512
#define FSIZE 131072
#define RTIME 250
#define NAME "Nienazwany Nadajnik"
#define MAX_UDP_SIZE 65536
#define REPLY_SIZE 100
#define TTL_VALUE 64

extern int setup_sender(char *remote_dotted_address, in_port_t remote_port);

#endif


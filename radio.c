#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>

#include "err.h"
#include "radio.h"

int setup_sender(char *remote_dotted_address, in_port_t remote_port) {
    int sock, optval;
    struct sockaddr_in remote_address;

    /* Open socket */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        syserr("socket");

    /* Activate broadcast */
    optval = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *) &optval, sizeof optval) < 0)
        syserr("setsockopt broadcast");

    /* Setup TTL for datagrams sent to group */
    optval = TTL_VALUE;
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &optval, sizeof optval) < 0)
        syserr("setsockopt multicast ttl");

    /* Setup address and port number of receiver */
    remote_address.sin_family = AF_INET;
    remote_address.sin_port = htons(remote_port);
    if (inet_aton(remote_dotted_address, &remote_address.sin_addr) == 0)
        syserr("inet_aton");
    if (connect(sock, (struct sockaddr *) &remote_address, sizeof remote_address) < 0)
        syserr("connect");

    return sock;
}

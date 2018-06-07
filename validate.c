#include <memory.h>
#include <ctype.h>
#include <stdio.h>
#include "validate.h"
#include "radio.h"

#define REXMIT_LEN 13
#define LOOKUP_LEN 19

/* Validates message and returns its type */
int validate_control_protocol(char *protocol_msg) {      // protocol_msg is allocated to MAX_UDP_SIZE
    char dummy[MAX_UDP_SIZE];

    if (*protocol_msg == 'Z') {             // ZERO_SEVEN_COME_IN
        if (sscanf(protocol_msg, "%s\n", dummy) == 1 && strncmp(dummy, "ZERO_SEVEN_COME_IN\n", LOOKUP_LEN) == 0)
            return  LOOKUP;
    } else if (*protocol_msg == 'L') {      // LOUDER_PLEASE 512,1024,1536,5632,3584
        char temp1[MAX_UDP_SIZE];
        char temp2[MAX_UDP_SIZE];

        if (sscanf(protocol_msg, "%s %{%[1-99],%}%[1-99]\n", dummy, temp1, temp2) == 3 &&
                && strncmp(dummy, "LOUDER_PLEASE", REXMIT_LEN) == 0)
            return REXMIT;
    }

    return IGNORE;
}

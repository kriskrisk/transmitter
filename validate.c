#include <memory.h>
#include <ctype.h>
#include <stdio.h>

#include "validate.h"
#include "radio.h"

/* Validates message and returns its type */
int validate_control_protocol(char *protocol_msg) {      // protocol_msg is allocated to MAX_UDP_SIZE
    if (*protocol_msg == 'Z') {             // ZERO_SEVEN_COME_IN
        if (sscanf(protocol_msg, "ZERO_SEVEN_COME_IN\n") == 0)
            return  LOOKUP;
    } else if (*protocol_msg == 'L') {      // LOUDER_PLEASE 512,1024,1536,5632,3584
        char temp1[MAX_UDP_SIZE];
        char temp2[MAX_UDP_SIZE];

        if (sscanf(protocol_msg, "LOUDER_PLEASE %{%[1-99],%}%[1-99]\n", temp1, temp2) == 2)
            return REXMIT;
    }

    return IGNORE;
}

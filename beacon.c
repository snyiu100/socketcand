#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>

#include "socketcand.h"
#include "beacon.h"

void *beacon_loop(void *ptr) {
    int i, n, chars_left;
    int udp_socket;
    struct sockaddr_in s;
    size_t len;
    int optval;
    char buffer[BEACON_LENGTH];
    char hostname[32];

    if ((udp_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        PRINT_ERROR("Failed to create broadcast socket");
        return NULL;
    }

    /* Activate broadcast option */
    optval = 1;
    setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(int));

    /* Connect the socket */
    len = sizeof(broadcast_addr);
    if (connect(udp_socket, (struct sockaddr *) &broadcast_addr, len) < 0) {
        PRINT_ERROR("Failed to connect broadcast socket");
        return NULL;
    }

    while(1) {
        /* Build the beacon */
        gethostname((char *) &hostname, (size_t)  32);
        snprintf(buffer, BEACON_LENGTH, "<CANBeacon name=\"%s\" type=\"%s\" description=\"%s\">\n<URL>can://%s:%d</URL>", 
                hostname, BEACON_TYPE, description, inet_ntoa( laddr ), port);

        for(i=0;i<interface_count;i++) {
            /* Find \0 in beacon buffer */
            for(n=0;;n++) {
                if(buffer[n] == '\0')
                    break;
            }
            chars_left = BEACON_LENGTH - n;

            snprintf(buffer+(n*sizeof(char)), chars_left, "<Bus name=\"%s\"/>", interface_names[i]);
        }
        
        /* Find \0 in beacon buffer */
        for(n=0;;n++) {
            if(buffer[n] == '\0')
                break;
        }
        chars_left = BEACON_LENGTH - n;

        snprintf(buffer+(n*sizeof(char)), chars_left, "</CANBeacon>");

        sendto(udp_socket, buffer, strlen(buffer), 0, (struct sockaddr *) &s, sizeof(struct sockaddr_in));
        sleep(3);
    }

    return NULL;
}

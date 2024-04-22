#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "vm/quivm.h"
#include "dev/network.h"

/* Constants. */
#define BIND_ADDRESS             "0.0.0.0"
#define TARGET_ADDRESS   "255.255.255.255"
#define PORT                         42400
#define MAX_PACKET_SIZE               1024

/* Data structures and types */
/* Internal data structure for the network device */
struct ntw_internal {
    int sockfd;                 /* socket file descriptor */
    struct sockaddr addr;       /* the address for sending data */
};

/* Functions */

int network_init(struct network *ntw)
{
    struct ntw_internal *ni;

    ntw->internal = NULL;

    ni = (struct ntw_internal *) malloc(sizeof(struct ntw_internal));
    if (!ni) {
        fprintf(stderr, "dev/network: init: "
                "memory exhausted\n");
        network_destroy(ntw);
        return 1;
    }

    ni->sockfd = -1;

    ntw->bind_address = BIND_ADDRESS;
    ntw->target_address = TARGET_ADDRESS;
    ntw->port = PORT;
    ntw->internal = ni;
    ntw->initialized = 0;
    return 0;
}

void network_destroy(struct network *ntw)
{
    if (ntw->internal) {
        struct ntw_internal *ni;
        ni = (struct ntw_internal *) ntw->internal;
        if (ni->sockfd >= 0) {
            close(ni->sockfd);
            ni->sockfd = -1;
        }
        free(ni);
    }
    ntw->internal = NULL;
}

void network_configure(struct network *ntw, const char *bind_address,
                       const char *target_address, int port)
{
    ntw->bind_address = (bind_address) ? bind_address : BIND_ADDRESS;
    ntw->target_address = (target_address) ? target_address
                                           : TARGET_ADDRESS;
    ntw->port = (port) ? port : PORT;
}

uint32_t network_read_callback(struct network *ntw,
                               struct quivm *qvm, uint32_t address)
{
    uint32_t v;
    (void)(qvm); /* UNUSED */

    switch (address) {
    case IO_NETWORK_DATA:
        v = ntw->data;
        break;
    case IO_NETWORK_LEN:
        v = ntw->len;
        break;
    case IO_NETWORK_OP:
        v = ntw->op;
        break;
    default:
        v = -1;
        break;
    }
    return v;
}

/* Opens the UDP socket and bind its address.
 * Returns zero on success.
 */
static
int start_network(struct network *ntw)
{
    struct ntw_internal *ni;
#ifndef __EMSCRIPTEN__
    struct timeval tv;
    int val;
#endif
    struct addrinfo hints, *addrs;
    struct sockaddr_in bind_addr;
    char port_str[16];
    int ret;

    ni = (struct ntw_internal *) ntw->internal;
    ni->sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (ni->sockfd < 0) {
        fprintf(stderr, "dev/network: start_network: "
                "could not create UDP socket: %s\n",
                strerror(errno));
        return ni->sockfd;
    }

#ifndef __EMSCRIPTEN__
    /* allow multiple clients on the same port */
    val = 1;
    ret = setsockopt(ni->sockfd, SOL_SOCKET, SO_REUSEADDR,
                     &val, sizeof(val));
    if (ret < 0) {
        fprintf(stderr, "dev/network: start_network: "
                "could not set SO_REUSEADDR: %s\n",
                strerror(errno));
        goto error_start;
    }

    /* allow broadcasting */
    val = 1;
    ret = setsockopt(ni->sockfd, SOL_SOCKET, SO_BROADCAST,
                     &val, sizeof(val));
    if (ret < 0) {
        fprintf(stderr, "dev/network: start_network: "
                "could not set SO_BROADCAST: %s\n",
                strerror(errno));
        goto error_start;
    }

    /* set the receive timeout */
    tv.tv_sec = 0;
    tv.tv_usec = 10;
    ret = setsockopt(ni->sockfd, SOL_SOCKET, SO_RCVTIMEO,
                     &tv, sizeof(tv));
    if (ret < 0) {
        fprintf(stderr, "dev/network: start_network: "
                "could not set SO_RCVTIMEO: %s",
                strerror(errno));
        goto error_start;
    }
#endif /* ! def __EMSCRIPTEN__ */

    /* resolve the target address */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    sprintf(port_str, "%d", ntw->port);
    ret = getaddrinfo(ntw->target_address, port_str, &hints, &addrs);
    if (ret) {
        fprintf(stderr, "dev/network: start_network: "
                "could not resolve %s: %s\n",
                ntw->target_address, gai_strerror(ret));
        goto error_start;
    }
    ni->addr = addrs->ai_addr[0];
    freeaddrinfo(addrs);

    /* bind the address */
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(ntw->port);
    ret = inet_pton(AF_INET, ntw->bind_address, &bind_addr.sin_addr);
    if (ret != 1) {
        fprintf(stderr, "dev/network: start_network: "
                "invalid bind address: %s",
                ntw->bind_address);
        ret = -1;
        goto error_start;
    }

    ret = bind(ni->sockfd,
               (struct sockaddr *) &bind_addr,
               sizeof(bind_addr));
    if (ret < 0) {
        fprintf(stderr, "dev/network: start_network: "
                "could not bind socket to port %d: %s",
                ntw->port, strerror(errno));
        goto error_start;
    }

    return 0;

 error_start:
    close(ni->sockfd);
    ni->sockfd = -1;
    return ret;
}

/* Receives a packet from the network.
 * The packet data is stored in `message`, whose maximum
 * length is given by `length`.
 * Returns the number of bytes received (or negative for error).
 */
static
int receive_data(struct network *ntw,
                 void *message, size_t length)
{
    struct ntw_internal *ni;
    int ret;

    ni = (struct ntw_internal *) ntw->internal;
    ret = recvfrom(ni->sockfd, message, length,
                   0, NULL, NULL);
    return ret;
}

/* Sends a packet across the network.
 * The packet data is given by `message`, whose length is
 * given by `length`.
 * Returns the number of bytes send (or negative for error).
 */
static
int send_data(struct network *ntw,
              const void *message, size_t length)
{
    struct ntw_internal *ni;
    int ret;

    ni = (struct ntw_internal *) ntw->internal;
    ret = sendto(ni->sockfd, message, length, 0,
                 &ni->addr, sizeof(ni->addr));
    if (ret < 0) {
        fprintf(stderr, "udp_transport: send_data: "
                "could not send packet: %s\n",
                strerror(errno));
    }

    return ret;
}

/* Performs the network operation */
static
void do_operation(struct network *ntw, struct quivm *qvm)
{
    if ((ntw->op != NETWORK_OP_RECEIVE) && (ntw->op != NETWORK_OP_SEND)) {
        ntw->len = -1;
        return;
    }

    if (ntw->data < qvm->memsize) {
        if (ntw->len > (qvm->memsize - ntw->data))
            ntw->len = qvm->memsize - ntw->data;
    } else {
        ntw->len = 0;
    }

    if (!ntw->initialized) {
        if (start_network(ntw)) {
            /* terminate the VM */
            qvm->status |= STS_TERMINATED;
            qvm->termvalue = 1;
            return;
        }
    }
    ntw->initialized = 1;

    switch (ntw->op) {
    case NETWORK_OP_RECEIVE:
        ntw->len = receive_data(ntw, &qvm->mem[ntw->data], ntw->len);
        break;
    case NETWORK_OP_SEND:
        ntw->len = send_data(ntw, &qvm->mem[ntw->data], ntw->len);
        break;
    }
}

void network_write_callback(struct network *ntw, struct quivm *qvm,
                            uint32_t address, uint32_t v)
{
    switch (address) {
    case IO_NETWORK_DATA:
        ntw->data = v;
        break;
    case IO_NETWORK_LEN:
        ntw->len = v;
        break;
    case IO_NETWORK_OP:
        ntw->op = v;
        do_operation(ntw, qvm);
        break;
    }
}


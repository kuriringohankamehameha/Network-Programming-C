#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <poll.h>
#include <assert.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

// Set it to whatever size you want
#define PACKET_SIZE 5

// Obviously you want to test with more than two channels
// so it's another macro. I've generalized it to work with N channels
// As always, change in both the client and server files, since I'm lazy to
// include a "utils.h" header file.
#define NUM_CHANNELS 3

// Packet Drop Rate (Probability)
#define PDR 0.1

// Timeout Macro (Client Side)
#define TIMEOUT_SECS 2

// Macro Hax
// Take htonl 32 bits at a time and shift both sides. Finally bitwise OR the result. Damn.
#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

typedef struct {
    /* START OF PACKET HEADER */
    uint64_t size; // Size, in bytes, of the payload
    uint64_t seq_no; // The sequence number (in terms of byte). Gives offset of first byte of packet wrt i/p file
    uint8_t is_last; //Is this the last packet?
    uint8_t type; // Whether Data or Ack
    uint8_t channel_id; // Which channel am I sent to?
    /* END OF PACKET HEADER */
}Header;

typedef struct {
    Header header;
    char data[PACKET_SIZE]; // The actual payload data
}Packet;


void error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void hton_header(Header header, char buffer[sizeof(Header)]) {
    // Pointless function in the end. Absolutely useless.
    // Header size is 19 bytes
    uint64_t size = htonll(header.size);
    memcpy(buffer + 0, &size, 8);
    uint64_t seq_no = htonll(header.seq_no);
    memcpy(buffer + 8, &seq_no, 8);
    // Remaining part doesn't need to be converted, since it needs
    // only one byte per field
    memcpy(buffer + 16, &(header.is_last), 1);
    memcpy(buffer + 17, &(header.type), 1);
    memcpy(buffer + 18, &(header.channel_id), 1);
}

char* get_next_chunk(FILE* fp, char* buffer, int chunk_size, int* is_eof, int* payload_size) {
    int ret = fread(buffer, sizeof(char), chunk_size, fp);
    buffer[ret] = '\0';
    *payload_size = ret;
    // printf("ret = %d, chunk_size = %d\n", ret, chunk_size);
    if (ret < chunk_size) {
        // End of file possibly
        *is_eof = 1;
        if (ret > 0) {
            // For some reason, I must subtract one character
            // when EOF is reached and if ret is not zero
            buffer[ret - 1] = '\0';
            *payload_size = ret - 1;
        }
        return buffer;
    }
    else {
        return buffer;
    }
}

#endif

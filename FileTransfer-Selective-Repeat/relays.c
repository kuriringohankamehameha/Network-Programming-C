#include "utils.h"

int listenfd[NUM_CHANNELS];
uint64_t last_seq_no = 0;

static void signal_handler(int signo) {
    // Press Ctrl-C to stop this useless program
    if (signo == SIGINT) {
        printf("\nServer Exiting...\n");
        for (int i=0; i < NUM_CHANNELS; i++) {
            shutdown(listenfd[i], SHUT_RDWR);
            close(listenfd[i]);
        }
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char* argv[]) {
    int server_port;
    if (argc != (2)) {
        fprintf(stderr, "Format : ./server <PORT_NO>");
        fprintf(stderr, "\n");
        exit(0);
    }

    FILE* fp = fopen("output.txt", "wb");
    
    int MAX_CLIENTS = 3; // Max 3 pending connections
    
    socklen_t clilen;
    
    struct sockaddr_in serv_addr[NUM_CHANNELS];
    struct sockaddr_in cli_addr;

    struct sockaddr relay_address[NUM_CHANNELS];

    server_port = atoi(argv[1]);
    for (int i=0; i < NUM_CHANNELS; i++) {
        listenfd[i] = socket(AF_INET, SOCK_DGRAM, 0);
        if (listenfd[i] < 0) {
            fclose(fp);
            perror("Error opening socket");
            exit(EXIT_FAILURE);
        }

        bzero((char*)&(serv_addr[i]), sizeof(serv_addr[i]));
        
        serv_addr[i].sin_family = AF_INET;
        serv_addr[i].sin_addr.s_addr = INADDR_ANY;
        serv_addr[i].sin_port = htons(server_port + i);
        
        if (bind(listenfd[i], (struct sockaddr*)&(serv_addr[i]), sizeof(serv_addr[i])) < 0) {
            fclose(fp);
            perror("Error binding");
            exit(EXIT_FAILURE);
        }

        listen(listenfd[i], MAX_CLIENTS);
        int addr_len;
        getsockname(listenfd[i], &(relay_address[i]), &addr_len);
        char ip[16];
        inet_ntop(AF_INET, &(relay_address[i]), ip,  sizeof(ip));
        printf("Relay<%d> Address: %s Port: %d\n", i+1, ip, ntohs(serv_addr[i].sin_port));
    }
    clilen = sizeof(cli_addr);

    signal(SIGINT, signal_handler);

    fd_set readfds;

    int is_last = 0;
    
    srand((unsigned int) time(NULL));

    while(is_last != 1) {
        // Clear the socket set
        FD_ZERO(&readfds);
        
        // Add server listening socket
        FD_SET(listenfd[0], &readfds);
        int max_sd = listenfd[0];
        for (int i=1; i < NUM_CHANNELS; i++) {
            int sd = listenfd[i];
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            fclose(fp);
            perror("select()");
            exit(EXIT_FAILURE);
        }

        for (int i=0; i < NUM_CHANNELS; i++) {
            if (FD_ISSET(listenfd[i], &readfds)) {
                Packet recv_packet;
                uint64_t seq_no = 1;
                uint8_t is_last = 0;
                
                socklen_t addr_len = sizeof(struct sockaddr_in);

                int num_bytes = recvfrom(listenfd[i], &recv_packet, sizeof(Packet), 0, (struct sockaddr*)&cli_addr, &addr_len);
                // printf("New Connection on Listening Socket %d: IP is : %s, PORT : %d\n", listenfd[i], inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                if (num_bytes < 0) {
                    perror("Error reading from socket");
                    exit(EXIT_FAILURE);
                }

                // Drop a packet with probability PDR
                int res = (rand() <  (double) PDR * ((double)RAND_MAX + 1.0));
                if (res == 1) {
                    // Drop this packet
                    // Don't send anything
                    printf("Dropped packet with seq_no: %lu\n", ntohll(recv_packet.header.seq_no));
                }
                else {
                    seq_no = ntohll(recv_packet.header.seq_no);
                    uint64_t pkt_size = ntohll(recv_packet.header.size);
                    if (recv_packet.header.is_last != 1) {
                        // I have no clue what you meant by handling out of order reception
                        // with buffering, when you mentioned you cannot write to the file using
                        // a buffer. So I'm simply moving the pointer position based on the offset
                        fseek(fp, seq_no, SEEK_SET);
                        fwrite(recv_packet.data, 1, pkt_size, fp);
                    }
                    is_last = recv_packet.header.is_last;
                    if (is_last == 0)
                        printf("Received Packet: Seq. No. %lu of size %lu Bytes from Channel %d\n", seq_no, pkt_size, recv_packet.header.channel_id);
                    else
                        printf("Received FIN Packet: Seq. No. %lu from Channel %d\n", seq_no, recv_packet.header.channel_id);
                    Packet send_packet = recv_packet;
                    send_packet.header.seq_no = htonll(seq_no);
                    send_packet.header.type = 1; // Sending ACK
                    num_bytes = sendto(listenfd[i], &send_packet, sizeof(Packet), 0, (struct sockaddr*)&cli_addr, addr_len);
                    if (num_bytes < 0) {
                        perror("Error writing to socket");
                        exit(EXIT_FAILURE);
                    }
                    if (is_last == 0)
                        printf("Sent ACK: For packet with Seq. No. %lu from channel %d\n", seq_no, send_packet.header.channel_id);
                    else
                        printf("Sent FIN ACK: For packet with Seq. No. %lu from channel %d\n", seq_no, send_packet.header.channel_id);
                    if (seq_no > last_seq_no) last_seq_no = seq_no;
                }
            }
        }
    }
    fclose(fp);
    for (int i=0; i < NUM_CHANNELS; i++) {
        shutdown(listenfd[i], SHUT_RDWR);
        close(listenfd[i]);
    }
    return 0;
}

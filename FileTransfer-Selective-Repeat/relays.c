#include "utils.h"

int listenfd[NUM_CHANNELS];
int serverfd;
uint64_t last_seq_no = 0;

static void signal_handler(int signo) {
    // Press Ctrl-C to stop this useless program
    if (signo == SIGINT) {
        printf("\nServer Exiting...\n");
        for (int i=0; i < NUM_CHANNELS; i++) {
            shutdown(listenfd[i], SHUT_RDWR);
            close(listenfd[i]);
        }
        shutdown(serverfd, SHUT_RDWR);
        close(serverfd);
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char* argv[]) {
    int server_port;
    if (argc != (4)) {
        fprintf(stderr, "Format : %s <START_PORT_NO> <SERVER_IP> SERVER_PORT>", argv[0]);
        fprintf(stderr, "\n");
        exit(0);
    }

    int MAX_CLIENTS = NUM_CHANNELS; // Max 3 pending connections
    
    socklen_t clilen;
    
    struct sockaddr_in serv_addr[NUM_CHANNELS] = {0};
    struct sockaddr_in cli_addr = {0};
    struct sockaddr_in dst_addr = {0};

    struct sockaddr relay_address[NUM_CHANNELS];

    server_port = atoi(argv[3]);
    
    serverfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverfd < 0) {
        error("Error opening socket");
    }

    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(server_port);

    int start_port = atoi(argv[1]);
    for (int i=0; i < NUM_CHANNELS; i++) {
        listenfd[i] = socket(AF_INET, SOCK_DGRAM, 0);
        if (listenfd[i] < 0) {
            perror("Error opening socket");
            exit(EXIT_FAILURE);
        }

        bzero((char*)&(serv_addr[i]), sizeof(serv_addr[i]));
        
        serv_addr[i].sin_family = AF_INET;
        serv_addr[i].sin_addr.s_addr = INADDR_ANY;
        serv_addr[i].sin_port = htons(start_port + i);
        
        if (bind(listenfd[i], (struct sockaddr*)&(serv_addr[i]), sizeof(serv_addr[i])) < 0) {
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
        FD_SET(serverfd, &readfds);
        int max_sd = serverfd;
        for (int i=0; i < NUM_CHANNELS; i++) {
            int sd = listenfd[i];
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select()");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(serverfd, &readfds)) {
            // Server must send ACKs through this socket
            // ACKs are NOT lost
            Packet recv_packet;
            socklen_t addr_len = sizeof(struct sockaddr_in);
            int num_bytes = recvfrom(serverfd, &recv_packet, sizeof(Packet), 0, (struct sockaddr*)&dst_addr, &addr_len);
            if (num_bytes < 0) {
                error("recvfrom()");
            }
            is_last = recv_packet.header.is_last;
            int relay_no = rand() % NUM_CHANNELS;
            
            Packet send_packet = recv_packet;
            send_packet.header.seq_no = recv_packet.header.seq_no;
            uint64_t seq_no = ntohll(recv_packet.header.seq_no);
            send_packet.header.type = 1; // Sending ACK
            
            num_bytes = sendto(listenfd[relay_no], &send_packet, sizeof(Packet), 0, (struct sockaddr*)&cli_addr, addr_len);
            if (num_bytes < 0) {
                perror("Error writing to socket");
                exit(EXIT_FAILURE);
            }
            if (is_last == 0)
                printf("Server Sent ACK to relay %d: For packet with Seq. No. %lu from channel %d\n", relay_no + 1, seq_no, send_packet.header.channel_id);
            else
                printf("Server Sent FIN ACK to relay %d: For packet with Seq. No. %lu from channel %d\n", relay_no + 1, seq_no, send_packet.header.channel_id);
        }

        for (int i=0; i < NUM_CHANNELS; i++) {
            if (FD_ISSET(listenfd[i], &readfds)) {
                Packet recv_packet;
                uint64_t seq_no = 1;
                
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
                    is_last = recv_packet.header.is_last;
                    if (is_last == 0)
                        printf("Relay %d Received Packet: Seq. No. %lu of size %lu Bytes from Channel %d\n", i+1, seq_no, pkt_size, recv_packet.header.channel_id);
                    else
                        printf("Relay %d Received FIN Packet: Seq. No. %lu from Channel %d\n", i+1, seq_no, recv_packet.header.channel_id);

                    // Forward the packet to the Server
                    num_bytes = sendto(serverfd, &recv_packet, sizeof(Packet), 0, (struct sockaddr*)&dst_addr, addr_len);
                    if (num_bytes < 0) {
                        perror("Error writing to socket");
                        exit(EXIT_FAILURE);
                    }

                    if (seq_no > last_seq_no) last_seq_no = seq_no;
                }
            }
        }
    }
    for (int i=0; i < NUM_CHANNELS; i++) {
        shutdown(listenfd[i], SHUT_RDWR);
        close(listenfd[i]);
    }
    shutdown(serverfd, SHUT_RDWR);
    close(serverfd);
    return 0;
}

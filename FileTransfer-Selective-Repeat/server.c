#include "utils.h"

int listenfd;
uint64_t last_seq_no = 0;

static void signal_handler(int signo) {
    // Press Ctrl-C to stop this useless program
    if (signo == SIGINT) {
        printf("\nServer Exiting...\n");
        shutdown(listenfd, SHUT_RDWR);
        close(listenfd);
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char* argv[]) {
    int server_port;
    if (argc != 3) {
        printf("Format : ./server MAX_CLIENTS PORT_NO\n");
        exit(0);
    }

    FILE* fp = fopen("output.txt", "wb");
    
    int MAX_CLIENTS = atoi(argv[1]);
    
    socklen_t clilen;
    
    struct sockaddr_in serv_addr, cli_addr;

    server_port = atoi(argv[2]);

    listenfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listenfd < 0) {
        fclose(fp);
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(server_port);
    
    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        fclose(fp);
        perror("Error binding");
        exit(EXIT_FAILURE);
    }

    listen(listenfd, MAX_CLIENTS);
    clilen = sizeof(cli_addr);

    signal(SIGINT, signal_handler);

    fd_set readfds;

    int is_last = 0;
    
    srand((unsigned int) time(NULL));

    while(is_last != 1) {
        // Clear the socket set
        FD_ZERO(&readfds);
        
        // Add server listening socket
        FD_SET(listenfd, &readfds);
        int max_sd = listenfd;

        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            fclose(fp);
            perror("select()");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(listenfd, &readfds)) {
            Packet recv_packet;
            uint64_t seq_no = 1;
            uint8_t is_last = 0;
            
            struct sockaddr_storage client_address;
            socklen_t addr_len;

            int num_bytes = recvfrom(listenfd, &recv_packet, sizeof(Packet), 0, (struct sockaddr*)&client_address, &addr_len);
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
                num_bytes = sendto(listenfd, &send_packet, sizeof(Packet), 0, (struct sockaddr*)&client_address, addr_len);
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
    fclose(fp);
    close(listenfd);
    return 0;
}

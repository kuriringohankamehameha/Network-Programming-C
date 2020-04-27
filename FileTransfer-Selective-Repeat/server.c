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
    if (argc != 2) {
        printf("Format : ./server PORT_NO\n");
        exit(0);
    }

    FILE* fp = fopen("output.txt", "wb");
    
    int MAX_CLIENTS = atoi(argv[1]);
    
    socklen_t clilen;
    
    struct sockaddr_in serv_addr, cli_addr;

    server_port = atoi(argv[1]);

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
    int addr_len;
    struct sockaddr server_address = {0};
    getsockname(listenfd, &server_address, &addr_len);
    char ip[16];
    inet_ntop(AF_INET, &server_address, ip, sizeof(ip));
    printf("Server Address: %s Port %d\n", ip, server_port);

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
            
            socklen_t addr_len;

            int num_bytes = recvfrom(listenfd, &recv_packet, sizeof(Packet), 0, (struct sockaddr*)&cli_addr, &addr_len);
            // printf("New Connection on Listening Socket %d: IP is : %s, PORT : %d\n", listenfd, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            if (num_bytes < 0) {
                perror("Error reading from socket");
                exit(EXIT_FAILURE);
            }

            seq_no = ntohll(recv_packet.header.seq_no);
            uint64_t pkt_size = ntohll(recv_packet.header.size);
            if (recv_packet.header.is_last != 1) {
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
            num_bytes = sendto(listenfd, &send_packet, sizeof(Packet), 0, (struct sockaddr*)&cli_addr, addr_len);
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
    fclose(fp);
    close(listenfd);
    return 0;
}

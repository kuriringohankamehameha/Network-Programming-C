#include "utils.h"

void connection_loop(int sockfd[NUM_CHANNELS], char* buffer, int buffer_size, int chunk_size, FILE* fp) {
    int is_eof = 0;
    int payload_size = 0;
    uint64_t seq_no = 0;
    uint64_t eof_seq_no = -1;

    struct pollfd pollfds[NUM_CHANNELS];
    for (int i=0; i < NUM_CHANNELS; i++) {
        pollfds[i].fd = sockfd[i];
        pollfds[i].events = 0;
        pollfds[i].events |= POLLIN;
    }
    
    time_t timeout = TIMEOUT_SECS * 1000; // 2000 milliseconds

    Packet tmp[NUM_CHANNELS] = {0};

    while (is_eof != 1) {
        // printf("Please enter the message: ");
        // bzero(buffer, buffer_size);
        // fgets includes newline in buffer input!
        // fgets(buffer,8192,stdin);
        if (eof_seq_no == -1) {
            for (int i=0; i < NUM_CHANNELS; i++) {
                buffer = get_next_chunk(fp, buffer, chunk_size, &is_eof, &payload_size);
                if (is_eof) { eof_seq_no = seq_no; is_eof = 0; }
                
                // Put this onto our packet
                Header header = { htonll( (uint64_t) payload_size ), htonll( seq_no ), 0, 0, (uint8_t) i };
                
                // Populate the packet header
                Packet send_packet = {0};
                send_packet.header = header;
                strncpy(send_packet.data, buffer, PACKET_SIZE);
                tmp[i] = send_packet;
                int n = write(sockfd[i], (void*) &send_packet, sizeof(Packet));
                if (n < 0) {
                    fclose(fp);
                    error("ERROR writing to socket");
                }
                printf("Sent Packet: With Seq. No. %lu of size %lu Bytes, via Channel %d\n", seq_no, ((uint64_t) payload_size), i);
                seq_no += ntohll(send_packet.header.size);
            }
        }
        
        for (int i=0; i < NUM_CHANNELS; i++) {
            Packet recv_packet;
            struct pollfd pollfds[0];
            pollfds[0].fd = sockfd[i];
            pollfds[0].events = 0;
            pollfds[0].events |= POLLIN;

            while (poll(pollfds, 1, timeout) == 0) {
                printf("Timeout: Retransmitting seq_no %lu on Channel %d\n", ntohll(tmp[i].header.seq_no), i);
                int n = write(sockfd[i], (void*) &tmp[i], sizeof(Packet));
                if (n < 0) {
                    fclose(fp);
                    error("ERROR writing to socket");
                }
            }
            int n = read(sockfd[i], &recv_packet, sizeof(Packet));
            if (n < 0)  {
                fclose(fp);
                error("ERROR reading from socket");
            }
            if (recv_packet.header.type == 1) {
                // Received ACK from Server
                printf("Received ACK: For Packet with Sequence No: %lu, from Channel %d\n", ntohll(recv_packet.header.seq_no), i);
                
                if (ntohll(recv_packet.header.seq_no) == eof_seq_no) {
                    is_eof = 1;
                }
            }
        }

        if (is_eof == 1) {
            // Send a fin packet
            // Put this onto our packet
            Header header = { htonll( (uint64_t) payload_size ), htonll( seq_no ), 1, 1, (uint8_t) 0 };
            
            // Populate the packet header
            Packet send_packet = {0};
            send_packet.header = header;
            strcpy(send_packet.data, "exit");
            int n = write(sockfd[0], (void*) &send_packet, sizeof(Packet));
            if (n < 0) {
                fclose(fp);
                error("ERROR writing to socket");
            }
            seq_no += ntohll(send_packet.header.size);
            Packet recv_packet;
            struct pollfd pollfds[0];
            pollfds[0].fd = sockfd[0];
            pollfds[0].events = 0;
            pollfds[0].events |= POLLIN;

            while (poll(pollfds, 1, timeout) == 0) {
                printf("Timeout: Retransmitting seq_no %lu on Channel %d\n", ntohll(send_packet.header.seq_no), 0);
                int n = write(sockfd[0], (void*) &send_packet, sizeof(Packet));
                if (n < 0) {
                    fclose(fp);
                    error("ERROR writing to socket");
                }
            }
            n = read(sockfd[0], &recv_packet, sizeof(Packet));
            if (n < 0)  {
                fclose(fp);
                error("ERROR reading from socket");
            }
            if (recv_packet.header.type == 1) {
                // Received ACK from Server
                // Final ACK is always at Channel 0
                printf("Received FIN ACK: For Packet with Sequence No: %lu, from Channel %d\n", ntohll(recv_packet.header.seq_no), 0);
                
                if (ntohll(recv_packet.header.seq_no) == eof_seq_no) {
                    is_eof = 1;
                }
            }
        }
        
    }
}

int main(int argc, char *argv[])
{
    int sockfd[NUM_CHANNELS];
    int portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[8192];
    if (argc != 3) {
       fprintf(stderr,"Usage: %s hostname port (Server Port)\n", argv[0]);
       exit(EXIT_FAILURE);
    }
    
    // Open the input file
    FILE* fp = fopen("input.txt", "rb");
    if (!fp) {
        fprintf(stderr, "Error Opening FIle\n");
        exit(EXIT_FAILURE);
    }
    
    portno = atoi(argv[2]);
    
    for (int i=0; i < NUM_CHANNELS; i++) {
        sockfd[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd[i] < 0) 
            error("ERROR opening socket");

        server = gethostbyname(argv[1]);
        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host\n");
            fclose(fp);
            exit(EXIT_FAILURE);
        }
        
        bzero((char *) &serv_addr, sizeof(serv_addr));
        
        serv_addr.sin_family = AF_INET;
        
        bcopy((char *)server->h_addr, 
             (char *)&serv_addr.sin_addr.s_addr,
             server->h_length);
        
        serv_addr.sin_port = htons(portno);
        
        //printf("h_addr: %s\n", inet_ntoa(serv_addr.sin_addr));
        
        if (connect(sockfd[i],(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            fclose(fp);
            error("ERROR connecting");
        }

    }

    int buffer_size = 8192; int chunk_size = PACKET_SIZE;
    // Please don't be that retarded to violate this condition
    assert (buffer_size > chunk_size);
    connection_loop(sockfd, buffer, buffer_size, chunk_size, fp);
    
    fclose(fp);
    for (int i=0; i < NUM_CHANNELS; i++)
        close(sockfd[i]);
    return 0;
}

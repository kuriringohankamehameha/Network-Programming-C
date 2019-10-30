#define _XOPEN_SOURCE 700
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#define BUFSIZ 8192
#define PORT 80

int main(int argc, char* argv[]) {
    char buffer[BUFSIZ];
    int MAX_REQUEST_LEN = 1024;
    char request[MAX_REQUEST_LEN];

    // HTTP Get request header format ---
    // This automatically closes the connection after retrieving the first request,
    // so that the TCP Client can close the TCP Connection. Otherwise, both the 
    // client and Server waits for further requests until an appropriate signal is
    // received
    char request_header[] = "GET / HTTP/1.1\r\nHost: %s\r\n\r\nConnection: close\r\n";
    
    // Defines the entry protocol (TCP/UDP)
    struct protoent* protoent;

    // Default hostname is binded to example.com
    char* hostname = "example.com";
    
    in_addr_t in_addr;
    int request_len;
    int sockfd;
    ssize_t nbytes_total, nbytes_last;
    struct hostent* hostent;
    struct sockaddr_in sockaddr_in;
    int server_port = PORT;

    clock_t start, end;
    float seconds;

    if (argc > 1)
        hostname = argv[1];
    if (argc > 2)
        server_port = strtoul(argv[2], NULL, 10);
    
    start = clock();

    // Defining the request header to get the request length from hostname
    request_len = snprintf(request, MAX_REQUEST_LEN, request_header, hostname);
    if (request_len >= MAX_REQUEST_LEN) {
        fprintf(stderr, "Request length exceeded limit: %d bytes\n", request_len);
        exit(EXIT_FAILURE);
    }

    protoent = getprotobyname("tcp");
    if (protoent == NULL) {
        perror("getprotobyname()");
        end = clock();
        seconds = (float) (end - start) / CLOCKS_PER_SEC;
        fprintf(stdout, "Time taken : %.3lf seconds", seconds);
        exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
    if (sockfd == -1) {
        perror("socket()");
        end = clock();
        seconds = (float) (end - start) / CLOCKS_PER_SEC;
        fprintf(stdout, "Time taken : %.3lf seconds", seconds);
        exit(EXIT_FAILURE);
    }

    hostent = gethostbyname(hostname);
    if (hostent == NULL) {
        fprintf(stderr, "Error: gethostbyname(\"%s\"). Host does not exist.\n", hostname);
        end = clock();
        seconds = (float) (end - start) / CLOCKS_PER_SEC;
        fprintf(stdout, "Time taken : %.3lf seconds", seconds);
        exit(EXIT_FAILURE);
    }
    
    in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
    if (in_addr == (in_addr_t)-1) {
        fprintf(stderr, "Error: inet_addr(\"%s\")\n", *(hostent->h_addr_list));
        end = clock();
        seconds = (float) (end - start) / CLOCKS_PER_SEC;
        fprintf(stdout, "Time taken : %.3lf seconds", seconds);
        exit(EXIT_FAILURE);
    }

    sockaddr_in.sin_addr.s_addr = in_addr;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(server_port);

    if (connect(sockfd, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) {
        perror("connect()");
        end = clock();
        seconds = (float) (end - start) / CLOCKS_PER_SEC;
        fprintf(stdout, "Time taken : %.3lf seconds", seconds);
        exit(EXIT_FAILURE);
    }

    // Send HTTP Request to the server
    nbytes_total = 0;
    while (nbytes_total < request_len) {
        // Traverse the request string until you reach EOF or if any error is encountered
        nbytes_last = write(sockfd, request + nbytes_total, request_len - nbytes_total);
        if (nbytes_last == -1) {
            perror("write()");
            end = clock();
            seconds = (float) (end - start) / CLOCKS_PER_SEC;
            fprintf(stdout, "Time taken : %.3lf seconds", seconds);
            exit(EXIT_FAILURE);
        }
        nbytes_total += nbytes_last;
    }

    // Client now can read the socket fd to receive the output
    while ((nbytes_total = read(sockfd, buffer, BUFSIZ)) > 0) {
        write(STDOUT_FILENO, buffer, nbytes_total);
    }

    if (nbytes_total == -1) {
        perror("read()");
        end = clock();
        seconds = (float) (end - start) / CLOCKS_PER_SEC;
        fprintf(stdout, "Time taken : %.3lf seconds", seconds);
        exit(EXIT_FAILURE);
    }

    close(sockfd);
    end = clock();
    seconds = (float) (end - start) / CLOCKS_PER_SEC;
    fprintf(stdout, "Time taken : %.3lf seconds", seconds);
    exit(EXIT_SUCCESS);
}

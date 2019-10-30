#include <arpa/inet.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#define BUFSIZ 8192
#define PORT 8080
#define KEY (1234)

int listenfd, sockfd;
int sem_id;

union semun {
    int val;
    struct semid_ds *buf;
    short * array;
} argument;


static void signal_handler(int signo) {
    if (signo == SIGINT) {
        printf("\nServer Exiting...\n");
        shutdown(listenfd, SHUT_RDWR);
        close(listenfd);
        close(sockfd);
        semctl(sem_id, 0, IPC_RMID);
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    int server_port;
    if (argc != 2) {
        printf("Format : ./tcp_server_N_clients MAX_CLIENTS\n");
        exit(0);
    }
    int MAX_CLIENTS = atoi(argv[1]);
    socklen_t clilen;
    char buffer[8192];
    struct sockaddr_in serv_addr, cli_addr;
    int num_bytes;

    // Create the semaphore counter
    sem_id = semget(KEY, 1, 0666 | IPC_CREAT); 
    argument.val = MAX_CLIENTS + 1;
    semctl(sem_id, 0, SETALL, argument);
    struct sembuf operations[1];

    operations[0].sem_num = 0;
    operations[0].sem_op = MAX_CLIENTS;
    operations[0].sem_flg = 0;
    semop(sem_id, operations, 1);

    server_port = PORT;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(server_port);
    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding");
        exit(EXIT_FAILURE);
    }

    listen(listenfd, MAX_CLIENTS);
    clilen = sizeof(cli_addr);


    signal(SIGINT, signal_handler);

    while(1) {
        sockfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
        if (sockfd < 0) {
            perror("Error with accept()");
            exit(EXIT_FAILURE);
        }

        printf("New Connection, Socket FD is %d, IP is : %s, PORT : %d\n", listenfd, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        if (fork() == 0) {
            do {
                bzero(buffer, 8192);
                num_bytes = read(sockfd, buffer, 8192);
                if (num_bytes < 0) {
                    perror("Error reading from socket");
                    exit(EXIT_FAILURE);
                }
                printf("Client sent: ");
                printf("%s\n", buffer);
                //char send_buffer[8192];
                num_bytes = write(sockfd, buffer, strlen(buffer));
                if (num_bytes < 0) {
                    perror("Error writing to socket");
                    exit(EXIT_FAILURE);
                }
            } while(strncmp(buffer, "exit", 4) != 0);
            printf("Client has exited\n");
            shutdown(sockfd, SHUT_RDWR);
            close(sockfd);
            exit(EXIT_SUCCESS);
        } else {

        }
    }
    close(listenfd);
    close(sockfd);
    return 0;
}

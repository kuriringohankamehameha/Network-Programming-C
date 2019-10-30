#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define KEY (1234)

int sem_id;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    struct sembuf operations[1];
    sem_id = semget(KEY, 1 , 0666);

    char buffer[8192];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    //printf("h_addr: %s\n", inet_ntoa(serv_addr.sin_addr));
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    operations[0].sem_num = 0;
    operations[0].sem_op = -1;
    operations[0].sem_flg = 0;
    int ret = semop(sem_id, operations, 1);
    if (ret == -1) {
        printf("Maximum No Clients exceeded\n");
        exit(0);
    }
    do {
        printf("Please enter the message: ");
        bzero(buffer,8192);
        fgets(buffer,8192,stdin);
        n = write(sockfd,buffer,strlen(buffer));
        if (n < 0) 
             error("ERROR writing to socket");
        bzero(buffer,8192);
        n = read(sockfd,buffer,8192);
        if (n < 0) 
             error("ERROR reading from socket");
        printf("Server sent: %s\n",buffer);
    }
    while(strncmp(buffer, "exit", 4) != 0);
    operations[0].sem_num = 0;
    operations[0].sem_op = 1;
    operations[0].sem_flg = 0;
    ret = semop(sem_id, operations, 1);
    if (ret == -1) {
        printf("Maximum Number of clients exceeded\n");
    }
    close(sockfd);
    return 0;
}

#define _GNU_SOURCE
#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<signal.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/time.h>

#define PORT 8080
#define BLOCK_SIZE 10 * 1024
#define NUM_SERVERS 2

enum bool { false = 0, true = ~0 };

typedef enum bool bool;

char msg[8192];

void signal_handler(int signo) {
    if (signo == SIGPIPE)
        kill(getpid(), SIGINT);
}

void printPrompt()
{
    printf("client > ");
}

void *recvmessage(void *my_sock)
{
    int sock = *((int *)my_sock);
    int len;

    // client thread always ready to receive message
    while((len = recv(sock, msg, sizeof(msg), 0)) > 0) {
        msg[len] = '\0';
        printf("Server sent:%s", msg);
        // Do msg processing here...
        //fputs(msg,stdout);
        bzero(msg, sizeof(msg));
    }
    return NULL;
}

struct MyData{
    int* socket;
    int node_num;
}MyData;

int fd;

int get_file_size(int fd) {
   struct stat s;
   if (fstat(fd, &s) == -1) {
      int saveErrno = errno;
      fprintf(stderr, "fstat(%d) returned errno=%d.", fd, saveErrno);
      return -1;
   }
   return s.st_size;
}

void *send_files(void *data)
{
    struct MyData params = *((struct MyData*) data);
    int* sock = params.socket;
    int node_num = params.node_num;
    int offset = BLOCK_SIZE * (node_num - 1);
    int file_size = get_file_size(fd);
    int blocks_in_node;
    if (file_size % BLOCK_SIZE == 0)
        blocks_in_node = (int)((file_size/BLOCK_SIZE)/node_num);
    else {
        blocks_in_node = (int)((file_size/BLOCK_SIZE)/node_num);
        if ((node_num) == ((file_size - (file_size % BLOCK_SIZE))%NUM_SERVERS + 1))
            blocks_in_node++;
    }
    int i = blocks_in_node;
    char* buff[BLOCK_SIZE];
    while (i > 0) {
        lseek(fd, offset, SEEK_SET);
        read(fd, buff, BLOCK_SIZE);
        write(sock[node_num], buff, BLOCK_SIZE);
        i--;
        offset += BLOCK_SIZE;
    }
    return NULL;
}

char msg_tok[256];

char** split_string(char* delim) {
    char** op = malloc (100 * sizeof(char*));
    for (int i=0; i<100; i++)
        op[i] = malloc(100);
    int curr = 0;
    char* tok;
    tok = strtok(msg_tok, delim);
    while (tok != NULL) {
        strcpy(op[curr], tok);
        tok = strtok(NULL, delim);
        curr++;
    }
    op[curr] = NULL;
    return op;
}

int main(int argc, char* argv[])
{
    if (argc != 2*NUM_SERVERS + 3) {
        printf("Format : ./client NUMBER IP_ADDRESS0 PORT_1 IP_ADDRESS1 PORT_2 IP_ADDRESS2 ...\n");
        exit(0);
    }
    // Initialise stuff ...
    pthread_t recvt;
    pthread_t servt[NUM_SERVERS];
    int sock[NUM_SERVERS+1];
    int len;
    char send_msg[8192];
    struct sockaddr_in ServerIp[NUM_SERVERS+1];
    char client_name[100];
    char IP_ADDRESS[NUM_SERVERS+1][256];
    int PORTS[NUM_SERVERS+1];
    int curr = 0;
    strcpy(IP_ADDRESS[curr++], argv[2]);
    for (int i=3; curr<=NUM_SERVERS; i+=2) {
        //strcpy(IP_ADDRESS[curr], argv[i+1]);
        strcpy(IP_ADDRESS[curr], argv[i+1]);
        PORTS[curr-1] = atoi(argv[i]);
        curr++;
    }
    for(int j=0; j<curr; j++)
        printf("%s, %d\n", IP_ADDRESS[j], PORTS[j]);
    //IP_ADDRESS = argv[2];
    strcpy(client_name, argv[1]);
    for (int i=0; i<=NUM_SERVERS; i++) {
        printf("%d\n", i);
        sock[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (i == 0) {
            ServerIp[i].sin_port = htons(8080);
        }
        else
            ServerIp[i].sin_port = htons(PORTS[i-1]);
        ServerIp[i].sin_family= AF_INET;
        ServerIp[i].sin_addr.s_addr = inet_addr(IP_ADDRESS[i]);
    }
    
    for (int i=0; i<=NUM_SERVERS; i++) {
        if (i == 0)
            printf("Connecting to FileNameServer....\n");
        else
            printf("Connecting to Node%d....\n", i);
        if( (connect( sock[i] ,(struct sockaddr *)&ServerIp[i],sizeof(ServerIp[i]))) == -1 ) {
            printf("%s\n",IP_ADDRESS[i]);
            printf("Connection to socket failed \n");
            printf("Closing the Socket. Try again later\n");
            shutdown(sock[i], SHUT_RDWR);
            close(sock[i]);
            for (int j=i-1; j>=0 ; j--) {
                shutdown(sock[j], SHUT_RDWR);
                close(sock[j]);
            }
            return 0;
        }
        else {
            if (i == 0)
                printf("Connected to FileNameServer!\n");
            else
                printf("Connected to Node%d!\n", i);
        }
    }

    //creating a client thread which is always waiting for a message
    pthread_create(&recvt, NULL, (void *)recvmessage, &sock);
    
    //ready to read a message from console
    printPrompt();
    while(fgets(msg, 8192, stdin) > 0) {
        if (strncmp(msg, "exit", 4) == 0) {
            printf("Client exited\n");
            break;
        }
        
        memset(&msg_tok, 0, sizeof(msg_tok));
        strcpy(msg_tok, msg);
        char** op = split_string(" ");
        if (op[0] && strcmp(op[0], "cp") == 0) {
            if (op[1] && op[2]) {
                printf("Copying %s to %s\n", op[1], op[2]);
                fd = open(op[1], O_RDONLY);
                if (fd == -1) {
                    printf("Error: Bad file descriptor\n");
                    printPrompt();
                    continue;
                }
                for (int i=0; i<NUM_SERVERS; i++) {
                    struct MyData data = {sock, i+1};
                    pthread_create(&servt[i], NULL, (void *)send_files, &data);
                }
                for (int i=0; i<NUM_SERVERS; i++)
                    pthread_join(servt[i], NULL);
            }
            else {
                printf("Format: cp <src> <dst>\n");
                printPrompt();
                continue;
            }       
        }
        else if (op[0] && strcmp(op[0], "cat") == 0) {
            if (op[1]) {
                strcpy(send_msg, op[1]); 
            }
            else {
                printf("Format: cat <filename>\n");
                printPrompt();
                continue;
            }
        }
        
        //strcpy(send_msg, "Client#");
        bzero(send_msg, sizeof(send_msg));
        strcat(send_msg, client_name);
        strcat(send_msg,":");
        strcat(send_msg,msg);
        len = send(sock[0],send_msg,strlen(send_msg), 0);
        if(len < 0) 
            printf("\n message not sent \n");
        printPrompt();
        //REPL(send_msg + strlen(client_name) + 1);
    }
    
    //thread is closed
    //pthread_join(recvt,NULL);
    for (int i=0; i<=NUM_SERVERS; i++) {
        shutdown(sock[i], SHUT_RDWR);
        close(sock[i]);
    }
    return 0;
}

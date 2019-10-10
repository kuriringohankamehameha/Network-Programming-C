#include "cluster.h"
#define PORT 8080
#define MAX 300
#define MAX_NODES 200

/* The Server creates a socket and binds to a port.
 * It listens on the port and starts accept() to 
 * receive packets. Once a Client has invoked 
 * connect(), a connection is finally established. */

char** nodes;
char** ip;
int curr = 0;
int* client_fds;
int listenfd; // Listening Socket

struct clients {
    struct sockaddr_in clientAddr;
    struct client* next;
};

static void signal_handler(int signo)
{
    if (signo == SIGINT) {
        printf("Server Exiting...\n");
        close(listenfd);
        for (int i=0; i < MAX_NODES; i++) {
            free(nodes[i]);
            free(ip[i]);
        }
        free(nodes);
        free(client_fds);
        free(ip);
        exit(0);
    }
}

void broadcast_message(char* msg, int* sockfds[])
{
    for (int i=0; i<MAX_NODES; i++) {
        if ((*sockfds)[i] != 0)
            send(*(sockfds)[i], msg, strlen(msg), 0);
            //write((*sockfds)[i], msg, msgsize);
    }
}

void chat_with_client(struct sockaddr_in client_addr, int sockfd, int* sockfds[]) 
{ 
    char buff[MAX]; 
    int n; 
   
    char* client_ip = inet_ntoa(client_addr.sin_addr);
    int length = strlen(client_ip);
    
    for (int i=0; i < curr; i++) {
        if (strncmp(client_ip, ip[i], length) == 0) {
            break;
        }
        else {
            if (i == curr - 1) {
                printf("This IP Address %s is blocked.\n", client_ip);
                return;
            }
        }
    }

    printf("IP address is: %s\n", client_ip);
    printf("port is: %d\n", (int) ntohs(client_addr.sin_port));
    for (;;) { 
        bzero(buff, MAX); 
  
        read(sockfd, buff, sizeof(buff)); 

        printf("From client: %sTo client : ", buff); 
        
        bzero(buff, MAX); 
        n = 0; 

        while ((buff[n++] = getchar()) != '\n') 
            ; 
  
        buff[n] = '\n';
        buff[n+1] = '\0';
        broadcast_message(buff, sockfds);
        //write(sockfd, buff, sizeof(buff)); 
  
        if (strncmp("exit", buff, 4) == 0) { 
            printf("Server has closed\n"); 
            break; 
        } 
    } 
} 

int main()
{
    // Store the node names and IPs
    nodes = (char**) malloc (MAX_NODES * sizeof(char*));
    ip = (char**) malloc (MAX_NODES * sizeof(char*));
    for (int i=0; i < MAX_NODES; i++) {
        nodes[i] = (char*) malloc (sizeof(char));
        ip[i] = (char*) malloc (sizeof(char));
    }

    // Get the mapping from the config file
    FILE* fp = fopen("ipmap.cfg", "r");
    if (fp == NULL) {
        printf("Couldn't open ipmap.cfg\n");
        exit(0);
    }

    ssize_t read_size;
    size_t length = 0;
    char* line = NULL;
    while ((read_size = getline(&line, &length, fp)) != -1) {
        if (strncmp(line, "# End", 5) == 0) {
            break;
        }

        if (line[0] == '#') {
            // Comment
            continue;
        }
        for (int i=0; i<read_size; i++) {
            if (i > 0 && line[i] == ',') {
                strncpy(nodes[curr], line, i);
                for (int j=i+1; line[j] != '\0'; j++) {
                    strncpy(ip[curr++], line + i + 1, read_size - i - 1);
                    break;
                }
                break;
            }
        }
    }

    struct sigaction sa = {0};
	sa.sa_sigaction = signal_handler;
	sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);
    fclose(fp);

    int connfd;

    struct sockaddr_in cliaddr, servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        printf("Couldn't create socket\n");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Bind Socket to IP
    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Failed to bind to local endpoint\n");
        exit(0);
    }

    // Start listening
    if (listen(listenfd, 5) != 0) {
        printf("Listen failed\n");
        exit(0);
    }
    else {
        printf("Server listening on Port %d\n", PORT);
    }

    int len = sizeof(cliaddr);

    fd_set readfds;

    int max_fd = 0;
    int sd = 0;
    int activity;
    ssize_t valread;
    char buffer[1025];
    client_fds = (int*) calloc (MAX_NODES,  sizeof(int));
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(listenfd, &readfds);
        max_fd = listenfd;

        for (int i=0; i<curr; i++) {
            if ((sd = client_fds[i]) > 0)
                FD_SET(sd, &readfds);
            if (sd > max_fd)
                max_fd = sd;
        }

        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR))
            printf("Select Error\n");

        if (FD_ISSET(listenfd, &readfds)) {
            connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
            if (connfd < 0) {
                printf("Server accept failed\n");
                exit(0);
            }
            else
                //printf("Server is now accepting Client!\n");
                printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , connfd , inet_ntoa(cliaddr.sin_addr) , ntohs 
                  (cliaddr.sin_port));

            for (int i=0; i < curr; i++) {
                if (client_fds[i] == 0) {
                    client_fds[i] = connfd;
                    printf("Adding to list of sockets as %d\n" , i);
                    break;
                }
            }
        }

        for (int i=0; i<curr; i++) {
            sd = client_fds[i];

            if (FD_ISSET(sd, &readfds)) {
                if ((valread = read(sd, buffer, 1024)) == 0) {
                    getpeername(sd, (struct sockaddr*)&cliaddr, (socklen_t*)&len);
                    printf("Host disconnected , ip %s , port %d \n" ,  
                          inet_ntoa(cliaddr.sin_addr) , ntohs(cliaddr.sin_port)); 
                    close(sd);
                    client_fds[i] = 0;
                }
                else {
                    buffer[valread] = '\0';
                    send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
        chat_with_client(cliaddr, connfd, &client_fds);

    }

    close(listenfd);
    for (int i=0; i < MAX_NODES; i++) {
        free(nodes[i]);
        free(ip[i]);
    }
    free(nodes);
    free(ip);

    return 0;
}

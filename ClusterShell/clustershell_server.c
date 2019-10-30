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
#define MAX 300
#define MAX_NODES 200

/* The Server creates a socket and binds to a port.
 * It listens on the port and starts accept() to 
 * receive packets. Once a Client has invoked 
 * connect(), a connection is finally established. */

char** nodes;
char** ip;
int curr = 0;
int client_fds[MAX_NODES] = {0};
int listenfd;
int num_clients = 0;
int SEND_ALL = 0;
fd_set readfds;
int max_sd;
int SEND_TO_FD = -1;
int SEND_TO_FD_STAR = -1;
pthread_mutex_t mutex;

struct clients {
    int fd;
    int num;
    struct sockaddr_in clientAddr;
    struct clients* next;
};

typedef struct clients clients;

clients* client_list = NULL;

clients* addClient(clients* head, int fd, struct sockaddr_in addr)
{
    if (head == NULL) {
        clients* node = (clients*) malloc (sizeof(clients));
        node->clientAddr = addr;
        node->num = num_clients;
        node->fd = fd;
        node->next = NULL;
        return node;
    }

    clients* temp = head;
    clients* node = (clients*) malloc (sizeof(clients));
    node->clientAddr = addr;
    node->fd = fd;
    node->num = num_clients;
    node->next = temp;
    head = node;
    return head;
}

clients* removeClient(clients* head, int removefd)
{
    clients* temp = head;
    if (temp == NULL)
        return NULL;
    if (temp->fd == removefd) {
        head = temp->next;
        temp->next = NULL;
        free(temp);
        return head;
    }
    if (temp->next == NULL) {
        if (temp->fd == removefd) {
            free(temp);
            return NULL;
        }
        return head;
    }

    while (temp->next) {
        if (temp->next->fd == removefd) { 
            clients* node = temp->next;
            temp->next = node->next;
            node->next = NULL;
            free(node);
            return head;
        }
        temp = temp->next;
    }
    return head;
}

void printClients(clients* head)
{
    clients* temp = head;
    int i = 0;
    while (temp) {
        char* ip_addr = inet_ntoa(temp->clientAddr.sin_addr);
        printf("%d. fd = %d, IP = %s\n", i, temp->fd, ip_addr);
        i++;
        temp = temp->next;
    }
    printf("\n");
    return;
}

void freeClients(clients* head)
{
    if (head == NULL)
        return;
    clients* temp = head;
    while (temp) {
        clients* node = temp;
        temp = temp->next;
        node->next = NULL;
        free(node);
    }
    return;
}

void send_to_all(clients* head, char* msg, int msgsize)
{
    clients* temp = head;
    while(temp != NULL) {
        write(temp->fd, msg, msgsize);
        temp = temp->next;
    }
}

static void signal_handler(int signo)
{
    if (signo == SIGINT) {
        printf("\nServer Exiting...\n");
        shutdown(listenfd, SHUT_RDWR);
        close(listenfd);
        for (int i=0; i < MAX_NODES; i++) {
            free(nodes[i]);
            free(ip[i]);
        }
        free(nodes);
        free(ip);
        freeClients(client_list);
        exit(0);
    }
    else if (signo == SIGCHLD) {
        int status;
        int pid;
        // To avoid zombie processes
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        }
        return;
    }
}

void chat_with_client(struct sockaddr_in client_addr, int sockfd) 
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
                printf("This IP Address %s is blocked.\n\n", client_ip);
                return;
            }
        }
    }

    printf("IP address is: %s\n", client_ip);
    printf("Port is: %d\n\n", (int) ntohs(client_addr.sin_port));

    for (;;) { 
        bzero(buff, MAX); 
  
        read(sockfd, buff, sizeof(buff)); 

        if (strncmp("exit", buff, 4) == 0) { 
            printf("Chat has ended\n\n"); 
            break; 
        } 
        
        if (buff[0] != '\0')
            printf("From client: %sTo client : ", buff); 
        else
            printf("To client : ");
        

        if (buff[0] != '\0' && buff[1] != '\0' && buff[0] == 'n' && (buff[1] == '1' || buff[1] == '2' || buff[1] == '3')) {
            bzero(buff, MAX);
            read(sockfd, buff, sizeof(buff)); 

            if (strncmp("exit", buff, 4) == 0) { 
                printf("Chat has ended\n\n"); 
                break; 
            } 
            
            if (buff[0] != '\0')
                printf("From client: %sTo client : ", buff); 
            else
                printf("To client : ");
        }

        bzero(buff, MAX); 
        n = 0; 

        while ((buff[n++] = getchar()) != '\n') 
            ; 
  
        buff[n] = '\n';
        buff[n+1] = '\0';

        if (strncmp(buff, "sendall", 7) == 0) {
            send_to_all(client_list, buff, strlen(buff));
            bzero(buff, MAX);
            continue;
        }
            
        write(sockfd, buff, sizeof(buff)); 
    } 
} 

void sendtoAllfds(int sd, char* buffer)
{
    for (int a=0; a<MAX_NODES; a++) {
        if (client_fds[a] != 0 && client_fds[a] != sd)
            send(client_fds[a], buffer, strlen(buffer), 0);
    }
}

pthread_mutex_t mutex;
int n=0;

void sendtoall(char *msg,int curr){
    int i;
    pthread_mutex_lock(&mutex);
    for(i = 0; i < n; i++) {
        if(send(client_fds[i],msg,strlen(msg),0) < 0) {
            printf("sending failure \n");
            continue;
        }
    }
    pthread_mutex_unlock(&mutex);
}

void *recvmg(void *client_sock){
    int sock = *((int *)client_sock);
    char msg[5000];
    char big_msg[8000];
    bzero(big_msg, sizeof(big_msg));
    int len;
    int counter = 0;
    while((len = recv(sock,msg,5000,0)) > 0) {
        msg[len] = '\0';
        printf("msg = %s\n", msg);
        if (strncmp(msg+2, "nodes", 5) == 0 ) {
            printClients(client_list);
            continue;
        }
        if (SEND_TO_FD != -1) {
            send(SEND_TO_FD, msg, strlen(msg), 0);
            SEND_TO_FD = -1;
            continue;
        }
        else if (SEND_TO_FD_STAR != -1) {
            if (counter > 0) {
                send(SEND_TO_FD_STAR, msg, strlen(msg), 0);
                counter --;
                continue;
            }
            SEND_TO_FD_STAR = -1;
            continue;
        }
        for (int i=0; i<curr; i++) {
            if (strncmp(nodes[i], msg+2, strlen(nodes[i])) == 0) {
                if (client_fds[i] != 0) {
                    send(client_fds[i], msg + 3 + strlen(nodes[i]), strlen(msg+2) - strlen(nodes[i]), 0);
                    SEND_TO_FD = sock;
                    break;
                } 
            }
        }

        if (strncmp(msg + 2, "n*.", 3) == 0 && strlen(msg + 2) > 3) {
            SEND_TO_FD_STAR = sock;
            sendtoall(msg + 5,sock);
            counter = n;
            bzero(big_msg, sizeof(big_msg));
        }       
    }

    struct sockaddr_in cliaddr;
    int length = sizeof(cliaddr);
    getpeername(sock, (struct sockaddr*)&cliaddr, (socklen_t*)&length);
    client_list = removeClient(client_list, sock);
    printf("Host disconnected , Socket fd is %d,  IP %s, PORT %d \n", sock, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
    return NULL;
}

int main(){
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
    sigaction(SIGCHLD, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);
    fclose(fp);

    struct sockaddr_in ServerIp, cliaddr;
    int len = sizeof(cliaddr);
    pthread_t recvt;
    int sock=0 , Client_sock=0;
    
    ServerIp.sin_family = AF_INET;
    ServerIp.sin_port = htons(PORT);
    ServerIp.sin_addr.s_addr = htonl(INADDR_ANY);
    sock = socket( AF_INET , SOCK_STREAM, 0 );
    if( bind( sock, (struct sockaddr *)&ServerIp, sizeof(ServerIp)) == -1 )
        printf("Error while binding to Socket\n");
    else
        printf("Server Started\n");
        
    if( listen( sock ,20 ) == -1 )
        printf("Failed to connect to Listening Socket\n");
    printf("Server listening on Port %d\n", PORT);
        
    while(1){
        if( (Client_sock = accept(sock, (struct sockaddr *)&cliaddr, &len)) < 0 )
            printf("Accept failed\n");
        printf("New Connection , Socket FD is %d, IP is : %s , PORT : %d\n", sock, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
        client_list = addClient(client_list, sock, cliaddr);
        pthread_mutex_lock(&mutex);
        client_fds[n]= Client_sock;
        n++;
        // creating a thread for each client 
        pthread_create(&recvt,NULL,(void *)recvmg,&Client_sock);
        pthread_mutex_unlock(&mutex);
    }
    return 0; 
    
}

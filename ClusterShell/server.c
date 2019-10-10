#define _GNU_SOURCE
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
int client_fds[MAX_NODES] = {0};
int listenfd;
int num_clients = 0;
int SEND_ALL = 0;
fd_set readfds;
int max_sd;

char shared_buffer[256], shared_users[256][256];
pthread_t thread_id;
pthread_mutex_t lock;


struct clients {
    int fd;
    int pid;
    struct sockaddr_in clientAddr;
    struct clients* next;
};

struct thread_args {
    int sock;
    int id;
};

void *client_thread(void*);

typedef struct clients clients;

clients* client_list = NULL;

clients* addClient(clients* head, int fd, int pid, struct sockaddr_in addr)
{
    if (head == NULL) {
        clients* node = (clients*) malloc (sizeof(clients));
        node->clientAddr = addr;
        node->pid = pid;
        node->fd = fd;
        node->next = NULL;
        return node;
    }

    clients* temp = head;
    clients* node = (clients*) malloc (sizeof(clients));
    node->clientAddr = addr;
    node->fd = fd;
    node->pid = pid;
    node->next = temp;
    head = node;
    return head;
}

clients* removeClient(clients* head, int removepid)
{
    clients* temp = head;
    if (temp == NULL)
        return NULL;
    if (temp->pid == removepid) {
        head = temp->next;
        temp->next = NULL;
        free(temp);
        return head;
    }
    if (temp->next == NULL) {
        if (temp->pid == removepid) {
            free(temp);
            return NULL;
        }
        return head;
    }

    while (temp->next) {
        if (temp->next->pid == removepid) { 
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
        printf("%d. fd = %d, pid = %d, IP = %s\n", i, temp->fd, temp->pid, ip_addr);
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
            // Remove the client from the list of clients
            client_list = removeClient(client_list, pid);
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
    sigaction(SIGCHLD, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);
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
    int sd;
    int activity;

    int i = 0;

    while (1) {
        connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
        if (connfd < 0)
            printf("ERROR on accept");

        // New Thread for client
        struct thread_args args;
        args.sock = connfd;
        args.id = i++;

        bzero(shared_users[args.id], 256);
        sprintf(shared_users[args.id], "client-%d", args.id);

        if (pthread_create(&thread_id, NULL, client_thread, (void*) &args)) {
            printf("Error creating thread");
        }

    }

    pthread_join(thread_id, NULL);
    pthread_mutex_destroy(&lock);
    close(listenfd);
    return 0;

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add the listening socket to the set 
        FD_SET(listenfd, &readfds);
        max_sd = listenfd;

        for (int i=0; i<MAX_NODES; i++) {
            sd = client_fds[i];
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }

        // Wait for activity on one of the sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR))
            printf("Select Error\n");

        if (FD_ISSET(listenfd, &readfds)) {
            connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
            if (connfd < 0) {
                printf("Server accept failed\n");
            }

            printf("New Connection , socket fd is %d, ip is : %s , port : %d\n", connfd, inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

            if (send(connfd, "Hello Client!", 13, 0) != 13)
                perror("send");
            printf("Welcome message sent successfully!\n");

            for (int i=0; i<MAX_NODES; i++) {
                if (client_fds[i] == 0) {
                    client_fds[i] = connfd;
                    printf("Adding to list of sockets as %d\n\n", i);
                    break;
                }
            }

        }
        int valread;
        char buffer[1025];

        for (int i=0; i<MAX_NODES; i++) {
            sd = client_fds[i];
            if (FD_ISSET(sd, &readfds)) {
                if ((valread = read(sd, buffer, 1024)) == 0) {
                    getpeername(sd, (struct sockaddr*)&cliaddr, (socklen_t*)&len);
                    printf("Host Disconnected , ip %s , port %d \n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

                    close(sd);
                    client_fds[i] = 0;
                }

                else {
                    buffer[valread] = '\0';
                    if (strncmp(buffer, "sendall", 7) == 0) {
                        for (int a=0; a<MAX_NODES; a++) {
                            if (client_fds[a] != 0 && client_fds[a] != sd)
                                send(client_fds[a], buffer, strlen(buffer), 0);
                        }
                    }
                    send(sd, buffer, strlen(buffer), 0);
                }
            }
            
        
        }
    }

    for (int i=0; i < MAX_NODES; i++) {
        free(nodes[i]);
        free(ip[i]);
    }
    free(nodes);
    free(ip);

    return 0;
}


void *client_thread(void* args_ptr)
{
    struct thread_args* args = (struct thread_args*) args_ptr;
    int sock, id, n;
    sock = args->sock;
    id = args->id;

    char buffer[256], message[256], msg_token;

    pthread_mutex_lock(&lock);
    bzero(shared_users[id], 256);
    sprintf(shared_users[id], "client-%d", id);
    pthread_mutex_unlock(&lock);

    bzero(buffer, 256);
    sprintf(buffer, "Welcome to the Server");
    n = write(sock, buffer, 256);
    if (n < 0) return NULL;
    
    /* write new connections to shared buffer*/
    printf("%s has joined\n", shared_users[id]);

    pthread_mutex_lock(&lock);
    bzero(shared_buffer, 256);
    sprintf(shared_buffer, "%s has joined", shared_users[id]);
    pthread_mutex_unlock(&lock);

    while (1) {
        /* read new message into buffer */
        bzero(buffer, 256);
        n = recv(sock, buffer, 256-1, MSG_DONTWAIT); /* non-blocking read */
        if (n < 0) {
            /* write current message back to client */
            n = write(sock, shared_buffer, strlen(shared_buffer));
            if (n < 0) {
                /* if we can't write to the client that means they
                 * have disconnected from the server */
                printf("%s has disconnected\n", shared_users[id]);

                /* let other users know of disconnection */
                pthread_mutex_lock(&lock);
                bzero(shared_buffer, 256);
                sprintf(shared_buffer, "%s has disconnected", shared_users[id]);
                pthread_mutex_unlock(&lock);
                break;
            }
        } else {
            msg_token = buffer[0];
            /* write new message to shared_buffer */
            printf("%s: %s", shared_users[id], buffer);

            pthread_mutex_lock(&lock);
            bzero(shared_buffer, 256);
            sprintf(shared_buffer, "%s: %s", shared_users[id], message);
            pthread_mutex_unlock(&lock);
        }

        /* don't bombard the client */
        sleep(2);
    }
    printf("%s has exited\n", shared_users[id]);
    /* zero out username If client does not exit cleanly (telnet q)
     * then username may not get removed */
    pthread_mutex_lock(&lock);
    memset(shared_users[id], 0, 256);
    pthread_mutex_unlock(&lock);

    close(sock);
    return NULL;
}

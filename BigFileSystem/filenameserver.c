#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<signal.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/time.h>
#include<sys/stat.h>
#include<semaphore.h>
#include<stdbool.h>

#define BUFSIZE 8192
#define BLOCK_SIZE 10*1024
#define NUM_SERVERS 2

struct TreeNode {
    char* value;
    bool isFile;
    struct TreeNode* parent;
    struct TreeNode* firstSon;
    struct TreeNode* nextSibling;
}Tree;

typedef struct TreeNode TreeNode;

TreeNode* init_treenode(char* val, bool isFile) {
    TreeNode* treenode = (TreeNode*) malloc (sizeof(TreeNode));
    strcpy(treenode->value, val);
    treenode->isFile = isFile;
    treenode->parent = treenode->firstSon = treenode->nextSibling = NULL;
    return treenode;
}

struct FileTree {
    TreeNode* root;
}Filetree;

typedef struct FileTree FileTree;

TreeNode* root_node;
char msg_tok[1024];
char ABS_DIR[1024];
struct stat st = {0};

FileTree* init_filetree() {
    root_node = init_treenode("/", false);
    FileTree* file_tree = malloc (sizeof(FileTree));
    file_tree->root = root_node;
    return file_tree;
}

char** path_split(char* path, char delim) {
    char** op = calloc (sizeof(msg_tok), sizeof(char*));
    for (int i=0; i<256; i++)
        op[i] = calloc (sizeof(msg_tok), sizeof(char));
    int j = 0;
    int k = 0;
    for (int i=0 ; path[i]!='\0'; i++) {
        if (path[i] == delim) {
            op[j++][k] = '\0';
            k = 0;
        }
        else {
            op[j][k++] = path[i];
        }
    }
    op[j] = NULL;
    return op;
}

bool find_node(char* path, TreeNode** last_node) {
    char** path_dirs = path_split(path, '/');
    TreeNode* node = root_node->firstSon;
    *last_node = root_node;
    for (int i=0; path_dirs[i] != NULL; i++) {
        char* name = path_dirs[i];
        while (node && strcmp(node->value, name) != 0)
            node = node->nextSibling;
        if (!node)
            return false;
        *last_node = node;
        node = node->firstSon;
    }
    return true && node->isFile;
}

bool insert_node(char* path, bool isFile) {
    TreeNode* parent = NULL;
    bool isFound = find_node(path, &parent);
    if (isFound)
        return false;
    char** path_dirs = path_split(path, '/');
    TreeNode* newNode = init_treenode(path, isFile);
    newNode->parent = parent;
    TreeNode* child = parent->firstSon;
    if (!child)
        parent->firstSon = newNode;
    else {
        while(child->nextSibling)
            child = child->nextSibling;
        child->nextSibling = newNode;
    }
    while(child)
        child = child->nextSibling;
    child = newNode;
    if (!isFile && stat(strcat(ABS_DIR, path), &st) == -1)
        mkdir(strcat(ABS_DIR, path), 0700);
    return true;
}

void list_node(TreeNode* node) {
    if (node) {
        printf("%s\n", node->value);
        list_node(node->firstSon);
        list_node(node->nextSibling);
    }
}

void file_tree_list() {
    list_node(root_node);
}

unsigned long hash(char *str)
{
    unsigned long hash = 5381;
    int c;
    while (c = *str++)
        hash = ((hash << 5) + hash) + c;
    return hash;
}

int get_file_size(int fd) {
   struct stat s;
   if (fstat(fd, &s) == -1) {
      int saveErrno = errno;
      fprintf(stderr, "fstat(%d) returned errno=%d.", fd, saveErrno);
      return -1;
   }
   return s.st_size;
}

struct inode_bigfs{
    unsigned long id;
    void* data;
    int num_blocks;
    struct inode_nigfs* next;
    bool last_node;
}inode_bigfs;

typedef struct inode_bigfs inode;

inode** init_inode_readonly(char* filename)
{
    // Is this supposed to be O_CREAT???
    int fd = open(filename, O_RDONLY);
    int file_size = get_file_size(fd);
    char buffer[2*BLOCK_SIZE];
    memset(&buffer, 0, sizeof(buffer));
    int curr = 0;
    while (curr < file_size/BLOCK_SIZE) {
        size_t b_read = read(fd, buffer, BLOCK_SIZE);
        if (b_read > 0 && b_read < BLOCK_SIZE) {
            printf("Buffer = %s\n\n", buffer);
            printf("EOF is reached in %lu bytes\n", BLOCK_SIZE - b_read);
            break;
        }
        else if (b_read == 0) {
            printf("Reached EOF\n");
            break;
        }
        else if (b_read == BLOCK_SIZE) {
            printf("Buffer = %s\n\n", buffer);
        }
        curr++;
    }
    int total_blocks = (file_size / BLOCK_SIZE) + 1;
    int max_blocks_per_server = (total_blocks / NUM_SERVERS) + 1;
    inode** head = calloc(NUM_SERVERS, sizeof(inode*));
    unsigned long hash_id = hash(filename);
    for (int i=0; i<max_blocks_per_server; i++) {
        head[i] = calloc(1, sizeof(inode));
        head[i]->id = hash_id;
    }
    close(fd);
    return head;
}

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

void error(char *msg) {
  perror(msg);
  exit(1);
}

void init_dirs_per_node() {
    char inode[100];
    char orig_dir[256];
    strcpy(orig_dir, ABS_DIR);
    for (int i=1; i<=NUM_SERVERS; i++) {
        bzero(inode, sizeof(inode));
        sprintf(inode, "/node%d", i);
        char* dir = strcat(ABS_DIR, inode);
        if (stat(dir, &st) == -1)
            mkdir(dir, 0700);
        strcpy(ABS_DIR, orig_dir);
    }
}

int main(int argc, char **argv) {
    int connectcnt = 0;
    int parentfd; /* parent socket */
    int childfd; /* child socket */
    int portno; /* port to listen on */
    int clientlen; /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    char buf[BUFSIZE]; /* message buffer */
    int optval; /* flag value for setsockopt */
    int n; /* message byte size */
    int notdone;
    fd_set readfds;
    int client_fds[1024] = {0};

    if (!getcwd(ABS_DIR, sizeof(ABS_DIR))) {
        perror("getcwd()");
    }

    if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
    }
    portno = atoi(argv[1]);
    init_dirs_per_node();

    parentfd = socket(AF_INET, SOCK_STREAM, 0);
    if (parentfd < 0) 
    error("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets 
    * us rerun the server immediately after we kill it; 
    * otherwise we have to wait about 20 secs. 
    * Eliminates "ERROR on binding: Address already in use" error. 
    */
    optval = 1;
    setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
         (const void *)&optval , sizeof(int));

    /*
    * build the server's Internet address
    */
    bzero((char *) &serveraddr, sizeof(serveraddr));

    /* this is an Internet address */
    serveraddr.sin_family = AF_INET;

    /* let the system figure out our IP address */
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* this is the port we will listen on */
    serveraddr.sin_port = htons((unsigned short)portno);

    /* 
    * bind: associate the parent socket with a port 
    */
    if (bind(parentfd, (struct sockaddr *) &serveraddr, 
       sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

    /* 
    * listen: make this socket ready to accept connection requests 
    */
    if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */ 
    error("ERROR on listen");

    connectcnt++;

    clientlen = sizeof(clientaddr);
    notdone = 1;
    connectcnt = 0;
    int sd = 0;
    int max_fd = 0;
    printf("server> ");
    fflush(stdout);

    while (notdone) {

    /* 
     * select: Has the user typed something to stdin or 
     * has a connection request arrived?
     */
    FD_ZERO(&readfds);          /* initialize the fd set */
    FD_SET(parentfd, &readfds); /* add socket fd */
    FD_SET(0, &readfds);        /* add stdin fd (0) */
    max_fd = parentfd;
    for (int i=0; i<connectcnt; i++) {
        if ((sd = client_fds[i]) > 0)
            FD_SET(sd, &readfds);
        if (sd > max_fd)
            max_fd = sd;
    }
        if (select(max_fd+1, &readfds, NULL, NULL, NULL) < 0 && (errno != EINTR)) {
          error("ERROR in select");
        }

        /* if the user has entered a command, process it */
        if (FD_ISSET(0, &readfds)) {
          fgets(buf, BUFSIZE, stdin);
          if (strncmp(buf, "count", 5) == 0) {
            printf("Received %d connection requests so far.\n", connectcnt);
            printf("server> ");
            fflush(stdout);
          }
          else if (strncmp(buf, "exit", 4) == 0) {
            notdone = 0;
          }
          else {
            printf("ERROR: unknown command\n");
            printf("server> ");
            fflush(stdout);
          }
        }    

        if (FD_ISSET(parentfd, &readfds)) {
          childfd = accept(parentfd, 
                   (struct sockaddr *) &clientaddr, &clientlen);
          if (childfd < 0) 
        error("ERROR on accept");
          connectcnt++;
        printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , childfd , inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
          for (int i=0; i<connectcnt; i++) {
              if (client_fds[i] == 0) {
              client_fds[i] = childfd;
              break;
              }
          }
          
        bzero(buf, BUFSIZE);
        n = recv(childfd, buf, BUFSIZE, 0);
        if (n < 0)
            error("ERROR reading from socket");
        printf("%s\n", buf);
      
      /* 
       * write: echo the input string back to the client 
       */
        memset(&msg_tok, 0, sizeof(msg_tok));
        int i = 0;
        for (i=0; buf[i] && buf[i]!=':'; i++) {
        }
        strcpy(msg_tok, buf + i + 1);
        char** op = split_string(" ");
        if (op[0] && strcmp(op[0], "cp") == 0) {
            if (op[1] && op[2] && !op[3]) {
                printf("Copying %s to %s\n", op[1], op[2]);
                // For every server, read BLOCK_SIZE bytes and then
                // seek to BLOCK_SIZE*(NUM_SERVERS-1) location from 
                // current offset position
                // Do sequential writes per server, but do all r/w
                // operations parallely 
                insert_node(op[2], true);
            }
            else {
                printf("Format: cp <src> <dst>\n");
                continue;
            }       
        }
        else if (op[0] && strcmp(op[0], "cat") == 0) {
            if (op[1]) {
                strcpy(buf, op[1]);
                n = send(childfd, buf, strlen(buf), 0);
                if (n < 0)
                    error("ERROR writing to socket");
                continue;
            }
            else {
                printf("Format: cat <filename>\n");
                continue;
            }
        }
      n = send(childfd, buf + 1, strlen(buf), 0);
      if (n < 0) 
        error("ERROR writing to socket");
    }
    else {
      for (int i=0; i<connectcnt; i++) {
          sd = client_fds[i];
          if (FD_ISSET(client_fds[i], &readfds)) {
              bzero(buf, BUFSIZE);
            if ((n = read(sd, buf, BUFSIZE)) == 0) {
                getpeername(sd, (struct sockaddr*)&clientaddr, (socklen_t*)&clientlen);
                printf("Host disconnected , ip %s , port %d \n" ,  
                      inet_ntoa(clientaddr.sin_addr) , ntohs(clientaddr.sin_port)); 
                close(sd);
                client_fds[i] = 0;
            }
            else {
                    if (n < 0) 
                      error("ERROR reading from socket");
                    printf("%s\n", buf);
                    n = send(client_fds[i], buf, strlen(buf), 0);
                    if (n < 0) 
                      error("ERROR writing to socket");
                    }
                }
            }
        }
    }
    /* clean up */
    printf("Terminating server.\n");
    close(parentfd);
    exit(0);
}

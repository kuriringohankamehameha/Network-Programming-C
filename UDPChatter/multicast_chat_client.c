#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#define BUFSIZE 8192
#define GROUP_NO "239.0.0.1"
#define TIMEOUT 30
#define MAX_SUBJECTS 20

int portno;
int recv_sockfd[MAX_SUBJECTS];
char client_id[256];
char send_msg[BUFSIZE];
struct ip_mreq mreq;
struct sockaddr_in addr;
pthread_t thread_id[MAX_SUBJECTS];
pthread_mutex_t lock;

// TODO: Add subject -> group mapping 
// and "!notify SubjectName"
char subject_list[MAX_SUBJECTS][256];
//char group_list[MAX_SUBJECTS][16];
char group_prefix[15] = "239.0.0.";
int num_groups = 1;
int num_threads = 1;
int tid = 0;


struct ThreadData {
    int fd;
    struct sockaddr_in address;
    size_t address_length;
    int tid;
}ThreadData;

void receiver_add_group(int sockfd, int group_num);

static void thread_exit_handler(int sig)
{ 
    pthread_exit(0);
}

static void signal_handler(int signo) {
    if (signo == SIGINT) {
        fprintf(stdout, "Terminating Client....\n");
        for (int i=0; i<num_threads; i++)
            pthread_cancel(thread_id[i]);
        exit(0);
    }
    else if (signo == SIGALRM) {
        return;
    }
    else if (signo == SIGUSR1) {
        //receiver_add_group(recv_sockfd[num_groups-1], num_groups-1);
        return;
    }
}

void init_subject_groups() {
    for (int i=0; i<MAX_SUBJECTS; i++) {
        subject_list[i][0] = '\0';
    }
}

void receiver_add_group(int sockfd, int group_num) {
    char group[15];
    sprintf(group, "%s%d", group_prefix, group_num);
    mreq.imr_multiaddr.s_addr = inet_addr(group);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt mreq");
        exit(1);
    }
}

void sender_add_group(int sockfd, int group_num) {
    char group[15];
    sprintf(group, "%s%d", group_prefix, group_num);
    addr.sin_addr.s_addr = inet_addr(group);
}

void* ThreadEntryPoint(void* params) {
    struct ThreadData* data = (struct ThreadData*)params;
    int tid = data->tid;
    char buffer[BUFSIZE];
    char recvline[BUFSIZE + 1];
    strcpy(recvline, buffer);
    int num_bytes;

    struct sockaddr_in addr;
    struct ip_mreq mreq;

    recv_sockfd[tid] = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (recv_sockfd[tid] < 0) {
        perror("Socket");
        exit(1);
    }

    // For allowing multiple processes to listen on the same TCP/UDP Port
    // on the same host machine
    uint32_t optval = 1;
    if (setsockopt(recv_sockfd[tid], SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
        perror("setsockopt reuseport");
        exit(1);
    }
    
    memset((char*)&addr, 0, sizeof(addr));
    char group[15];
    sprintf(group, "%s%d", group_prefix, tid);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(group);
    addr.sin_port = htons(portno);
    uint32_t addrlen = sizeof(addr);
    
    if (bind(recv_sockfd[tid], (struct sockaddr*) &addr, addrlen) < 0) {
        perror("bind");
        exit(1);
    }
        mreq.imr_multiaddr.s_addr = inet_addr(group);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);

        if (setsockopt(recv_sockfd[tid], IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            perror("setsockopt mreq");
            exit(1);
        }
    fd_set readfds; 
    FD_ZERO(&readfds);
    
    sigset_t set_empty, set_alrm;
    sigemptyset(&set_empty);
    sigemptyset(&set_alrm);
    sigaddset(&set_alrm, SIGALRM);
    signal(SIGALRM, signal_handler);
    signal(SIGUSR1, signal_handler);

    // Apply the signal mask on this set
    sigprocmask(SIG_BLOCK, &set_alrm, NULL);
    
    alarm(TIMEOUT);

    int pret;

    while (true) {
        FD_SET(recv_sockfd[tid], &readfds);
        int max_fd = 0;
        for (int i = 0; i<=tid; i++)
            if (max_fd < recv_sockfd[i])
                max_fd = recv_sockfd[i];
        pret = pselect(max_fd + 1, &readfds, NULL, NULL, NULL, &set_empty);
        if (pret < 0) {
            if (errno == EINTR)
                break;
            else {
                perror("pselect");
                break;
            }
        }
        else if (pret != 1) {
            perror("pselect");
            break;
        }
        // Now the mask is restored to the blocking mask
        if ((num_bytes = recvfrom(recv_sockfd[tid], recvline, BUFSIZE, 0, (struct sockaddr*) &addr, &addrlen)) < 0) {
            if (errno == EINTR) {
                fprintf(stderr, "Socket Timeout\n");
            }
            else {
                fprintf(stderr, "Recvfrom Error. Trying again....\n");
                exit(1);
                //continue;
            }
        }
        else {
            
            recvline[num_bytes] = 0;
            if (recvline[0] == '#') {
                // Check for subject header
                for (int i=0; recvline[i]; i++)
                    if (recvline[i] == '\n')
                        recvline[i] = '\0';
                for (int i=0; i<num_groups; i++) {
                    if (strcmp(subject_list[i], recvline+1) == 0)
                        break;
                    if (i == num_groups-1) {
                        num_groups++;
                        strcpy(subject_list[num_groups-1], recvline+1);
                        //pthread_create(&thread_id[num_groups-1], NULL, ThreadEntryPoint, NULL);
                        break;
                    }
                }
            }
            
            else {
                fprintf(stdout, "(%s) ", inet_ntoa(addr.sin_addr));
                fputs(recvline, stdout);
            }
        }
    }
    return NULL;
}

void add_group(int sock,
              const char* mc_addr_str,
              const char* imr_interface
              ) {
  struct sockaddr_in mc_addr;
  memset(&mc_addr, 0, sizeof(mc_addr));
  mc_addr.sin_family      = AF_INET;
  mc_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  mc_addr.sin_port        = htons(portno);

  // construct an IGMP join request structure
  struct ip_mreq mc_req;
  mc_req.imr_multiaddr.s_addr = inet_addr(mc_addr_str);
  //mc_req.imr_interface.s_addr = inet_addr(interface);
  mc_req.imr_interface.s_addr = INADDR_ANY;

  if ((setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                  (void*) &mc_req, sizeof(mc_req))) < 0) {
    perror("setsockopt() failed");
    exit(1);
  }

}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Format: %s <CLIENT_ID> <PORT_NO>\n", argv[0]);
        exit(1);
    }
    strcpy(client_id, argv[1]);
    portno = atoi(argv[2]);
    
    //struct sockaddr_in addr;
    struct ip_mreq mreq;
    size_t addrlen;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&addr, 0, sizeof(addr));

    // Send to any IP Address withinin the INET in the same port
    addr.sin_family = AF_INET;
    //addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(portno);
    addrlen = sizeof(addr);
    
    // Group Specifications for Sender
    //addr.sin_addr.s_addr = inet_addr(GROUP_NO);
    char group[15];
    for (int i=0; i < num_groups; i++) {
        sprintf(group, "%s%d", group_prefix, i);
        addr.sin_addr.s_addr = inet_addr(group);
    }

    int bytes_sent;

    struct ThreadData data = { sockfd , addr, sizeof(addr), tid };
    pthread_mutex_init(&lock, NULL);

    pthread_create(&thread_id[num_groups-1], NULL, ThreadEntryPoint, (void*)&data);
    tid++;

    signal(SIGINT, signal_handler);

    char subject[256];
    while (true) {
        time_t t;
        struct tm* tm_info;
        time(&t);
        tm_info = localtime(&t);
        char buffer[26];
        strftime(buffer, 26, "%H:%M", tm_info);
        sprintf(send_msg, "%s at %s: ", client_id, buffer);
        size_t size;
        char* line = NULL;
        bool no_send = false;
        bool msg_send = false;
        fprintf(stdout, "Chat > ");
        if (getline(&line, &size, stdin) != -1) {
            if (strcmp(line, "exit\n") == 0) {
                fprintf(stdout, "Exiting Chat...\n");
                break;
            }
            else if (strcmp(line, "\n") == 0) {
                continue;
            }
            else if (strncmp(line, "!subscribe ", 11) == 0) {
                int epos = -1;
                if (line[11] == '\"')
                {
                    for (int i=12; line[i] != '\n'; i++) {
                        if (i > 12 && line[i] == '\"' && line[i+1] == '\n') {
                            epos = i;
                            break;
                        }
                    } 
                    if (epos != -1) {
                        for (int i=12; i<epos; i++) {
                            subject[i-12] = line[i];
                        }
                        subject[epos-12] = '\0';
                        bool addtoList = true;
                        for (int i=0; subject_list[i][0]!='\0'; i++) {
                            if (strcmp(subject_list[i], subject) == 0) {
                                addtoList = false;
                                break;
                            }
                        }
                        if (addtoList) {
                            strcpy(subject_list[num_threads-1], subject);
                            //sender_add_group(sockfd, num_groups);
                            //receiver_add_group(sockfd, num_groups);
                            num_groups++;
                            num_threads++;
                            //pthread_kill(thread_id[num_groups-1], SIGUSR1);
                            sprintf(group, "%s%d", group_prefix, num_groups-1);
                            //add_group(sockfd, group, "0.0.0.0");
                            //add_group(recv_sockfd[num_threads-1], group, "0.0.0.0");
                            struct ThreadData data = { sockfd , addr, sizeof(addr), tid };
                            pthread_create(&thread_id[num_threads-1], NULL, ThreadEntryPoint, (void*)&data);
                            tid++;
                            no_send = true;
                            msg_send = true;
                            fprintf(stdout, "Subscribed to %s!\n", subject);
                        }
                    }
                }
            }
            else if (strncmp(line, "!sendto ", 8) == 0) {
                if (line[8] == '\"') {
                    int epos = -1;
                    for (int i=9; line[i]!='\n'; i++) {
                        if (i > 9 && line[i] == '\"' && line[i+1] == ' ') {
                            epos = i+1;
                            break;
                        }
                    } 
                    if (epos == -1)
                        strcat(send_msg, line);
                    else {
                        char subject[256];
                        strncpy(subject, line + 9, epos-1-9+1);
                        //printf("epos = %d\n", epos);
                        //printf("subject = %s\n", subject);
                        for (int i=0; i<num_threads-1; i++) {
                            //printf("i=%d, %s\t", i, subject_list[i]);
                            //printf("subject_list[i] == %s\n", subject_list[i]);
                            if (strncmp(subject_list[i], subject, strlen(subject_list[i])) == 0) {
                                // Send to the group
                                strcat(send_msg, line+epos+1);
                                bytes_sent = sendto(recv_sockfd[i+1], send_msg, sizeof(send_msg), 0, (struct sockaddr*)&addr, addrlen);
                                if (bytes_sent < 0) {
                                    fprintf(stderr, "Error in sendto()\n");
                                    for (int i=0; i<num_threads; i++)
                                        pthread_cancel(thread_id[i]);
                                    exit(1);
                                }
                                no_send = true;
                            }
                        }
                    }
                }
                else {
                    strcat(send_msg, line);
                }
            }
        else {
            strcat(send_msg, line);
        }
    }
        
        if (!no_send) {
            bytes_sent = sendto(sockfd, send_msg, sizeof(send_msg), 0, (struct sockaddr*)&addr, addrlen);
            if (bytes_sent < 0) {
                fprintf(stderr, "Error in sendto()\n");
                for (int i=0; i<num_threads; i++)
                    pthread_cancel(thread_id[i]);
                exit(1);
            }
        }
        if (msg_send) {
            char temp[256];
            // Construct the subject header
            //temp[0] = '#';
            //temp[1] = '\0';
            //strcat(temp, subject);
            memset(&temp, 0, sizeof(temp));
            temp[0] = '#';
            int i;
            for (i=0; subject[i]; i++)
                temp[i+1] = subject[i];
            // Linebreak necessary as the buffer is line buffered
            // Otherwise it will not reach via sendto() and 
            // remain in the buffer
            temp[i+1] = '\n';
            temp[i+2] = '\0'; 
            bytes_sent = sendto(sockfd, temp, sizeof(temp), 0, (struct sockaddr*)&addr, addrlen);
            if (bytes_sent < 0) {
                fprintf(stderr, "Error in sendto()\n");
                for (int i=0; i<num_threads; i++)
                    pthread_cancel(thread_id[i]);
                exit(1);
            }
        }
        
        // Do some other work..
    }
    
    for (int i=0; i<num_threads; i++)
        pthread_cancel(thread_id[i]);

    return 0;
}

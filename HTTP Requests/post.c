#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>

#define SA      struct sockaddr
#define MAXLINE 4096
#define MAXSUB  200


#define LISTENQ         1024

extern int h_errno;

ssize_t process_http(int sockfd, char *host, char *page, char *poststr)
{
	char sendline[MAXLINE + 1], recvline[MAXLINE + 1];
	char maxdata[MAXLINE + 1];
    ssize_t n;
    printf("Poststring = %s", poststr);
	snprintf(sendline, MAXSUB,
		 "POST %s HTTP/1.1\r\n"
		 "Host: %s\r\n"
         "User-Agent:Mozilla/5.0\r\n"
		 "Content-Type: application/x-www-form-urlencoded\r\n"
		 "Content-Length: %d\r\n\r\n"
		 "%s", page, host, strlen(poststr), poststr);

	write(sockfd, sendline, strlen(sendline));
	while ((n = read(sockfd, recvline, MAXLINE)) > 0) {
		recvline[n] = '\0';
		printf("%s", recvline);
	}
	return n;

}

long get_time() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec*1000;
}

int main(void)
{
	int sockfd;
	struct sockaddr_in servaddr;

	char **pptr;
	//********** You can change. Puy any values here *******
	char *hname = "fw.bits-pilani.ac.in";
	char *page = "/httpclient.html";
	char *poststr = "mode=191&username=f20170183&password=A7PS01839108#&a=%li&producttype=0\r\n";
	//*******************************************************

	char str[50];
	struct hostent *hptr;
	if ((hptr = gethostbyname(hname)) == NULL) {
		fprintf(stderr, " gethostbyname error for host: %s: %s",
			hname, hstrerror(h_errno));
		exit(1);
	}
	printf("hostname: %s\n", hptr->h_name);
	if (hptr->h_addrtype == AF_INET
	    && (pptr = hptr->h_addr_list) != NULL) {
		printf("address: %s\n",
		       inet_ntop(hptr->h_addrtype, *pptr, str,
				 sizeof(str)));
	} else {
		fprintf(stderr, "Error call inet_ntop \n");
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(8090);
	inet_pton(AF_INET, str, &servaddr.sin_addr);

	connect(sockfd, (SA *) & servaddr, sizeof(servaddr));
    char buffpost[MAXLINE + 1];
    sprintf(buffpost, poststr, get_time());
	process_http(sockfd, hname, page, buffpost);
	close(sockfd);
	exit(0);

}

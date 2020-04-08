#include<netinet/in.h>
#include<errno.h>
#include<netdb.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<netinet/ip_icmp.h>
#include<netinet/udp.h>
#include<netinet/tcp.h>
#include<netinet/ip.h>
#include<netinet/if_ether.h>
#include<net/ethernet.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/ioctl.h>
#include<sys/time.h>
#include<sys/types.h>
#include<unistd.h>
#include<pthread.h>
#include<signal.h>
 
int sock_raw, sock_eth;
FILE *logfile;
int tcp=0,udp=0,icmp=0,others=0,igmp=0,total=0,i,j;
struct sockaddr_in source,dest;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t thread;

void signal_handler(int signo) {
    if (signo == SIGINT) {
        pthread_cancel(thread);
        close(sock_raw);
        close(sock_eth);
        exit(0);
    }
}
 
void print_data (unsigned char* data, int Size)
{
    for(i=0 ; i < Size ; i++)
    {
        if( i!=0 && i%16==0)
        {
            fprintf(logfile,"         ");
            for(j=i-16 ; j<i ; j++)
            {
                if(data[j]>=32 && data[j]<=128)
                    fprintf(logfile,"%c",(unsigned char)data[j]);
                 
                else fprintf(logfile,".");
            }
            fprintf(logfile,"\n");
        } 
         
        if(i%16==0) fprintf(logfile,"   ");
            fprintf(logfile," %02X",(unsigned int)data[i]);
                 
        if( i==Size-1)
        {
            for(j=0;j<15-i%16;j++) fprintf(logfile,"   ");
             
            fprintf(logfile,"         ");
             
            for(j=i-i%16 ; j<=i ; j++)
            {
                if(data[j]>=32 && data[j]<=128) fprintf(logfile,"%c",(unsigned char)data[j]);
                else fprintf(logfile,".");
            }
            fprintf(logfile,"\n");
        }
    }
}

void print_ethernet_header(unsigned char* buffer)
{
    struct ethhdr *eth = (struct ethhdr *)buffer;
     
    fprintf(logfile , "\n");
    fprintf(logfile , "Ethernet Header\n");
    fprintf(logfile , "   |-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_dest[0] , eth->h_dest[1] , eth->h_dest[2] , eth->h_dest[3] , eth->h_dest[4] , eth->h_dest[5] );
    fprintf(logfile , "   |-Source Address      : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_source[0] , eth->h_source[1] , eth->h_source[2] , eth->h_source[3] , eth->h_source[4] , eth->h_source[5] );
    fprintf(logfile , "   |-Protocol            : %u \n",(unsigned short)eth->h_proto);
}

void print_ip_header(unsigned char* buffer)
{
    struct iphdr *iph = (struct iphdr *)buffer;
    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = iph->saddr;
     
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = iph->daddr;
     
    fprintf(logfile,"\n");
    fprintf(logfile,"IP Header\n");
    fprintf(logfile,"   |-Source IP        : %s\n",inet_ntoa(source.sin_addr));
    fprintf(logfile,"   |-Destination IP   : %s\n",inet_ntoa(dest.sin_addr));
    fprintf(logfile,"   |-TTL      : %d\n",(unsigned int)iph->ttl);
    fprintf(logfile,"   |-Protocol : %d\n",(unsigned int)iph->protocol);
}
 
void print_tcp_packet(unsigned char* buffer)
{
    unsigned short iphdrlen;
     
    struct iphdr *iph = (struct iphdr *)buffer;
    iphdrlen = iph->ihl*4;
     
    struct tcphdr *tcph=(struct tcphdr*)(buffer + iphdrlen);
             
    fprintf(logfile,"\n\n***********************TCP Packet*************************\n");    
         
    print_ip_header(buffer);
         
    fprintf(logfile,"\n");
    fprintf(logfile,"TCP Header\n");
    fprintf(logfile,"   |-Source Port      : %u\n",ntohs(tcph->source));
    fprintf(logfile,"   |-Destination Port : %u\n",ntohs(tcph->dest));
    fprintf(logfile,"   |-Sequence Number    : %u\n",ntohl(tcph->seq));
    fprintf(logfile,"   |- ACK : %d\n",(unsigned int)tcph->ack);
    fprintf(logfile,"   |- RST           : %d\n",(unsigned int)tcph->rst);
    fprintf(logfile,"   |- SYN     : %d\n",(unsigned int)tcph->syn);
    fprintf(logfile,"   |- FIN          : %d\n",(unsigned int)tcph->fin);
    fprintf(logfile,"   |- WINDOW         : %d\n",ntohs(tcph->window));
    fprintf(logfile,"\n");
    fprintf(logfile,"\n###########################################################");
}
 
void print_udp_packet(unsigned char *buffer)
{
     
    unsigned short iphdrlen;
     
    struct iphdr *iph = (struct iphdr *)buffer;
    iphdrlen = iph->ihl*4;
     
    struct udphdr *udph = (struct udphdr*)(buffer + iphdrlen);
     
    fprintf(logfile,"\n\n***********************UDP Packet*************************\n");
     
    print_ip_header(buffer); 
     
    fprintf(logfile,"\nUDP Header\n");
    fprintf(logfile,"   |-Source Port      : %d\n" , ntohs(udph->source));
    fprintf(logfile,"   |-Destination Port : %d\n" , ntohs(udph->dest));
     
    fprintf(logfile,"\n");
    fprintf(logfile,"IP Header\n");
    print_data(buffer , iphdrlen);
         
    fprintf(logfile,"UDP Header\n");
    print_data(buffer+iphdrlen , sizeof udph);
         
    fprintf(logfile,"\n###########################################################");
}
 
void print_icmp_packet(unsigned char* buffer)
{
    unsigned short iphdrlen;
     
    struct iphdr *iph = (struct iphdr *)buffer;
    iphdrlen = iph->ihl*4;
     
    struct icmphdr *icmph = (struct icmphdr *)(buffer + iphdrlen);
             
    fprintf(logfile,"\n\n***********************ICMP Packet*************************\n");   
     
    print_ip_header(buffer);
             
    fprintf(logfile,"\n");
         
    fprintf(logfile,"ICMP Header\n");
    fprintf(logfile,"   |-Type : %d",(unsigned int)(icmph->type));

    fprintf(logfile,"\n###########################################################");
}

void process_packet(unsigned char* buffer)
{
    //Get the IP Header part of this packet
    struct iphdr *iph = (struct iphdr*)buffer;
    ++total;
    switch (iph->protocol) //Check the Protocol and do accordingly...
    {
        case 1:  //ICMP
            ++icmp;
            print_icmp_packet(buffer);
            //PrintIcmpPacket(buffer,Size);
            break;
         
        case 2:  //IGMP
            print_ip_header(buffer);
            ++igmp;
            break;
         
        case 6:  //TCP
            ++tcp;
            print_tcp_packet(buffer);
            break;
         
        case 17: //UDP
            ++udp;
            print_udp_packet(buffer);
            break;
         
        default: //Some Other Protocol like ARP etc.
            print_ip_header(buffer);
            ++others;
            break;
    }
    printf("TCP : %d   UDP : %d   ICMP : %d   IGMP : %d   Others : %d   Total : %d\r",tcp,udp,icmp,igmp,others,total);
}

void* ThreadEntry() {
    int saddr_size , data_size;
    struct sockaddr saddr;
    unsigned char *buffer = (unsigned char *) malloc(65536);
    printf("Starting the Raw Ethernet Socket for MAC Addresses...\n");
     
    sock_eth = socket(AF_PACKET ,SOCK_RAW ,htons(ETH_P_ALL));
    if (sock_eth < 0) {
        perror("Socket error");
        exit(1);
    }
    while(1)
    {
        saddr_size = sizeof saddr;
        //Receive a packet
        data_size = recvfrom(sock_eth , buffer , 65536 , 0 , &saddr , (socklen_t*)&saddr_size);
        if(data_size <0 )
        {
            printf("Recvfrom error , failed to get packets\n");
            continue;
        }
        //Now process the packet
        pthread_mutex_lock(&mutex);
        print_ethernet_header(buffer);
        process_packet(buffer);
        pthread_mutex_unlock(&mutex);
    }
    close(sock_eth);
    return NULL;
}
 
int main()
{
    int saddr_size , data_size;
    struct sockaddr saddr;
    int BUFSIZE = 65536;
    unsigned char *buffer = (unsigned char *)malloc(BUFSIZE);

    pthread_create(&thread, NULL, ThreadEntry, NULL);
     
    logfile=fopen("log.txt","w");
    if(logfile==NULL) printf("Unable to create file.");
    printf("Starting the Raw Socket...\n");
    //Create a raw socket that shall sniff
    sock_raw = socket(AF_INET , SOCK_RAW , IPPROTO_TCP);
    if(sock_raw < 0)
    {
        printf("Socket Error\n");
        return 1;
    }
    while(1)
    {
        saddr_size = sizeof saddr;
        //Receive a packet
        data_size = recvfrom(sock_raw , buffer , 65536 , 0 , &saddr , &saddr_size);
        if(data_size <0 )
        {
            printf("Recvfrom error , failed to get packets\n");
            return 1;
        }
        //Now process the packet
        pthread_mutex_lock(&mutex);
        process_packet(buffer);
        pthread_mutex_unlock(&mutex);
    }
    close(sock_raw);
    printf("Done.\n");
    return 0;
}
 

#include<stdio.h>
#include <sys/time.h>

int main() {
    char* str = "Hello %s %llu\n";
    char buff[100];
    sprintf(buff, str, "Vijay", 285212672);
    printf("%s\n",buff);
    struct timeval now;
    gettimeofday(&now, NULL);
    printf("Time = %li\n", now.tv_sec*1000);
    return 0;
}

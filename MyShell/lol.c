#include<stdio.h>
#include<fcntl.h>

int main()
{
    FILE* ls_cmd = popen("ls -l", "r");
    
    int SIZE = 2550;
    char buffer[SIZE];

    size_t n;
    while((n = fread(buffer, 1, sizeof(buffer)-1, ls_cmd)) > 0)
    {
        buffer[n] = '\0';
    }

    pclose(ls_cmd);
    
    printf("buffer = %s\n", buffer);
    return 0;
}

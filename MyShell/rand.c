#include "shell.h"

#define MAX_NUM_PATHS 100

int main(int argc, char* argv[])
{
    char c;
    char* buf = NULL;
    size_t n = 0;
    
    printf("bash:$ ");
    while((c = getline(&buf, &n, stdin)))
    {
        for(int i=0; buf[i]!='\0'; i++)
            if(buf[i] == '\n')
                buf[i] = '\0';
        
        if(strcmp(buf, "exit") == 0)
            break;

        int pid = fork();
        if(pid == 0)
        {
            //Inside Child Process
            //arg is a NULL terminated Array of Strings
            int curr = 0;
            char* arg[] = {"python", NULL};
            if(execv("/bin/python", arg) < 0)
            {
                perror("Error");
                exit(-1);
            }
        }
        else
        {
            //Parent waits for Child Process to terminate and then show the prompt
            wait(NULL);
            printf("bash:$ ");
        }
    }
    return 0;
}

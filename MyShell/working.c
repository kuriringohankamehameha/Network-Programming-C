#include "shell.h"

int compare_strs(int size, char* s1, char* s2)
{
    if(size-1 != strlen(s2))
        return 0;

    for(int i=0; i<size-1; i++)
    {
        if(s1[i] != s2[i])
            return 0;
    }

    return 1;
}

char* str_concat(char* s1, char* s2)
{
    int l1 = strlen(s1);
    int l2 = strlen(s2);
    char* op = (char*) malloc ((l1 + l2)*sizeof(char));
    for(int i=0; i<l1 + l2; i++)
    {
        if (i < l1)
            op[i] = s1[i];
        else
            op[i] = s2[i-l1];
    }
    return op;
}

int main(int argc, char* argv[])
{
    char c;
    //char* buf = (char*) malloc (sizeof(char));
    char* buf = NULL;
    size_t n = 0;
    char* exit_str = "exit"; 
    while((c = getline(&buf, &n, stdin)))
    {
        
        if(compare_strs(c, buf, exit_str) == 1)
        {
            free(buf);
            exit(0);
        }

        int pid = fork();

        if(pid == 0)
        {
            char* op;
            
            for(int i=0; buf[i]!='\0'; i++)
                if(buf[i] == '\n')
                    buf[i] = '\0';
           
            int MAX_ARG_LENGTH = 50;
            char** arg = (char**) malloc (MAX_ARG_LENGTH*sizeof(char*));
            for(int i=0; i<MAX_ARG_LENGTH; i++)
            {
                arg[i] = (char*) malloc (70*sizeof(char));
            }

            int count = 0;
            int j=0;
            for(int i=0; buf[i]!='\0'; i++)
            {
                if(buf[i] == ' ')
                {
                    arg[count][j] = '\0';
                    j = 0;
                    count ++;
                }

                else
                {
                    arg[count][j++] = buf[i];
                }
            }
            
            arg[count][j] = '\0';
            arg[count+1] = NULL;

            execv(op=str_concat("/bin/",arg[0]), arg);
            printf("Executed %s\n", op);
            free(op);
            for(int i=0; i<MAX_ARG_LENGTH; i++)
            {
                free(arg[i]);
            }
            free(arg);
            perror("Error:");
            exit(0);
        }

        //Otherwise, set the fork() and set the child pid to a pgroup
        wait(NULL);
    }

    free(buf);

    return 0;
}

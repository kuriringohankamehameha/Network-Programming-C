#include "shell.h"

int compare_strs(char* s1, char* s2)
{
    if(strlen(s1) != strlen(s2))
        return 0;

    for(int i=0; i<strlen(s1); i++)
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

int get_args(char* s)
{
    int c = 0;
    for(int i=0; s[i]!='\0'; i++)
    {
        if(s[i] == ' ')
        {
            c++;
        }
    }

    return c;
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
        
        for(int i=0; buf[i]!='\0'; i++)
            if(buf[i] == '\n')
                buf[i] = '\0';

        if(compare_strs(buf, exit_str) == 1)
        {
            free(buf);
            exit(0);
        }

        int pid = fork();

        if(pid == 0)
        {
            char* op;
           
            int ARG_LENGTH = get_args(buf) + 1;
            char** arg = (char**) malloc (ARG_LENGTH*sizeof(char*));
            for(int i=0; i<ARG_LENGTH; i++)
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
            free(op);
            for(int i=0; i<ARG_LENGTH; i++)
            {
                free(arg[i]);
            }
            free(arg);
            perror("Error:");
            exit(0);
        }

        wait(NULL);
    }

    free(buf);

    return 0;
}

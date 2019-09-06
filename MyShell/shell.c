#include "shell.h"

#define MAX_NUM_PATHS 100

char** PATH;
int PATH_LENGTH;

//Current Child PID
int CURR_CHILD_PID = -1;
int PARENT_PID;

//Shell Signal Handler
static void shell_handler(int, siginfo_t*, void*);

static void shell_handler(int signo, siginfo_t* info, void* context)
{
    //Pass the signals to the process running in the shell
    if(signo == SIGINT)
    {
        if (CURR_CHILD_PID > 0)
        {
            kill(CURR_CHILD_PID, signo);
            kill(PARENT_PID, SIGCHLD);
        }
        else
        {
            signal(signo, SIG_IGN);
            printf("\n");
            printf("myShell~:$ ");
        }
    }
}

void initialize_path(const char* filename)
{
    //Sets the PATH from filename (the Shell config file)
    PATH = (char**) malloc (MAX_NUM_PATHS*sizeof(char*));
    for(int i=0; i<MAX_NUM_PATHS; i++)
        PATH[i] = (char*) malloc (sizeof(char));

    char buffer[256];
    int k = 0;
    int j = 0;
    FILE* file = fopen(filename, "r");
    
    if (!file)
    {
        printf("Error opening file '%s'\n", filename);
        exit(1);
    }

    while(fgets(buffer, sizeof(buffer), file))
    {
        //printf("buffer = %s",buffer);
        for(int i=0; ; i++)
        {
            if(buffer[i] == '\n')
            {
                PATH[j][k] = '\0';
                break;
            }

            else
                PATH[j][k++] = buffer[i];
        }

        j++;
        k = 0;
    }

    PATH_LENGTH = j;
}

int compare_strs(char* s1, char* s2)
{
    if(strlen(s1) != strlen(s2))
        return 0;

    for(int i=0; i<strlen(s1); i++)
        if(s1[i] != s2[i])
            return 0;
    return 1;
}

char* str_concat(char* s1, char* s2)
{
    int l1 = strlen(s1);
    int l2 = strlen(s2);
    char* op = (char*) malloc ((l1 + l2)*sizeof(char));
    for(int i=0; i<l1 + l2; i++)
        if (i < l1)
            op[i] = s1[i];
        else
            op[i] = s2[i-l1];
    return op;
}

int get_args(char* s)
{
    int c = 0;
    for(int i=0; s[i]!='\0'; i++)
        if(s[i] == ' ')
            c++;
    return c;
}

void clear_terminal()
{
    for(int i=0; i<200; i++)
        printf("\n");
}

int custom_command(char* command)
{
    if(strcmp(command, "clear") == 0)
    {
       clear_terminal();    
       return 1;
    }

    return 0;
}

int main(int argc, char* argv[])
{
    //Set the PATH first
    initialize_path(".myshellrc");

    char c;
    char* buf = NULL;
    size_t n = 0;
    char* exit_str = "exit"; 
    
    printf("myShell~:$ ");

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGKILL);

    struct sigaction sa;
    sa.sa_handler = shell_handler;
    sa.sa_flags = SA_SIGINFO;

    sigprocmask(SIG_BLOCK, &set, NULL);
    //sigaction(SIGINT, &sa, NULL);

    int pipefd[2];
    pipe(pipefd);

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
        
        if(custom_command(buf) == 1)
        {
            printf("myShell~:$ ");
        }
        
        else
        {
            int pid = fork();
            PARENT_PID = getpid();

            int ARG_LENGTH = get_args(buf) + 1;
            char** arg = (char**) malloc (ARG_LENGTH*sizeof(char*));
            for(int i=0; i<ARG_LENGTH; i++)
                arg[i] = (char*) malloc (70*sizeof(char));
            
            if(pid == 0)
            {
                //Inside Child Process
                sigprocmask(SIG_UNBLOCK, &set, NULL);
                sigaction(SIGINT, &sa, NULL);
                CURR_CHILD_PID = getpid();

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
                        arg[count][j++] = buf[i];
                }
                
                arg[count][j] = '\0';
                arg[count+1] = NULL;

                //Now, arg is a NULL terminated Array of Strings
                int curr = 0;

                while(curr < PATH_LENGTH)
                {
                    if(execv(str_concat(PATH[curr], arg[0]), arg) < 0)
                    {
                        //Try all possible outcomes in PATH until it executes
                        curr ++;
                        if(curr == PATH_LENGTH)
                        {
                            perror("Error");
                            exit(0);
                        }
                    }
                }
                for(int i=0; i<ARG_LENGTH; i++)
                {
                    free(arg[i]);
                }
                free(arg);
                perror("Error:");
                exit(0);
            }
            else
            {
                wait(NULL);

                for(int i=0; i<ARG_LENGTH; i++)
                    free(arg[i]);
                free(arg);

                printf("myShell~:$ ");
                CURR_CHILD_PID = -1;
            }
        }
    }

    free(buf);
    return 0;
}

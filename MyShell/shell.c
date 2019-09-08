#include "shell.h"

#define MAX_NUM_PATHS 100

char** PATH;
int PATH_LENGTH;

//Current Child PID
int CURR_CHILD_PID = -1;
int PARENT_PID;
int background_proc = 0;
int fg_command = 0;
int bg_command = 0;
int bg_to_fg = 0;

static int* background_jobs;
static int bc = 0;

int cpipe[2];

//Shell Signal Handler
static void shell_handler(int, siginfo_t*, void*);
static inline void print_prompt();

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
            print_prompt();
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
    
    else if(strcmp(command, "fg") == 0)
    {
        if(bc == 0)
        {
            printf("No background jobs running now!\n");
            return 1;
        }

        //Move the background process to the foreground pgrp
        pid_t bgpgrp = background_jobs[bc-1];
        //printf("background_jobs[bc-1] =  %d\n",background_jobs[bc-1]);
        
        kill(background_jobs[bc-1], SIGCONT);
        if(tcsetpgrp(0, background_jobs[bc-1]) < 0)
            perror("tcsetpgrp()");
        signal(SIGTTOU, SIG_IGN);
        tcsetpgrp(0, getpid());
        signal(SIGTTOU, SIG_DFL);
        return 1;

    }

    return 0;
}

static inline void print_prompt()
{    
    printf("myShell~: $ ");
}

static void pipeline(char* command1, char* command2, char** arg1, char** arg2)
{
    //Pass command from fromPID to toPID using pipes
    static int pipefd1[2];
    static int pipefd2[2];
    pipe(pipefd1);

    int childPID = fork();
    if(childPID == 0)
    {
        //Inside Child
        
        //dup2(pipefd1[0], STDIN_FILENO);
        dup2(pipefd1[1], STDOUT_FILENO);
        execv(command1, arg1);
    }

    else
    {
        //Inside Parent
        int secondPID = fork();
        if(secondPID == 0)
        {
            dup2(pipefd2[0], STDOUT_FILENO);
            execv(command2, arg2);
        }

        wait(NULL);
        wait(NULL);
    }
}

int main(int argc, char* argv[])
{
    //Set the PATH first
    initialize_path(".myshellrc");

    char c;
    char* buf = NULL;
    size_t n = 0;
    char* exit_str = "exit"; 
    
    print_prompt();

    //Add the list of Signals to be handled to the signal set
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGKILL);

    struct sigaction sa;
    sa.sa_handler = shell_handler;
    sa.sa_flags = SA_SIGINFO;

    //The parent must not receive signals
    sigprocmask(SIG_BLOCK, &set, NULL);

    int pipefd[2];
    pipe(pipefd);
    
    background_jobs = (int*) malloc (sizeof(int));

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
            print_prompt();
        }
        
        else
        {
            if(bc>0 && waitpid(background_jobs[bc-1], NULL, WNOHANG))
            {
                //printf("Bg returned\n");
                bc --;
            }

            int ARG_LENGTH = get_args(buf) + 1;
            char** arg = (char**) malloc (ARG_LENGTH*sizeof(char*));
            for(int i=0; i<ARG_LENGTH; i++)
                arg[i] = (char*) malloc (70*sizeof(char));
            
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
            
            if(strcmp(arg[count], "&") == 0)
            {
                background_proc = 1;
                //Now remove & from the arg list
                arg[count] = NULL;
            }


            int pid = fork();

            if(pid == 0)
            {
                //Inside Child Process
                cpipe[2];
                pipe(cpipe);

                if(background_proc == 1)
                {
                    //Add the background_proc to a process group
                    printf("[bg]\t%d\n", getpid());
                    setpgid(0, 0);
                    
                    //signal(SIGTTOU, SIG_IGN);
                    //tcsetpgrp(0, getpgrp());
                    
                    dup2(cpipe[0], STDIN_FILENO);
                    
                    //close(cpipe[0]);
                    //kill(getpid(), SIGTTIN);
                }
                
                else
                {
                    //Add to foreground process group
                    pid_t cpgrp = getpgrp();
                    signal(SIGTTOU, SIG_IGN);
                    if(tcsetpgrp(STDIN_FILENO, cpgrp) < 0)
                    {   
                        perror("tcsetpgrp()");
                    }
                }

                //The child must receive the blocked signals
                sigprocmask(SIG_UNBLOCK, &set, NULL);
                sigaction(SIGINT, &sa, NULL);
                CURR_CHILD_PID = getpid();

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
                PARENT_PID = getpid();
                if(background_proc == 1)
                {
                    pid_t cpgrp = pid;
                    background_jobs[bc++] = cpgrp;
                    //signal(SIGTTIN, SIG_DFL);
                    signal(SIGTTOU, SIG_IGN);
                    //Make the parent control the Terminal
                    kill(pid, SIGCONT);
                    setpgrp();
                }
                else
                    waitpid(pid, NULL, 0);
                background_proc = 0;

                for(int i=0; i<ARG_LENGTH; i++)
                    free(arg[i]);
                free(arg);

                print_prompt();
                CURR_CHILD_PID = -1;
            }
        }
    }

    free(buf);
    return 0;
}

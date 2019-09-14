#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<signal.h>

int argLength = -1;
char** argVector;
int isBackground = 0;
int childPID = -1;
int parentPID = -1;

void printPrompt()
{
    printf("Shell~: $ ");
}

char* str_concat(char* s1, char* s2)
{
    int l1 = strlen(s1);
    int l2 = strlen(s2);
    char* op = (char*) calloc (l1 + l2, sizeof(char));
    for(int i=0; i<l1 + l2; i++)
        if (i < l1)
            op[i] = s1[i];
        else
            op[i] = s2[i-l1];
    return op;
}

static void shellHandler(int signo, siginfo_t* info, void* context)
{
    if (signo == SIGCHLD)
    {
        if (isBackground == 1)
            printf("[bg] finished\n");
        isBackground = 0;
    }

}

sigset_t createSignalSet(int* signalArray)
{
    sigset_t set;
    sigemptyset(&set);
    for(int i=0; signalArray[i]!=-1; i++)
        sigaddset(&set, signalArray[i]);
    return set; 
}

int getArgumentLength(char* command)
{
    int count = 1;
    for(int i=0; command[i] != '\0'; i++)
        if(command[i] == ' ')
            count ++;
    return count;
}

void parseCommand(char* buffer)
{
    argLength = getArgumentLength(buffer);
    argVector = (char**) malloc ((argLength+1)* sizeof(char*));
    for (int i=0; i <= argLength; i++)
        argVector[i] = (char*) calloc (100, sizeof(char));

    int count = 0;
    int j = 0;
    for (int i=0; buffer[i] != '\0'; i++)
    {
        if (buffer[i] == ' ')
        {
            argVector[count][j] = '\0';
            j = 0;
            count ++;
        }

        else
            argVector[count][j++] = buffer[i];
    }

    argVector[count][j] = '\0';
    argVector[count + 1] = 0;

    if (strcmp(argVector[count], "&") == 0)
    {
        isBackground = 1;
        argVector[count] = 0;
    }

}

char* lineget(void) {
    char * line = malloc(100), * linep = line;
    size_t lenmax = 100, len = lenmax;
    int c;

    if(line == NULL)
        return NULL;

    for(;;) {
        c = fgetc(stdin);
        if(c == EOF)
            break;

        if(--len == 0) {
            len = lenmax;
            char * linen = realloc(linep, lenmax *= 2);

            if(linen == NULL) {
                free(linep);
                return NULL;
            }
            line = linen + (line - linep);
            linep = linen;
        }

        if((*line++ = c) == '\n')
            break;
    }
    *line = '\0';
    return linep;
}

int main(int argc, char* argv[])
{
    int argLength = -1;
    argVector = NULL;

    int signalArray[] = {SIGINT, SIGKILL, SIGTSTP, -1};
    sigset_t set = createSignalSet(signalArray);
    struct sigaction sa = {0};
    sa.sa_handler = shellHandler;
    sa.sa_flags = SA_SIGINFO;

    sigprocmask(SIG_BLOCK, &set, NULL);

    printPrompt();
    
    //char* buffer = NULL;
    char* buffer = (char*) malloc (sizeof(char)); 
    size_t n = 0;
    sigaction(SIGCHLD, &sa, NULL);

    // Now start the event loop
    //while (getline(&buffer, &n, stdin))
    while((buffer=lineget()))
    {
        for (int i=0; buffer[i] != '\0'; i++)
            if (buffer[i] == '\n')
                buffer[i] = '\0';

        if (strcmp(buffer, "exit") == 0)
        {
            exit(0);
        }

        else
        {
            parseCommand(buffer);
            int pid = fork();
            childPID = pid;
            
            // Parent process waits to receive SIGCHLD to indicate background termination

            if (pid == 0)
            {
                // The child must receive the previously blocked signals
                sigprocmask(SIG_UNBLOCK, &set, NULL);
                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                signal(SIGTTIN, SIG_DFL);
                signal(SIGTTOU, SIG_DFL);
                signal(SIGCHLD, SIG_DFL);

                if (isBackground == 1)
                {
                    printf("[bg]  %d\n",getpid());
                    setpgid(0, 0);
                }

                else 
                {
                    // Add to foreground process group
                    pid_t cgrp = getpgrp();
                    setpgid(getpgrp(), getppid());
                }

                childPID = getpid();
                int curr = 0;
                if (execv(str_concat("/bin/", argVector[0]), argVector) < 0)
                {
                    perror("Error");
                    exit(0);
                }
            }

            else
            {
                parentPID = getpid();
                setpgid(0, 0);
                if (isBackground == 1)
                {
                    setpgrp();
                    int status;
                    if (waitpid (pid, &status, WNOHANG) > 0)
                        printf("[bg]  %d finished\n", pid);
                }

                else
                {
                    int status;
                    waitpid(pid, NULL, WUNTRACED);
                }
                printPrompt();
            }
        }
    }   
    return 0;
}

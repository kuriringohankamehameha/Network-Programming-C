#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<stdlib.h>
#include<pthread.h>
#include<string.h>
#include<sys/types.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<errno.h>

#define cl(size, type) (type*) calloc (size, sizeof(type))
#define ml(type) (type*) malloc (sizeof(type))
#define ml2(size, type) (type**) malloc (size * sizeof(type*))
#define ml3(size, type) (type***) malloc (size * sizeof(type**))

int pipefd[2];
int* fd; 
char** argVector;
int lastPipe = 0;
int pipeCount = 0;
int* pipe_cmds;
int p = 0;

// Tee Command : Pipes output from STDIN into it's arguments and STDOUT
// Example : ping google.com | tee file.txt | less

static void sigHandler(int signo, siginfo_t* info, void* context)
{

}

sigset_t createSignalSet(int* signalArray)
{
	sigset_t set;
	sigemptyset(&set);
	for(int i=0; signalArray[i]!=-1; i++)
		sigaddset(&set, signalArray[i]);
	return set;	
}

static inline void printPrompt()
{
	printf("myShell~: $ ");
}


void parseCommand(char* buffer)
{
	int j = 0;
	int k = 0;

	for (int i=0; buffer[i] != '\0'; i++)
	{
		if (buffer[i] == ' ' && buffer[i+1] == ' ')
		{
			continue;
		}

		else if (buffer[i] == ' ')
		{
			argVector[j][k++] = '\0';
			j++;
			k = 0;
		}

		else
		{
			argVector[j][k] = buffer[i];
			k++;
		}
	}

	argVector[j][k] = '\0';
	argVector[j+1] = NULL;
}

void executePipe(char ***cmd)
{
	int p[2];
	int pid;
	int fd_in = 0;
    int save_out = dup(STDOUT_FILENO);
    int flag = 0;
    int fd;

	while (*cmd != NULL)
	{
		pipe(p);

        if (*(cmd+1) != NULL && strcmp((*(cmd+1))[0], "tee") == 0)
        {
            flag = 1;
            for(int k=1; (*(cmd+1))[k]!=NULL; k++)
            {
                if (fork() == 0)
                {
                    fd = open((*(cmd+1))[k], O_WRONLY | O_CREAT, 0664);
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                    execvp((*cmd)[0], *cmd);
                }
                else
                {
                    wait(NULL);
                }
            }

            if (*(cmd+2) == NULL)
            {
                dup2(save_out, STDOUT_FILENO);
            }
            else
                dup2(p[1], STDOUT_FILENO);
        }
		if ((pid = fork()) == -1)
		{
			printf("Error\n");
			exit(1);
		}
		else if (pid == 0)
		{
			dup2(fd_in, 0);
			if(*(cmd + 1) != NULL)
			{
				dup2(p[1], 1);
			}
			close(p[0]);
            if (flag == 0)
            execvp((*cmd)[0], *cmd);
            else
            {
                dup2(save_out, STDOUT_FILENO);
                close(save_out);
                execvp(*(cmd[0]), *cmd);
            }
			exit(0);
		}
		else{
            if (flag == 1)
            {
                flag = 0;
                wait(NULL);
                close(p[1]);
                fd_in = p[0];
                cmd += 2;
            }
            else
            {
                wait(NULL);
                close(p[1]);
                fd_in = p[0];
                cmd++;
            }
		}
	}
}

void execRedirect(char** cmd1, char** cmd2, int redirection)
{
    // Check sizes of commmands
    int count1 = 0;
    int count2 = 0;
    for (int i=0; cmd1[i]!=NULL; i++)
        count1 ++;
    for (int i=0; cmd2[i]!=NULL; i++)
        count2 ++;

    if (redirection == 0)
    {
        // Redirection to output file >
        if (count2 > 1)
        {
            printf("Error : Can only redirect \'>\' to a File\n");
            return;
        }
        
        int fd = open(cmd2[0], O_WRONLY | O_CREAT, 0644);

        int pid = fork();
        int save_out = dup(STDOUT_FILENO);
        if (pid == 0)
        {
            dup2(fd, STDOUT_FILENO);
            close(fd);
            execvp(cmd1[0], cmd1);
            fprintf(stderr, "Failed to execute %s\n", cmd1[0]);
        }
        else{
            int status;
            waitpid(pid, &status, NULL);
            dup2(save_out, STDOUT_FILENO);
            close(save_out);
        }
    }
    else if (redirection == 1)
    {
        // <
        if (count2 > 1)
        {
            printf("Error : Can only redirect \'<\' to a File\n");
            return;
        }
        
        int fd = open(cmd2[0], O_WRONLY | O_CREAT, 0644);

        int pid = fork();
        int save_in = dup(STDIN_FILENO);
        if (pid == 0)
        {
            dup2(fd, STDIN_FILENO);
            close(fd);
            execvp(cmd1[0], cmd1);
            fprintf(stderr, "Failed to execute %s\n", cmd2[0]);
        }
        else{
            int status;
            waitpid(pid, &status, NULL);
            dup2(save_in, STDIN_FILENO);
            close(save_in);
        }
    }
}

int main(int argc, char *argv[]) {

	int signalArray[] = {SIGINT, SIGKILL, SIGTSTP, -1};
	sigset_t set = createSignalSet(signalArray);
	struct sigaction sa;
	sa.sa_handler = sigHandler;
	sa.sa_flags = SA_SIGINFO;

	sigprocmask(SIG_BLOCK, &set, NULL);

	char* buffer = NULL;
	size_t n = 0;

    fd = ml(int);
	
    argVector = (char**) calloc (100, sizeof(char*));
	for (int i=0; i<100; i++)
		argVector[i] = (char*) malloc (sizeof(char));
    
    pipe_cmds = (int*) malloc (sizeof(int));

	printPrompt();

	// Now start the event loop
	while (getline(&buffer, &n, stdin))
	{
		for (int i=0; buffer[i] != '\0'; i++)
			if (buffer[i] == '\n')
				buffer[i] = '\0';

		if (strcmp(buffer, "exit") == 0)
			break;

		parseCommand(buffer);

		// Starts here
		char*** commands = (char***) calloc (10, sizeof(char**));
	    for (int i=0; i<10; i++)
	    {
	        commands[i] = (char**) calloc (10, sizeof(char*));
	        for (int j=0; j<10; j++)
	        {
	            commands[i][j] = (char*) calloc (10, sizeof(char));
	        }
	    }

	    int a = 0;

		for (int i=0; argVector[i] != NULL; i++)
    	{
        	if (strcmp(argVector[i], "|") == 0)
        	{
            	if (lastPipe == 0)
            	{
                	for (a=0; a<i; a++)
                    	strcpy(commands[pipeCount][a], argVector[a]); 

                	// Make NULL terminated command
                	commands[pipeCount][i] = NULL;

	                // Update Pipe location
	                lastPipe = i;
                    pipe_cmds[p++] = lastPipe;
	                pipeCount++;
	            }

	            else
	            {
	                for (a = lastPipe+1; a<i; a++)
	                {
	                    strcpy(commands[pipeCount][a-lastPipe-1], argVector[a]);
	                }

	                // Make NULL terminated command
	                commands[pipeCount][i-lastPipe-1] = NULL;

	                // Update Pipe location
	                lastPipe = i;
                    pipe_cmds[p++] = lastPipe;
	                pipeCount++;
	           }

	       }
	    }

        pipe_cmds[p] = -1;
        
        if (pipeCount > 0)
        {
            for(a=lastPipe + 1; argVector[a]!=NULL; a++)
            {
                if (strcmp(argVector[a], "|") != 0)
                {
                    strcpy(commands[pipeCount][a-lastPipe-1], argVector[a]);
                }
            }
            commands[pipeCount][a-lastPipe-1] = NULL;
        }

	    // Now, I must have a NULL terminated array of Commands
	    commands[pipeCount+1][a] = NULL;
	    commands[pipeCount+1] = NULL;


		if (pipeCount > 0)
		{
            pipeCount = 0;
			executePipe(commands);
			//fork_pipes(pipeCount+1, cmd);
			printPrompt();
			continue;
		}
        
        int redirect = 0;

		char** redirectcmd1 = (char**) calloc (5, sizeof(char*));
		char** redirectcmd2 = (char**) calloc (5, sizeof(char*));
	    for (int i=0; i<10; i++)
	    {
            redirectcmd1[i] = (char*) malloc (sizeof(char));
            redirectcmd2[i] = (char*) malloc (sizeof(char));
	    }

        for (int i=0; argVector[i]!=NULL; i++)
        {
            if (strcmp(argVector[i], ">") == 0)
            {
                if (i <= lastPipe)
                {
                    printf("Error: Output Redirection before a pipe\n");
                    redirect = 1;
                    break;
                }
                
                for(int j = (lastPipe>0) ? lastPipe+1 : 0; j<i; j++)
                {
                    if (lastPipe > 0)
                        strcpy(redirectcmd1[j-lastPipe-1], argVector[j]); 
                    else
                        strcpy(redirectcmd1[j], argVector[j]);
                }
                if (lastPipe > 0)
                    redirectcmd1[i-lastPipe-1] = 0; 
                else
                    redirectcmd1[i] = 0;
                int j;
                for(j = i+1; argVector[j]!=NULL; j++)
                {
                    strcpy(redirectcmd2[j-i-1], argVector[j]);
                }
                redirectcmd2[j-i-1] = 0;
                execRedirect(redirectcmd1, redirectcmd2, 0);
                redirect = 1;
                break;
            }
            
            else if (strcmp(argVector[i], "<") == 0)
            {
                for(int j = 0; j<(i <= lastPipe) ? i : lastPipe; j++)
                {
                    strcpy(redirectcmd1[j], argVector[j]); 
                }
                if (lastPipe >= i)
                    redirectcmd1[i] = 0; 
                else
                    redirectcmd1[lastPipe] = 0;
                int j;
                for(j = i+1; argVector[j]!=NULL; j++)
                {
                    strcpy(redirectcmd2[j-i-1], argVector[j]);
                }
                redirectcmd2[j-i-1] = 0;
                execRedirect(redirectcmd1, redirectcmd2, 1);
                redirect = 1;
                break;
            }
        }

        if (redirect == 1)
        {
            printPrompt();
            continue;
        }
		// Structure -> CMD1 | tee args | CMD2
		pipe(pipefd);

		int pid = fork();

		if (pid == 0)
		{
			sigprocmask(SIG_UNBLOCK, &set, NULL);
			if (execvp(argVector[0], argVector) < 0) {
				printf("myShell: %s: command not found\n", argVector[0]);
				exit(0);
			}
		}

		waitpid (pid, NULL, 0);
		printPrompt();
	}

	for(int i=0; i<100; i++)
		free(argVector[i]);
	if(argVector)
		free(argVector);

	return 0;
}

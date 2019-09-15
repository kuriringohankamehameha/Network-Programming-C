#include "shell.h"
#include<fcntl.h>

#define MAX_NUM_PATHS 100
#define uint32_t u_int32_t 
#define COLOR_NONE "\033[m"
#define COLOR_RED "\033[1;37;41m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_CYAN "\033[0;36m"
#define COLOR_GREEN "\033[0;32;32m"
#define COLOR_GRAY "\033[1;30m"

enum bool {false = 0, true = ~0};
enum _Status { STATUS_SUSPENDED, STATUS_BACKGROUND, STATUS_FOREGROUND };

typedef struct _Job_t Job;
typedef struct _Node_t Node;
typedef enum _Status Status;
typedef enum bool bool; 

bool isSuspended = false;

struct _Node_t{

	int pid;
    char* name;
    Status status;
    int gid;

};

struct _Job_t{

	Node node;
	Job* next;

};

// Create the struct
Job* createJobs(Node node)
{
	Job* head = (Job*) malloc (sizeof(Job));
	head->node = node;
	head->next = NULL;

	return head;
}

// Insert a Job
Job* insert(Job* head, Node node)
{
	Job* newnode = (Job*) malloc (sizeof(Job));
	newnode->node = node;
	newnode->next = NULL;

	Job* temp = head;
	
	if (temp->next != NULL)
	{
		while (temp->next)
		{
			temp = temp->next;
		}
	}

	if (temp->node.pid != node.pid)
		temp->next = newnode;

	return head; 

}

// Remove a Job
Job* removeJob(Job* head, int pid)
{
    if (isSuspended){
        isSuspended = false;
        return head;
    }

	Job* temp = head;
    if(pid < 0 || temp->node.pid == pid) {
	    head = temp->next;
	    temp->next = NULL;
	    free(temp);
	    return head;
    } else {
        // Remove PID
        while(temp->next) {
            if (temp->next->node.pid == pid) {
                Job* del = temp->next;
                temp->next = del->next;
                del->next = NULL;
                free(del);
            }
            else
                temp = temp->next;
        }
        return head;
    }
}

Job* findPID(Job* head, int pid)
{
	Job* temp = head;
	while (temp)
	{
		if (temp->node.pid == pid)
			return temp;
		temp = temp->next;
	}

	return NULL;
}

int findFG_STATUS(Job* head)
{
    // Returns the PID of the process in the FG that is not the parent
    Job* temp = head;
    temp = temp->next;
    while (temp)
    {
        if (temp->node.status == STATUS_FOREGROUND)
            return temp->node.pid;
        temp = temp->next;
    }

    return -1;
}

void printList(Job* head)
{
	Job* temp = head;
	while (temp)
	{
		printf("pid = %d, name = %s, status = %d -> ", temp->node.pid, temp->node.name, temp->node.status);
		temp = temp->next;
	}
	printf("\n");
}

Job* changeStatus(Job* head, int pid, Status status)
{
    Job* temp = head;
    while (temp)
    {
        if (temp->node.pid == pid)
        {
            temp->node.status = status;
            return head;
        }
        temp = temp->next;
    }
    return head;
}

void freeJobs(Job* jobSet)
{
    Job* temp = jobSet;
    while(temp)
    {
        free(temp->node.name);
        temp = temp->next;
    }
}

// Signal Handler Prototype
void shellfgCommand(int pid);

// Global Variables for Process Flow
char** PATH; // PATH Variable
pid_t childPID, parentPID;
bool isBackground = false;
char** argVector;
int redirection_in_pipe = 0;

// Initialize the jobSet
Job* jobSet; 

//static inline 
void printPrompt()
{
    printf("%smyShell~: $ %s", COLOR_GREEN, COLOR_YELLOW);
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
    if (signo == SIGINT)
    {
        signal(signo, SIG_DFL);
    }

    else if (signo == SIGTSTP)
    {
        char* name = (char*) malloc (sizeof(char));
        strcpy(name, argVector[0]);
        Node node = {childPID, name, STATUS_SUSPENDED, childPID};
        jobSet = insert(jobSet, node); 
        printList(jobSet);
        setpgid(childPID, childPID);
        kill(-childPID, SIGSTOP);
    }

    else if (signo == SIGCHLD)
    {
        Job* temp = findPID(jobSet, info->si_pid);
        if (temp && temp->node.status == STATUS_BACKGROUND)
        {
            printf("[bg]  %s - %d finished\n", temp->node.name, info->si_pid);
        }
        fflush(stdout);
        fflush(stdin);
        //signal(SIGCHLD, SIG_DFL);
    }

}

uint32_t setPATH(const char* filename)
{
	// Sets the PATH Variable from the config file
	// and returns it's length

	PATH = (char**) malloc (MAX_NUM_PATHS*sizeof(char*));
	for (uint32_t i=0; i<MAX_NUM_PATHS; i++)
		PATH[i] = (char*) calloc (50, sizeof(char));

	char buffer[256];
	FILE* fp = fopen(filename, "r");
	uint32_t pathLength = 0;
	uint32_t j = 0;

	if (!fp)
	{
		printf("Error opening file '%s'\n", filename);
		exit(1);
	}

	while (fgets(buffer, sizeof(buffer), fp))
	{
		for (uint32_t i=0; ; i++) {
			if (buffer[i] == '\n') {
				PATH[pathLength][j] = '\0';
				break;
			}
			else
				PATH[pathLength][j++] = buffer[i];
		}
		pathLength++;
		j = 0;
	}

	return pathLength;
}

sigset_t createSignalSet(int* signalArray)
{
	sigset_t set;
	sigemptyset(&set);
	for(int i=0; signalArray[i]!=-1; i++)
		sigaddset(&set, signalArray[i]);
	return set;	
}

void freePATH()
{
	// Release the PATH!!
	for (uint32_t i=0; i<MAX_NUM_PATHS; i++)
		free(PATH[i]);
	free(PATH);
}

uint32_t getArgumentLength(char* command)
{
	int count = 1;
	for(uint32_t i=0; command[i] != '\0'; i++)
		if(command[i] == ' ')
			count ++;
	return count;
}

int builtin(char* command)
{
    // TODO (Vijay): Add builtin commmands
    // 1. kill
    // 2. bg
    // 3. joblist
    // 4. cd
    
	return 0;
}

void shellkillCommand(int pid, int signo)
{
    Job* temp = findPID(jobSet, pid);
    if (temp){

        // Send a Signal to the child process based on the number 
        kill(pid, signo); 
    }
}

void shellfgCommand(int pid)
{
	Job* temp = findPID(jobSet, pid);
	if (temp && temp->node.status != STATUS_FOREGROUND)	
	{
		// Bring the process to foreground
        // First, give the pid a CONT signal to resume from suspended state
		if (kill(-pid, SIGCONT) < 0)
        {
            if (kill(pid, SIGCONT) < 0)
            {
                printf("myShell: fg %d: no such job\n", pid);
                return;
            }
        }

        // Make the pid get the terminal control
        printf("[fg]  + %d continued  %s\n", temp->node.pid, temp->node.name);
        if (tcsetpgrp(0, pid) < 0)
			perror ("tcsetpgrp()");

		int status = 0;
        
        // Now the parent waits for the child to do it's job
        waitpid(pid, &status, WUNTRACED);
        // Ignore SIGTTIN, SIGTTOU Signals till parents get back control
		signal(SIGTTOU, SIG_IGN);
        tcsetpgrp(0, getpid());
		signal(SIGTTOU, SIG_DFL);
	}

	else
        printf("myShell: fg %d: no such job\n", pid);
}

void shellbgCommand(int pid)
{
    // Resumes a job in the background

    Job* temp = findPID(jobSet, pid);
    if (temp && temp->node.pid != parentPID)   
    {
        if (temp->node.status != STATUS_SUSPENDED)
        {
            printf("myShell: bg %d: job not currently suspended. Cannot resume\n", pid);
            return;
        }

        jobSet = changeStatus(jobSet, pid, STATUS_BACKGROUND);

        if (kill(-pid, SIGCONT) < 0)
        {
            printf("myShell: bg %d: no such job\n", pid);
            return;
        }

        printf("[bg]  + %d continued  %s\n", temp->node.pid, temp->node.name);
        if (tcsetpgrp(pid, pid) < 0)
            perror ("tcsetpgrp()");

        signal(SIGTTOU, SIG_IGN);
        tcsetpgrp(0, getpid());
        signal(SIGTTOU, SIG_DFL);
    }

    else
        printf("myShell: bg %d: no such job\n", pid);

}

void freeArgVector(int argLength, char** argVector)
{
    for(int i=0; i<=argLength; i++)
        free(argVector[i]);
    free(argVector);
}

int getPIDfromname(Job* jobSet, char* name)
{
    Job* temp = jobSet;
    while (temp)
    {
        if (strcmp(temp->node.name, name) == 0)
            return temp->node.pid;
        temp = temp->next;
    }

    return 0;
}

void executePipe(char ***cmd)
{
    // Precondition : a < b | c can occur only if < is before the first pipe symbol
    int p[2];
    int pid;
    int fd_in = 0;

    while (*cmd != NULL)
    {
        pipe(p);
        if ((pid = fork()) == -1)
        {
            printf("Error\n");
            exit(1);
        }
        else if (pid == 0)
        {
            dup2(fd_in, 0);
            if (*(cmd + 1) != NULL)
            {
                dup2(p[1], 1);
            }
            
            if (redirection_in_pipe == 1)
            {
                for (int i=0; cmd[0][i]!=NULL; i++)
                {
                    if (strcmp(cmd[0][i], "<") == 0)
                    {
                        if (cmd[0][i+1] == NULL)
                        {
                            printf("Error: Redirection File does not exist\n");
                            exit(0);
                        }

                        // Check if file exists
                        if (access(cmd[0][i+1], F_OK) == -1)
                        {
                            printf("myShell: No such file or directory : %s\n", cmd[0][i+1]);
                            exit(0);
                        }

                        char* filename = (char*) calloc (strlen(cmd[0][i+1])+1, sizeof(char));
                        strcpy(filename, cmd[0][i+1]);
                        int fd = open(filename, O_RDONLY, 0644);

                        cmd[0][i+1] = 0;
                        cmd[0][i] = 0;
                        
                        int pid = fork();
                        int save_in = dup(STDIN_FILENO);
                        if (pid == 0)
                        {
                            dup2(fd, STDIN_FILENO);
                            close(fd);
                            execvp(cmd[0][0], cmd[0]);
                            fprintf(stderr, "Failed to execute %s\n", cmd[0][0]);
                        }
                        else{
                            int status;
                            waitpid(pid, &status, NULL);
                            dup2(save_in, STDIN_FILENO);
                            close(save_in);
                        }
                        redirection_in_pipe = 0;
                    }
                }
            }
            close(p[0]);
            execvp((*cmd)[0], *cmd);
            exit(1);
        }
        else{
            wait(NULL);
            close(p[1]);
            fd_in = p[0];
            cmd++;
        }
    }
}

void execRedirect(char** cmd1, char** cmd2, int redirection)
{
    // Check sizes of commmands
    int fd = -1;
    int count1 = 0;
    int count2 = 0;
    for (int i=0; cmd1[i]!=NULL; i++)
        count1 ++;
    for (int i=0; cmd2[i]!=NULL; i++)
        count2 ++;

    if (count2 > 1)
    {
        printf("Error : Can only redirect to a File\n");
        return;
    }

    if (redirection == 0)
    {
        // Redirection to output file >
        fd = open(cmd2[0], O_WRONLY | O_CREAT, 0644);
    }

    else if (redirection == 1)
    {
        // <
        if (count2 > 1)
        {
            printf("Error : Can only redirect \'<\' from a File\n");
            return;
        }

        if (access(cmd2[0], F_OK) == -1)
        {
            printf("myShell: No such file or directory : %s\n", cmd2[0]);
            return;
        }
        
        fd = open(cmd2[0], O_RDONLY, 0644);

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
        return;
    }
    else if (redirection == 2)
    {
        fd = open(cmd2[0], O_WRONLY | O_APPEND | O_CREAT, 0644);
    }

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
	// Set the PATH first
	
	uint32_t pathLength = setPATH(".myshellrc");
	char* buffer = (char*) malloc (sizeof(char));
    
    int argLength = -1;
    argVector = NULL;

	jobSet = (Job*) malloc (sizeof(Job));
	Node jnode;
	jnode.pid = getpid();
    jnode.name = (char*) calloc (150, sizeof(char));
    strcpy(jnode.name, argv[0]);
	jnode.status = STATUS_FOREGROUND;
    jnode.gid = getpid();
    jobSet->node = jnode;
	jobSet->next = NULL;

	int signalArray[] = {SIGINT, SIGKILL, SIGTSTP, -1};
	sigset_t set = createSignalSet(signalArray);
	struct sigaction sa = {0};
	sa.sa_handler = shellHandler;
	sa.sa_flags = SA_SIGINFO;

	sigprocmask(SIG_BLOCK, &set, NULL);

	printPrompt();

	// Now start the event loop
	while((buffer = lineget()))
    {
        redirection_in_pipe = 0;
        for (uint32_t i=0; buffer[i] != '\0'; i++)
			if (buffer[i] == '\n')
				buffer[i] = '\0';

        Job* curr = jobSet;
        while (curr)
        {
            if (curr->node.status == STATUS_BACKGROUND && waitpid(curr->node.pid, NULL, WNOHANG))
            {   // Background Job has returned
                //printf("[bg]  %d finished\n", curr->node.pid);
                jobSet = removeJob(jobSet, curr->node.pid);
            }
            curr = curr->next;
        }	

		if (strcmp(buffer, "exit") == 0)
		{
			freePATH();
            freeJobs(jobSet);
            free(jobSet);
			exit(0);
		}

		if (strcmp(buffer, "printlist") == 0)
        {
			printList(jobSet);
			printPrompt();
			continue;
		}

		if (builtin(buffer) == 1)
		{
			printPrompt();
			continue;
		}

		else
		{
			argLength = getArgumentLength(buffer);
			argVector = (char**) malloc ((argLength+1)* sizeof(char*));
			for (uint32_t i=0; i <= argLength; i++)
				argVector[i] = (char*) calloc (100, sizeof(char));

			int count = 0;
			int j = 0;
			for (uint32_t i=0; buffer[i] != '\0'; i++)
			{
				if (buffer[i] == ' ' && buffer[i+1] == ' ')
				{
                    continue;
                }
                else if (buffer[i] == ' ')
                {
                    argVector[count][j++] = '\0';
                    count++;
                    j = 0;
                }
                else
                {
                    argVector[count][j] = buffer[i];
                    j++;
                }
			}

			argVector[count][j] = '\0';
			argVector[count + 1] = 0;

			if (strcmp(argVector[count], "&") == 0)
			{
				isBackground = true;
				argVector[count] = 0;
			}

            if (strcmp(argVector[0], "fg") == 0)
            {
                int pid = -1;

                if (jobSet)
                {
                    if (argVector[1][0] == '%' && argVector[1][1] == '.' && argVector[1][2] != '\0')
                    {
                        // Search by name
                        char* name = (char*) malloc (sizeof(char));
                        
                        int i;
                        for (i=2; argVector[1][i] != '\0'; i++)
                            name[i-2] = argVector[1][i];
                        name[i-2] = '\0';

                        pid = getPIDfromname(jobSet, name);
                        if (!pid){
                            printf("myShell: fg %s: no such job\n", name);
                            printPrompt();
                            isBackground = false;
                            continue;
                        }
                        free(name);
                    }
                    shellfgCommand( pid == -1 ? atoi (argVector[1]) : pid );
                    freeArgVector(argLength, argVector);
                    printPrompt();
                    isBackground = false;
                    continue;
                }
                else
                {
                    printf("No background jobs!\n");
                    freeArgVector(argLength, argVector);
                    printPrompt();
                    isBackground = false;
                    continue;
                }
            }

            if (strcmp(argVector[0], "bg") == 0)
            {
                int pid = -1;

                if (jobSet)
                {
                    if (argVector[1][0] == '%' && argVector[1][1] == '.' && argVector[1][2] != '\0')
                    {
                        // Search by name
                        char* name = (char*) malloc (sizeof(char));
                        
                        int i;
                        for (i=2; argVector[1][i] != '\0'; i++)
                            name[i-2] = argVector[1][i];
                        name[i-2] = '\0';

                        pid = getPIDfromname(jobSet, name);
                        if (!pid){
                            printf("myShell: bg %s: no such job\n", name);
                            printPrompt();
                            isBackground = false;
                            continue;
                        }
                        free(name);
                    }
                    shellfgCommand( pid == -1 ? atoi (argVector[1]) : pid );
                    freeArgVector(argLength, argVector);
                    printPrompt();
                    isBackground = false;
                    continue;
                }
                else
                {
                    printf("No suspended jobs!\n");
                    freeArgVector(argLength, argVector);
                    printPrompt();
                    isBackground = false;
                    continue;
                }
            }
		
            char*** commands;
            int a = 0;
            int lastPipe = 0;
            int pipeCount = 0;
            int badRedirectpipe = 0;

            for (int i=0; argVector[i] != NULL; i++)
            {
                if (strcmp(argVector[i], "|") == 0)
                {
                    if (i > 2 && strcmp(argVector[i-2], "<") == 0)
                    {
                        if (redirection_in_pipe == 0)
                            redirection_in_pipe = 1;
                        else
                        {
                            printf("Error: Multiple Indirection Operators with Pipes\n");
                            badRedirectpipe = 1;
                            break;
                        }

                    }

                    if (lastPipe == 0)
                    {
                        commands = (char***) calloc (20, sizeof(char**));
                        for (int i=0; i<20; i++)
                        {
                            commands[i] = (char**) calloc (20, sizeof(char*));
                            for (int j=0; j<20; j++)
                            {
                                commands[i][j] = (char*) malloc (sizeof(char));
                            }
                        }

                        for (a=0; a<i; a++)
                        {
                            commands[pipeCount][a] = (char*) realloc (commands[pipeCount][a], strlen(argVector[a])+1);    
                            strcpy(commands[pipeCount][a], argVector[a]); 
                        }
                        // Make NULL terminated command
                        commands[pipeCount][i] = 0;

                        // Update Pipe location
                        lastPipe = i;
                        pipeCount++;
                    }

                    else
                    {
                        for (a = lastPipe+1; a<i; a++)
                        {
                            commands[pipeCount][a-lastPipe-1] = (char*) realloc (commands[pipeCount][a-lastPipe-1], strlen(argVector[a])+1);    
                            strcpy(commands[pipeCount][a-lastPipe-1], argVector[a]);
                        }

                        // Make NULL terminated command
                        commands[pipeCount][i-lastPipe-1] = 0;

                        // Update Pipe location
                        lastPipe = i;
                        pipeCount++;
                   }

               }
            }

            if (badRedirectpipe == 1)
            {
                printPrompt();
                continue;
            }
            
            if (pipeCount > 0)
            {
                for(a=lastPipe + 1; argVector[a]!=NULL; a++)
                {
                    if (strcmp(argVector[a], "|") != 0)
                    {
                        commands[pipeCount][a-lastPipe-1] = (char*) realloc (commands[pipeCount][a-lastPipe-1], strlen(argVector[a])+1);    
                        strcpy(commands[pipeCount][a-lastPipe-1], argVector[a]);
                    }
                }
                commands[pipeCount][a-lastPipe-1] = 0;

                // Now, I must have a NULL terminated array of Commands
                commands[pipeCount+1][a] = 0;
                commands[pipeCount+1] = 0;
            }


            if (pipeCount > 0)
            {
                executePipe(commands);
                printPrompt();
                for(int i=0; i<20; i++)
                {
                    free(commands[i]);
                }
                free(commands);
                freeArgVector(argLength, argVector);
                continue;
            }
            

            int redirect = 0;

            char** redirectcmd1 = (char**) calloc (5, sizeof(char*));
            char** redirectcmd2 = (char**) calloc (5, sizeof(char*));
            for (int i=0; i<5; i++)
            {
                redirectcmd1[i] = (char*) malloc (sizeof(char));
                redirectcmd2[i] = (char*) malloc (sizeof(char));
            }

            for (int i=0; argVector[i]!=NULL; i++)
            {
                if (strcmp(argVector[i], ">") == 0 || strcmp(argVector[i], ">>") == 0)
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
                    if (strcmp(argVector[i], ">") == 0)
                        execRedirect(redirectcmd1, redirectcmd2, 0);
                    else
                        execRedirect(redirectcmd1, redirectcmd2, 2);
                    redirect = 1;
                    break;
                }
                
                else if (strcmp(argVector[i], "<") == 0)
                {
                    for(int j = 0; j<i; j++)
                    {
                        strcpy(redirectcmd1[j], argVector[j]); 
                    }
                    redirectcmd1[i] = 0; 
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
                for (int i=0; i<5; i++)
                {
                    free(redirectcmd1[i]);
                    free(redirectcmd2[i]);
                }
                free(redirectcmd1);
                free(redirectcmd2);
                printPrompt();
                continue;
            }
            int pid = fork();
            childPID = pid;
            //sigaction(SIGCHLD, &sa, NULL);

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

				if (isBackground == true)
				{
					printf("[bg]  %d\n",getpid());
                    setpgid(0, 0);
				}

				else 
				{
					// Add to foreground process group
					pid_t cgrp = getpgrp();
					// Problem with below line
                    signal(SIGTTOU, SIG_IGN);
                    tcsetpgrp(STDIN_FILENO, cgrp);
                    signal(SIGTTOU, SIG_DFL);
                    setpgid(getpgrp(), getppid());
				}

                childPID = getpid();
                int curr = 0;
                while (curr < pathLength)
                {
                	if (execv(str_concat(PATH[curr], argVector[0]), argVector) < 0)
                	{
                		curr ++;
                		if (curr == pathLength)
                		{
                			perror("Error");
                            freeArgVector(argLength, argVector);
                			exit(0);
                		}
                	}
                }

			}

			else
			{
				parentPID = getpid();
                setpgid(0, 0);
                if (isBackground)
                {
                    Node node;
                    node.pid = pid;
                    node.status = STATUS_BACKGROUND;
                    node.gid = pid;
                    char* name = (char*) malloc (sizeof(char));
                    strcpy(name, argVector[0]);
                    node.name = name;
                    jobSet = insert(jobSet, node);
                    setpgrp();
                    //tcsetpgrp(STDIN_FILENO, getpgrp());
                    int status;
                    if (waitpid (pid, &status, WNOHANG) > 0)
                        printf("[bg]  %d finished\n", pid);
                }

                else
                {
                    int status;
                    //if(waitpid(-1, &status, WSTOPPED | WCONTINUED) >= 0){
                    waitpid(pid, &status, WUNTRACED);
                    if (WSTOPSIG(status)) {
                            setpgid(pid, pid);
                            char* name = (char*) malloc (sizeof(char));
                            strcpy(name, argVector[0]);
                            Node node = {pid, name, STATUS_SUSPENDED, getpgrp()};
                            jobSet = insert(jobSet, node);
                            printf("suspended  %s %d\n", node.name, node.pid);
                            signal(SIGTTOU, SIG_IGN);
                            tcsetpgrp(0, getpid());
                            signal(SIGTTOU, SIG_DFL);
                    }

                    //waitpid(pid, NULL, WUNTRACED);
                }
                isBackground = false;
				printPrompt();
                for (int i=0; i<5; i++)
                {
                    free(redirectcmd1[i]);
                    free(redirectcmd2[i]);
                }
                free(redirectcmd1);
                free(redirectcmd2);
			}
		}
	}	
	return 0;
}
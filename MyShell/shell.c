#include "shell.h"

#define MAX_NUM_PATHS 100
#define uint32_t u_int32_t 

enum bool {false, true};
enum _JobType {FG_JOB, BG_JOB, PEND_JOB, NONE};

typedef struct _Job_t Job;
typedef struct _Node_t Node;
typedef enum _JobType JobType;
typedef enum bool bool; 

struct _Node_t{

	int pid;
	JobType jobtype;

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
	Job* temp = head;
    if(pid < 0 || temp->node.pid == pid){
	    head = temp->next;
	    temp->next = NULL;
	    free(temp);
	    return head;
    } else {
        // Remove PID
        while(temp->next){
            if (temp->next->node.pid == pid){
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

void printList(Job* head)
{
	Job* temp = head;
	while (temp)
	{
		printf("pid = %d -> ", temp->node.pid);
		temp = temp->next;
	}
	printf("\n");
}

// Signal Handler Prototype
static void shellHandler(int, siginfo_t*, void*);
void shellfgCommand(int pid);

// Global Variables for Process Flow
char** PATH; // PATH Variable
JobType processType = NONE;
pid_t childPID, parentPID;
bool isBackground = false;

// Initialize the jobSet
static Job* jobSet; 

static inline void printPrompt()
{
    printf("myShell~: $ ");
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

static void shellHandler(int signo, siginfo_t* info, void* context)
{
	// Passes it into the child first
	if (signo == SIGINT || signo == SIGKILL)
	{
		kill(childPID, signo);
		kill(parentPID, SIGCHLD);
	}

	else
	{
		signal(signo, SIG_IGN);
		printf("\n");
		printPrompt();
	}
}

uint32_t setPATH(const char* filename)
{
	// Sets the PATH Variable from the config file
	// and returns it's length

	PATH = (char**) malloc (MAX_NUM_PATHS*sizeof(char*));
	for (uint32_t i=0; i<MAX_NUM_PATHS; i++)
		PATH[i] = (char*) malloc (sizeof(char));

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
		for (uint32_t i=0; ; i++)
		{
			if (buffer[i] == '\n')
			{
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

int bulletin(char* command)
{
    // TODO (Vijay): Add bulletin commmands
    // 1. kill
    // 2. bg
    // 3. joblist
    // 4. cd
    
	return 0;
}

void shellfgCommand(int pid)
{
	Job* temp = findPID(jobSet, pid);
	if (temp && temp->node.jobtype == BG_JOB)	
	{
		// Bring the process to foreground
        // First, give the pid a CONT signal to resume from suspended state
		if (kill(-pid, SIGCONT) < 0)
        {
            printf("myShell: fg %d: no such job\n", pid);
            return;
        }
	    
        // Make the pid get the terminal control
		if (tcsetpgrp(0, pid) < 0)
			perror ("tcsetpgrp()");

		int status = 0;
        
        // Now the parent waits for the child to do it's job
        waitpid(pid, &status, WUNTRACED);
        
        // Ignore SIGTTIN, SIGTTOU Signals till parents get back control
		//signal(SIGTTIN, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);
		
		//tcsetpgrp(0, parentPID);
        tcsetpgrp(0, getpid());

		//signal(SIGTTIN, SIG_DFL);
		signal(SIGTTOU, SIG_DFL);
	}

	else
	{
        printf("myShell: fg %d: no such job\n", pid);
	}
}

void freeArgVector(int argLength, char** argVector)
{
    for(int i=0; i<argLength; i++)
        free(argVector[i]);
    free(argVector);
}

int main(int argc, char* argv[])
{
	// Set the PATH first
	
	uint32_t pathLength = setPATH(".myshellrc");
	char* buffer = NULL;
	size_t n = 0;
    
    int argLength = -1;
    char** argVector = NULL;

	jobSet = (Job*) malloc (sizeof(Job));
	Node jnode;
	jnode.pid = getpid();
	jnode.jobtype = FG_JOB;
	jobSet->node = jnode;
	jobSet->next = NULL;

	int signalArray[] = {SIGINT, SIGKILL, -1};
	sigset_t set = createSignalSet(signalArray);
	struct sigaction sa;
	sa.sa_handler = shellHandler;
	sa.sa_flags = SA_SIGINFO;

	// Parent must be blocked from these signals
	sigprocmask(SIG_BLOCK, &set, NULL);

	printPrompt();

	// Now start the event loop
	while (getline(&buffer, &n, stdin))
	{
		for (uint32_t i=0; buffer[i] != '\0'; i++)
			if (buffer[i] == '\n')
				buffer[i] = '\0';

		if (strcmp(buffer, "exit") == 0)
		{
			free(buffer);
			freePATH();
			exit(0);
		}

		if (strcmp(buffer, "printlist") == 0)
        {
			printList(jobSet);
			printPrompt();
			continue;
		}

		if (bulletin(buffer) == 1)
		{
			printPrompt();
			continue;
		}

		else
		{
			Job* curr = jobSet;
			while (curr)
			{
				if (curr->node.jobtype == BG_JOB && waitpid(curr->node.pid, NULL, WNOHANG))
				{
					// Background Job has returned
					jobSet = removeJob(jobSet, curr->node.pid);
                    curr = curr->next;
				}

				else
					curr = curr->next;
			}	

			argLength = getArgumentLength(buffer);
			argVector = (char**) malloc (argLength * sizeof(char*));
			for (uint32_t i=0; i < argLength; i++)
				argVector[i] = (char*) malloc (100 * sizeof(char));

			int count = 0;
			int j = 0;
			for (uint32_t i=0; buffer[i] != '\0'; i++)
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
			argVector[count + 1] = NULL;

			if (strcmp(argVector[count], "&") == 0)
			{
				isBackground = true;
				argVector[count] = NULL;
			}

            if (strcmp(argVector[0], "fg") == 0)
            {
                if (jobSet)
                {
                    shellfgCommand(atoi(argVector[1]));
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

			int pid = fork();

			if (pid == 0)
			{
				// Child
				if (isBackground == true)
				{
					printf("[bg]\t%d\n",getpid());
					setpgid(0, 0);
				}

				else 
				{
					// Add to foreground process group
					pid_t cgrp = getpgrp();
					tcsetpgrp(STDIN_FILENO, cgrp);
				}

				// The child must receive the previously blocked signals
                sigprocmask(SIG_UNBLOCK, &set, NULL);
                sigaction(SIGINT, &sa, NULL);
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
                            free(buffer);
                			exit(0);
                		}
                	}
                }

			}

			else
			{
				parentPID = getpid();
                
                if (isBackground)
                {
                    pid_t cpgrp = pid;
                    Node node;
                    node.pid = pid;
                    node.jobtype = BG_JOB;
                    jobSet = insert(jobSet, node);
                    setpgrp();
                }
                else
                    waitpid(pid, NULL, 0);
				
                isBackground = false;

				for(uint32_t i=0; i<argLength; i++)
					free(argVector[i]);
				free(argVector);

				printPrompt();
			}

		}

	}	

	free(buffer);
	return 0;
}

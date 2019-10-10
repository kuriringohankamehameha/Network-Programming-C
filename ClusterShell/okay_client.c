#define _GNU_SOURCE
#include "cluster.h"
#define PORT 8080
#define MAX 300
#define MAX_NUM_PATHS 100
#define uint32_t u_int32_t 
#define COLOR_NONE "\033[m"
#define COLOR_RED "\033[1;37;41m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_CYAN "\033[0;36m"
#define COLOR_GREEN "\033[0;32;32m"
#define COLOR_GRAY "\033[1;30m"

enum bool { false = 0, true = ~0 };

typedef enum bool bool;

/* Declare all Global Variables here */
char** PATH; // PATH Variable
uint32_t pathLength = 0;
pid_t childPID, parentPID; // PIDs for Child and Parent Process
bool isBackground = false; // Checks whether the child is a background process
int bgcount = -1;
int argLength = 0;
char** argVector; // Argument Vector being passed to the Shell
int redirection_in_pipe = 0; // For checking if there is an input redirection via pipe
int redirection_out_pipe = 0; // For chcking if there is an output redirection via pipe
int badRedirectpipe = 0; // Checks for valid Pipe redirection
int pipeCount = 0; // Counter for the number of Pipes in the argVector
bool redirect = false;
char** redirectcmd1;
char** redirectcmd2;
int pipeargCount = 0;
int lastPipe = 0; // Position of the last pipe symbol
char* USER; // Current User
char* HOME; // Home Directory
char* HOST;
int HOME_LEN;
bool ignorePATH = false; // For executing dot slash command
char** command;
char*** commands;
bool noinit_cmd = false;
char* current_directory;

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

void replaceSubstring(char* str)
{
    char* ptr = strstr(str, HOME);
    if (ptr)
    {
        current_directory[0] = '~';
        
        if (str[HOME_LEN] == '\0')
        {
            current_directory[1] = '\0';
            return;
        }

        for(int i=1; str[i]!='\0'; i++)
        {
            if (str[i+HOME_LEN-1] != '\0')
                current_directory[i] = str[i+HOME_LEN-1];
            else
            {
                current_directory[i] = '\0';
                break;
            }
        }
        return;
    }
    return;
}

void printPrompt()
{
    gethostname(HOST, 100);
    if (current_directory != NULL)
    {
        printf("%s%s@%s%s | %s%s%s > $ %s", COLOR_CYAN, USER, HOST, COLOR_GRAY, COLOR_RED, current_directory, COLOR_GREEN, COLOR_YELLOW);
    }
    else
    {
        char* s = (char*) malloc (300* sizeof(char));
        current_directory = getcwd(s, 300);
        printf("%s%s@%s%s | %s%s%s > $ %s", COLOR_CYAN, USER, HOST, COLOR_GRAY, COLOR_RED, current_directory, COLOR_GREEN, COLOR_YELLOW);
        free(s);
    }
}

uint32_t getArgumentLength(char* command)
{
	int count = 1;
	for(uint32_t i=0; command[i] != '\0'; i++)
		if(command[i] == ' ' && command[i+1] != ' ' && command[i+1] != '\0')
			count ++;
	return count;
}

char* lineget(char* buffer) {
    if (buffer == NULL)
        buffer = malloc(300);
        
    char* line = buffer; 
    char* linep = line;
    size_t lenmax = 300, len = lenmax;
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

int initArgs(char* buffer)
{
    // Gets the argument length and copies all arguments as a list
    // to the argument vector
    int count = 0;
    int j = 0;
    for (uint32_t i=0; buffer[i] != '\0'; i++)
    {
        if (buffer[i] == ' ' && buffer[i+1] == ' ')
            continue;
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
    return count;
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

    fclose(fp);

	return pathLength;
}

void execute_child(int pathLength)
{
    // The child must receive the previously blocked signals
    if (isBackground == true)
    {
        // Make the process have it's own group id
        printf("[bg]  %d\n",getpid());
        setpgid(0, 0);
    }

    else 
    {
        // Add to foreground process group
        pid_t cgrp = getpgrp();
        tcsetpgrp(STDIN_FILENO, cgrp);
        setpgid(getpgrp(), getppid());
    }

    childPID = getpid();
    int curr = 0;
    
    if (ignorePATH == false)
    {
        while (curr < pathLength)
        {
            // Keep going through the PATH until we can execute one program
            if (execv(str_concat(PATH[curr], argVector[0]), argVector) < 0)
            {
                curr ++;
                if (curr == pathLength)
                {
                    perror("Error");
                    exit(0);
                }
            }
        }
    
    }

    else
    {
        ignorePATH = false;
        char s[300];
        if ( execv(str_concat(getcwd(s, 300), argVector[0]), argVector) < 0 )
        {
            perror("Error");
            exit(0);
        }
    }
}

void initStuff()
{
    // Get some Environment Variables for pretty prompt
    USER = getenv("USER");
    HOME = getenv("HOME");
    HOST = (char*) malloc (100 * sizeof(char));
    HOME_LEN = strlen(HOME);
}

void REPL(char* buffer)
{
    int count;
    current_directory = (char*) calloc (300, sizeof(char));
    
    // Initialise stuff ...
	pathLength = setPATH(".myshellrc");

    // Create the signal array for blocking/unblocking
    // The main process needs to block, while the children can unblock
    // the signals

    // We need to find the Current Directory before printing the prompt
    char* s = (char*) malloc (300*sizeof(char));
    replaceSubstring(getcwd(s, 300));
    free(s);
	
    printPrompt();

	// Now start the event loop
    // Get the argument length
    argLength = getArgumentLength(buffer);
    argVector= (char**) malloc ((argLength + 1)* sizeof(char*));
    for (int i=0; i <= argLength; i++)
        argVector[i] = (char*) malloc (sizeof(char));

    redirection_in_pipe = 0;
    redirection_out_pipe = 0;
    pipeCount = 0;
    pipeargCount = 0;
    bgcount = -1;

    for (uint32_t i=0; buffer[i] != '\0'; i++)
        if (buffer[i] == '\n')
            buffer[i] = '\0';

    if (strcmp(buffer, "exit") == 0)
    {
        return;
    }

    else
    {
        count = initArgs(buffer);
        
        if (strcmp(argVector[count], "&") == 0)
        {
            // Needs to be a Background process
            isBackground = true;
            bgcount = count;
            for(int i=count; i<argLength; i++)
                free(argVector[i]);
            argVector[count] = 0;
        }

        int pid = fork();
        childPID = pid;

        if (pid == 0)
            execute_child(pathLength);

        else
        {
            parentPID = getpid();
            setpgid(0, 0);
            int status;
            waitpid(pid, &status, WUNTRACED);

            if (isBackground) {
                for(int i=0; bgcount>=0 && i<bgcount; i++)
                    free(argVector[i]);
                free(argVector);
            }
            isBackground = false;
            noinit_cmd = false;
            lastPipe = 0;
            ignorePATH = false;
            tcsetpgrp(0, getpid());
            tcsetpgrp(1, getpid());
        }
    }
	return;
}

void chat_with_server(int sockfd)
{
    char buff[MAX];
    char buff2[MAX];
    int n;
    initStuff();
    int num_read;
    int opts;
    int r;
    int pid = fork();
    if (pid == 0) {
        bzero(buff2, sizeof(buff2));
        while(1) {
            r = recv(sockfd, buff2, sizeof(buff2), MSG_DONTWAIT);
            int len = strlen(buff2);
            if (len > 0) {
                printf("\nFrom server : %s\n", buff2);
                bzero(buff2, sizeof(buff2));
            }
        }
        exit(0);
    }

    for (;;) {
        //bzero(buff, sizeof(buff));
        //read(sockfd, buff, sizeof(buff));
        //printf("\n\nFrom Server : %s\n\n", buff);

        bzero(buff, sizeof(buff));
        bzero(buff2, sizeof(buff2));
        printPrompt();
        n = 0;
        //opts = fcntl(sockfd, F_GETFL);
        //opts = (opts | O_NONBLOCK);
        while ((buff[n++] = getchar()) != '\n' ) {
        }
        //opts = opts & (~O_NONBLOCK);
        buff[n] = '\n';
        buff[n+1] = '\0';
        int client_number = 0;
        if (buff[0] != '\0' && buff[1] != '\0' && buff[0] == 'n' && (buff[1] == '1' || buff[1] == '2' || buff[1] == '3') && buff[2] == '.') {
            // n1.ls
            client_number = buff[1] - '0';
            // Send the client number to the server
            write(sockfd, buff, sizeof(buff));
            //write(sockfd, buff, 2);
            //write(sockfd, buff+3, sizeof(buff) - 3);
            // Now send the command to the server in a separate message
        }
        else {
            // Execute the command in the client itself
            char* buffer = malloc (300);
            int a = 0;
            for (a=0; buff[a] != '\0'; a++)
                buffer[a] = buff[a];
            buffer[a] = '\0';
            REPL(buffer);
            free(buffer);
            write(sockfd, buff, sizeof(buff));
        }
        if (strncmp(buff, "exit", 4) == 0) {
            kill(pid, SIGINT);
            printf("Client has exited\n");
            break;
        }
        //bzero(buff, sizeof(buff));
        //read(sockfd, buff, sizeof(buff));
        //printf("\n\nFrom Server : %s\n\n", buff);
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Need an argument for client IP Address\n");
        exit(0);
    }
    char* IP_ADDRESS = argv[1];
    int sockfd, connfd;
    struct sockaddr_in servaddr, cliaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Failed to create Socket\n");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    servaddr.sin_port = htons(PORT);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Could not connect to Server\n");
        exit(0);
    }
    else
        printf("Connected to the Server!\n");

    chat_with_server(sockfd);

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    return 0;
}

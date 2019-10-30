#define _GNU_SOURCE
#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<signal.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/time.h>

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

char** initDoubleGlobalCharPtr(int num_arrays)
{
    char** ptr = (char**) calloc (num_arrays, sizeof(char*));
    for (int i=0; i<num_arrays; i++)
        ptr[i] = (char*) malloc (sizeof(char));
    return ptr;
}

void freeDoubleGlobalCharPtr(char*** ptr, int num_arrays)
{
    for (int i=0; i<num_arrays; i++)
        free((*ptr)[i]);
    free(*ptr);
    *ptr = NULL;
}

char*** initTripleGlobalCharPtr(int num_arrays1, int num_arrays2)
{
    char*** ptr = (char***) calloc (num_arrays1, sizeof(char**));
    for (int i=0; i<num_arrays1; i++)
    {
        ptr[i] = (char**) calloc (num_arrays2, sizeof(char*));
        for (int j=0; j<num_arrays2; j++)
            ptr[i][j] = (char*) malloc (sizeof(char));
    }
    return ptr;
}

void freeTripleGlobalCharPtr(char**** ptr, int num_arrays1, int num_arrays2)
{
    for (int i=0; i<num_arrays1 && (*ptr)[i]; i++)
    {
        for (int j=0; j<num_arrays2 && (*ptr)[i][j]; j++)
            free((*ptr)[i][j]);
        free((*ptr)[i]);
    }
    free(*ptr);
    *ptr = NULL;
}

void freeArgVector()
{
    if (!ignorePATH)
        for(int i=0; i<=argLength; i++)
            free(argVector[i]);
    free(argVector);
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

int builtin(char** argVector)
{
    // TODO (Vijay): Add more builtin commmands
    // 1. kill
    // 2. joblist
    
    // Execute dot slash commands

    if (argVector[1] == NULL && argVector[0][0] == '.' && argVector[0][1] == '/')
    {
        // Ignore PATH and simply execute from the current directory
        if (argVector[0][2] == '\0')
        {
            printf("myShell: Executabe must be a valid string\n");
            return 1;
        }

        argVector[0] ++;
        ignorePATH = true;

        return 0;
    }

    // Change Directory Command
    // The Shell itself changes it's current working directory

    else if (argVector[1] != NULL && strcmp(argVector[0], "cd") == 0)
    {
        int pos = -1;
        for(int i=0; argVector[1][i]!='\0'; i++)
        {
            if (argVector[1][0] == '~')
            {
                // Replace ~ with /home/$USR
                pos = i;
                break;
            }
        }
        if ( pos != -1 )
        {
            char* op = (char*) calloc (300, sizeof(char));
            int k = 0;
            for (int i=0; argVector[1][i] != '\0'; i++)
            {
                if (i == 0)
                {
                    strncpy(op, HOME, HOME_LEN);
                    k = k+HOME_LEN;
                }
                else
                    op[k++] = argVector[1][i];
            }
            op[k] = '\0';
            chdir(op);
            free(op);
        }
        else
            chdir(argVector[1]);

        char* s = (char*) malloc (300*sizeof(char));
        replaceSubstring(getcwd(s, 300));
        free(s);
        
        return 1;
    }
    
	return 0;
}

void executePipe(char ***cmd)
{
    int p[2];
    int pid;
    int fd_in = 0;
    int file_fd_in = -1;
    int file_fd_out = -1;
    int save_in = dup(STDIN_FILENO);
    int save_out = dup(STDOUT_FILENO);
    
    if (redirection_in_pipe == 1)
    {
        for (int i=0; cmd[0][i]!=NULL; i++)
        {
            if (strcmp(cmd[0][i], "<") == 0)
            {
                if (cmd[0][i+1] == NULL)
                {
                    printf("myShell: Redirection File does not exist\n");
                    return;
                }

                // Check if file exists
                if (access(cmd[0][i+1], F_OK) == -1)
                {
                    printf("myShell: No such file or directory : %s\n", cmd[0][i+1]);
                    return;
                }

                char* filename = (char*) calloc (strlen(cmd[0][i+1])+1, sizeof(char));
                strcpy(filename, cmd[0][i+1]);
                file_fd_in = open(filename, O_RDONLY, 0400);

                cmd[0][i+1] = 0;
                cmd[0][i] = 0;
                
                dup2(file_fd_in, STDIN_FILENO);
                close(file_fd_in);
            }
        }
    }

    if (redirection_out_pipe == 1)
    {
        for (int i=0; cmd[pipeCount][i]!=NULL; i++)
        {
            if (strcmp(cmd[pipeCount][i], ">") == 0 || strcmp(cmd[pipeCount][i], ">>") == 0)
            {
                if (cmd[pipeCount][i+1] == NULL)
                {
                    printf("myShell: Redirection File does not exist\n");
                    return;
                }

                char* filename = (char*) calloc (strlen(cmd[pipeCount][i+1])+1, sizeof(char));
                strcpy(filename, cmd[pipeCount][i+1]);
                
                if (strcmp(cmd[pipeCount][i], ">") == 0)
                    file_fd_out = open(filename, O_WRONLY | O_CREAT, 0644);
                else
                    file_fd_out = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0644);

                free(filename);
                
                cmd[pipeCount][i+1] = 0;
                cmd[pipeCount][i] = 0;
            }
        }
    }

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
            dup2(fd_in, STDIN_FILENO);
            if (*(cmd + 1) != NULL)
                dup2(p[1], STDOUT_FILENO);
            else
            {
                if (redirection_out_pipe == 1)
                {
                    dup2(file_fd_out, STDOUT_FILENO);
                    close(file_fd_out);
                }
            }
            
            close(p[0]);
            int curr = 0;
            while (curr < pathLength) {
                if (execvp(str_concat(PATH[curr], (*cmd)[0]), *cmd) < 0)
                    curr ++;
            }
            exit(1);
        }
        else{
            wait(NULL);
            close(p[1]);
            fd_in = p[0];
            cmd++;
        }
    }
    if (redirection_in_pipe == 1)
    {
        dup2(save_in, STDIN_FILENO);
        close(save_in);
    }
    if (redirection_out_pipe == 1)
    {
        dup2(save_out, STDOUT_FILENO);
        close(save_out);
    }
}

void count_pipes()
{
    int counter = 0;
    for (int i=0; argVector[i]!=NULL; i++)
    {
       if (strcmp(argVector[i], "|") == 0)
           counter ++;
    }
    if (counter == 0)
        noinit_cmd = true;
    else
        commands = initTripleGlobalCharPtr(counter + 2, 20);
    return;
}

int pipe_filter()
{
    int a = 0;

    // Get the number of pipe characters
    count_pipes();

    for (int i=0; argVector[i] != NULL; i++)
    {
        pipeargCount ++;
        if (strcmp(argVector[i], "|") == 0)
        {
            if (i > 2 && strcmp(argVector[i-2], "<") == 0)
            {
                if (redirection_in_pipe == 0)
                    redirection_in_pipe = 1;
                else
                {
                    printf("myShell: Multiple Indirection Operators with Pipes\n");
                    badRedirectpipe = 1;
                    break;
                }

            }

            if (lastPipe == 0)
            {
                for (a=0; a<i; a++)
                {
                    commands[pipeCount][a] = (char*) realloc (commands[pipeCount][a], strlen(argVector[a])+1);    
                    strcpy(commands[pipeCount][a], argVector[a]); 
                }
                // Make NULL terminated command
                for(int a = i; a<20; a++)
                    free(commands[pipeCount][a]);
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
                for(int a = i-lastPipe-1; a>=0 && a<20; a++)
                    free(commands[pipeCount][a]);
                commands[pipeCount][i-lastPipe-1] = 0;

                // Update Pipe location
                lastPipe = i;
                pipeCount++;
           }
       }
    }
    return a;
}

void execute_child(int pathLength)
{
    // The child must receive the previously blocked signals
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
    

    // Create the signal array for blocking/unblocking
    // The main process needs to block, while the children can unblock
    // the signals

    // We need to find the Current Directory before printing the prompt
    char* s = (char*) malloc (300*sizeof(char));
    replaceSubstring(getcwd(s, 300));
    free(s);
	
    //printPrompt();

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
        
        if (builtin(argVector) == 1)
        {
            // Execute a builtin command
            freeArgVector();
            return;
        }
        
        if (strcmp(argVector[count], "&") == 0)
        {
            // Needs to be a Background process
            isBackground = true;
            bgcount = count;
            for(int i=count; i<argLength; i++)
                free(argVector[i]);
            argVector[count] = 0;
        }

        int a = pipe_filter();

        if (badRedirectpipe == 1)
        {
            // Go back to the start of the REPL
            freeArgVector();
            if (noinit_cmd == false)
                freeTripleGlobalCharPtr(&commands, pipeCount+2, 20);
            noinit_cmd = false;
            lastPipe = 0;
            return;
        }
        
        if (pipeCount > 0)
        {
            freeArgVector();
            if (noinit_cmd == false)
                freeTripleGlobalCharPtr(&commands, pipeCount+2, 20);
            noinit_cmd = false;
            lastPipe = 0;
            return;
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
        }
    }
	return;
}

char msg[5000];

void *recvmg(void *my_sock)
{
    int sock = *((int *)my_sock);
    int len;
    initStuff();

    // client thread always ready to receive message
    while((len = recv(sock,msg,5000,0)) > 0) {
        msg[len] = '\0';
        printf("%s", msg);
        int save_out = dup(STDOUT_FILENO);
        //int save_sock = dup(sock);
        //close(STDOUT_FILENO);
        dup2(sock, STDOUT_FILENO);
        REPL(msg);
        dup2(save_out, STDOUT_FILENO);
        close(save_out);
        //dup2(save_sock, sock);
        //close(save_sock);
        bzero(msg, 5000);
        //fputs(msg,stdout);
    }
}

int main(int argc,char *argv[]){

    if (argc != 3) {
        printf("Format : ./client NUMBER IP_ADDRESS\n");
        exit(0);
    }
    // Initialise stuff ...
	pathLength = setPATH(".myshellrc");

    pthread_t recvt;
    int len;
    int sock;
    char send_msg[500];
    struct sockaddr_in ServerIp;
    char client_name[100];
    char* IP_ADDRESS = argv[2];
    strcpy(client_name, argv[1]);
    sock = socket( AF_INET, SOCK_STREAM,0);
    ServerIp.sin_port = htons(8080);
    ServerIp.sin_family= AF_INET;
    ServerIp.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    if( (connect( sock ,(struct sockaddr *)&ServerIp,sizeof(ServerIp))) == -1 )
        printf("\n connection to socket failed \n");
    else {
        printf("Connected to the Server!\n");
    }
    
    //creating a client thread which is always waiting for a message
    pthread_create(&recvt,NULL,(void *)recvmg,&sock);
    
    //ready to read a message from console
    while(fgets(msg,500,stdin) > 0) {
        if (strncmp(msg, "exit", 4) == 0) {
            printf("Client exited\n");
            break;
        }
        strcpy(send_msg,client_name);
        strcat(send_msg,":");
        strcat(send_msg,msg);
        len = write(sock,send_msg,strlen(send_msg));
        if(len < 0) 
            printf("\n message not sent \n");
        // Assume single digit maximum number of clients. We cn match only the 'n' and the dot, and apply similar logic for more
        if (msg[0] != '\0' && msg[1] != '\0' && msg[0] == 'n' && msg[2] == '.') {
            continue;
        }
        else if (strncmp(msg, "nodes", 5) == 0) {
            continue;
        }
        printPrompt();
        REPL(send_msg + strlen(client_name) + 1);
    }
    
    //thread is closed
    //pthread_join(recvt,NULL);
    shutdown(sock, SHUT_RDWR);
    close(sock);
    return 0;
}

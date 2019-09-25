#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

/* message structure */
struct message {
    long mtype;
    char mtext[8192];
};

int perform_vote()
{
    /* Performs a random Coin Toss and prints the result */
    srand(time(NULL) ^ getpid());
    int result = rand() % 2;
    return result;
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

int main(int arg, char* argv[])
{
    /* create message queue */
    int msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (msqid == -1) {
        perror("msgget");

        return EXIT_FAILURE;
    }

    int num_children = atoi(argv[1]);

    if (num_children < 0)
    {
        exit(1);
    }

    int* votes = (int*) malloc (sizeof(int));

    struct message message;
    message.mtype = 23;
    memset(&(message.mtext), 0, 8192 * sizeof(char));
    char* buffer = (char*) malloc (sizeof(char));
    while(1) {
        buffer = lineget();
        for(int i=0; buffer[i]!='\0'; i++)
            if (buffer[i] == '\n')
                buffer[i] = '\0';
        strcpy(message.mtext, buffer); 
        if (strcmp(message.mtext, "exit") == 0)
            break;
        
        if (strcmp(message.mtext, "vote") == 0)
        {
            printf("\n");
            for(int i=0; i<num_children; i++)
            {
                if (fork() == 0)
                {
                    int result = perform_vote();
                    printf("Vote for Child %d is %d\n", i, result);
                    if (result == 0)
                    {
                        strcpy(message.mtext, "0");
                    }
                    else if (result == 1)
                    {
                        strcpy(message.mtext, "1");
                    }
                    msgsnd(msqid, &message, sizeof(long) + (strlen(message.mtext) + 1), 0);
                    exit(0);

                }
                else
                {
                    msgrcv(msqid, &message, 8192, 0, 0);
                    votes[i] = atoi(message.mtext);
                    //printf("Parent received message.mtext = %s\n", message.mtext);                    
                }
                
            }

            printf("\n");
            int one_count = 0; 
            int zero_count = 0;
            for(int i=0; i<num_children; i++)
            {
                printf("Child %d's response is %d\n", i, votes[i]);
                if (votes[i] == 1)
                {
                    one_count ++;
                }
                else
                {
                    zero_count ++;
                }
                

            }
            printf("\n");
            printf("Printing Decision...\n");
            if (one_count > zero_count)
                printf("Decision : One is the majority\n");
            else if (one_count < zero_count)
                printf("Decision : Zero is the majority\n");
            else
            {
                printf("Decision : One and Zero are equal\n");
            }
            printf("\n");
            
        }
    }
            

    /* fork a child process */
    /* parent */
    /* send message to queue */

    /* destroy message queue */
    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
        perror("msgctl (Parent)");

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

#include "load_balancer.h"

//Some Global Variables that I needed
Node* head;
int counter = 0;
int fd;
const char* filename;
int j = 0;
int no_of_children;
int* cpid;
static volatile int child_pid = -1;

//User define signal handler
static void sig_usr1(int);
static void sig_usr2(int signo, siginfo_t* info, void* context);

static void sig_usr1(int signo)
{
    //Now the child process waits for reading the Filename
    
    //Block SIGUSR1 until it's complete
    signal(SIGUSR1, SIG_IGN);
    printf("Child no %d is reading now.\n\n",getpid());
    fd = open(filename, O_RDONLY | O_CREAT);
    
    char buf = 'a';
    int k=0;
    char* op = (char*) malloc (255*sizeof(char));

    while(read (fd, &buf, 1))
    {
        if (buf == '\n')
        {
            op[k] = '\0';
            break;
        }

        else
        {
            op[k++] = buf;
        }
    }
    //Now wait for a second and then send a signal
    sleep(1);
    //Print the contents of the buffer via op
    printf("Output: %s\n\n", op);


    //Now unblock the signal
    printf("%d sending signal to parent now...\n\n", getpid());
    kill(getppid(), SIGUSR2);
    signal(SIGUSR1, sig_usr1);
}

static void sig_usr2(int signo, siginfo_t* info, void* context)
{
    if (signo == SIGUSR2)
    {
        child_pid = info->si_pid;
        printf("Parent Received SIGUSR2. Child Process with PID %d is now free\n\n", child_pid);
    }
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("Error : Must specify 2 arguments.");
        exit (-1);
    }
    
    //Filename is the first argument
    filename = argv[1];

    //Number of Child Processes to be spawned
    no_of_children = atoi(argv[2]);
    
    if (no_of_children < 1)
    {
        printf("Error : Number of Child Processes must be a positive integer.");
        exit (-1);
    }

    cpid = (int*) malloc (no_of_children*sizeof(int));

    //Create a sigaction() handler for SIGUSR2
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sig_usr2;
    sigaction(SIGUSR2, &sa, NULL);
    
    //Create no_of_children children
    for(int i=0; i<no_of_children; i++)
    {
        cpid[i] = fork();
        if (cpid[i] == 0)
        {
            //Inside a child
            //printf("Created %dth child process", i); 
            //printf(" with Process ID = %d\n", getpid());            
            signal(SIGUSR1, sig_usr1);
            
            while(1)
            {
                pause();
                signal(SIGUSR1, sig_usr1);
            }
            
            //Every child process must exit so control goes back to the parent
            exit(0);
        }
    }
    
    //Returns to the parent process
    for(int i=0; i<no_of_children; i++)
    {
        if (i == 0)
        {
            head = create_queue(cpid[i]);
        }

        else
        {
            head = push(head, cpid[i]);
        }
    }
    print_queue(head);
    while(1)
    {
        kill(head->front->pid, SIGUSR1);
        head = shift_queue(head);
    }

    return 0;
}

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <string.h>

int main(void)
{
    int key = ftok("shmfile", 65);
    int shmid = shmget(key, 1024, 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmid");
        return EXIT_FAILURE;
    }


    /* fork a child process */
    pid_t pid = fork();
    if (pid == 0) {
        /* child */

        char* str = (char*) shmat(shmid, (void*)0,0);
        gets(str);
        //(void)strcpy(message.mtext, "Hello parent!");
        shmdt(str);
        exit(0);

        }
    else {
        /* parent */

        /* wait for child to finish */
        (void)waitpid(pid, NULL, 0);

        /* receive message from queue */
        char* str = (char*) shmat(shmid, (void*)0,0);

        printf("%s\n", str);
        shmdt(str);
    }

    shmctl(shmid, IPC_RMID, NULL);

    return EXIT_SUCCESS;
}


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main ()
{
    pid_t pid = fork();

    if (pid == -1) {
        printf("[ERROR] Fork error!\n");
    } else if (pid == 0) {
        printf("Child\n");
        printf("Waiting for input: %c\n", getchar());
        /*
        if (execvp(args[0], args) == -1) {
            printf("%s: command not found\n", args[0]);
        }
        */

        exit(2);
    } else {
        printf("Parent and child is %d\n", pid);
        int status;
        wait(&status);
        printf("Done waiting and status is %d\n", status);
    }

    return 0;
}


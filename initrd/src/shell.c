
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h> 

void main(int argc, char* argv[]) {

    (void) argc;
    (void) argv;

    while (true) {

        printf(" $ ");
        char command[6];
        //int proc;

        int c = read(stdin, command, 5);
        command[5] = '\0';

        printf("%s with %d\n", command, c);
    }

}




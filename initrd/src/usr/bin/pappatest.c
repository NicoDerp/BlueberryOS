
#include <stdio.h>
#include <unistd.h>


int main(int argc, char* argv[]) {

    (void) argc;
    (void) argv;

    char* str = (char*) 123;
    write(STDOUT_FILENO, str, 10);

    return 0;
}



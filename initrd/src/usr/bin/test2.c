
#include <stdio.h>
#include <string.h>


/*
char* prev;

char* mystrtok(char* str, const char* delim) {

    if (str != (char*) 0) {
        prev = str;
    }

    //printf("prev: 0x%x, str: 0x%x\n", prev, str);
    //printf("prev: %d\n", (unsigned int) &prev);
    for (unsigned int i = 0; prev[i] != 0; i++) {
        //printf("Testing character '%c'\n");
        if (strchr(delim, prev[i]) != 0) {
            prev[i] = '\0';
            char* ret = prev;
            prev += i + 1;
            return ret;
        }
    }

    return (char*) 0;
}
*/


void main ()
{

    char str[] = "/bin/test ooga booga dooga";
    char* tok;
    printf("Splitting string \"%s\" into tokens:\n", str);
    tok = strtok(str, " ");

    while (tok != NULL)
    {
        printf("Got token: %s\n", tok);
        printf("str: %s\n", str);
        getchar();
        tok = strtok(NULL, " ");
    }
}


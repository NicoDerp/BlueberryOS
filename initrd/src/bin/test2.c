
#include <stdio.h>
#include <string.h>

char* prev;

char* mystrtok(char* str, const char* delim) {

    if (str != (char*) 0) {
        prev = str;
    }
    printf("a");

    //printf("prev: 0x%x, str: 0x%x\n", prev, str);
    //printf("prev: %d\n", (unsigned int) &prev);
    for (unsigned int i = 0; prev[i] != 0; i++) {
        printf("Testing character '%c'\n");
        if (strchr(delim, prev[i]) != 0) {
            prev[i] = '\0';
            prev = prev + i + 1;
            return prev;
        }
    }

    return (char*) 0;
}
void main ()
{

    char str[] = "/bin/test ooga booga dooga";
    char* tok;
    printf ("Splitting string \"%s\" into tokens:\n", str);
    tok = mystrtok(str," ,.-");
    //printf("Got token %d\n", tok);
    /*
    while (pch != NULL)
    {
        printf ("%s\n",pch);
        printf ("%s\n",str);
        pch = strtok (NULL, " ,.-");
    }
    */
}


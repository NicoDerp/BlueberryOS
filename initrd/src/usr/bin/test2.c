
#include <stdio.h>
#include <string.h>

void main ()
{

    /*
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
    */

    const char* str = "Hello test this is a cool test :)";
    const char* sub = "test";
    char* s = strrstr(str, sub);
    if (s == NULL)
        printf("S not found\n");
    else
        printf("S is '%s'\n", s);
}


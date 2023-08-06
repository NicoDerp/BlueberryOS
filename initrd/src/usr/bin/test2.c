
#include <stdio.h>
#include <string.h>


int main(void) {

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

    char src[100] = " kul!";
    char dst[100] = "jeg er";
    strcat(dst, src);
    printf("dst is '%s'\n", dst);
    printf("Src is '%s'\n", src);

    /*
    const char* str = ".myfile";
    char c = '.';
    char* s = strrchr(str, c);
    if (s == NULL)
        printf("S not found\n");
    else
        printf("S is '%s'\n", s);
    */
}



#include <stdio.h>
#include <string.h>

int main ()
{
    char str[] ="/bin/test ooga booga dooga";
    char* pch;
    printf ("Splitting string \"%s\" into tokens:\n",str);
    pch = strtok (str," ,.-");
    while (pch != NULL)
    {
        printf ("%s\n",pch);
        printf ("%s\n",str);
        pch = strtok (NULL, " ,.-");
    }
    return 0;
}


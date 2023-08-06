
#include <stdio.h>
#include <bits/types/struct_FILE.h>


FILE _stdin;
FILE _stdout;
FILE _stderr;

FILE* stdin = &_stdin;
FILE* stdout = &_stdout;
FILE* stderr = &_stderr;


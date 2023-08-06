
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>


#define MAX_LINE_LENGTH 128
#define HISTORY_SIZE    64
#define MAX_ARGS        32


#define PS1 "\e[2K\e[40;46m%s\e[0m:\e[39;46m%s\e[0m$ "
#define PS1ARGS pwd.pw_name, cwd


char cmd[MAX_LINE_LENGTH+1];
char history[MAX_LINE_LENGTH][HISTORY_SIZE];
size_t historyCount = 0;
char cwd[256];

struct passwd pwd;
char pwdBuf[256];

void execArgs(char** args) {

    pid_t pid = fork();

    if (pid == -1) {
        int backup = errno;
        printf("shell: fork error: %s\n", strerror(backup));
    } else if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            int backup = errno;
            if (backup == ENOENT)
                printf("shell: %s: command not found\n", args[0]);
            else if (backup == EACCES)
                printf("shell: %s: permission denied\n", args[0]);
            else
                printf("shell: %s: %s\n", args[0], strerror(backup));
            exit(1);
        }

        exit(0);
    } else {
        int status;
        wait(&status);
    }
}

void cmdExit(unsigned int argCount, char** parsedArgs) {

    (void) parsedArgs;

    if (argCount == 1) {
        exit(0);
    }

    else if (argCount == 2) {
        printf("shell: not supported yet\n");
        //exit(0);
    }

    else {
        printf("shell: exit: too many arguments\n");
    }
}

void cmdCd(unsigned int argCount, char** parsedArgs) {

    if (argCount == 1) {
        // Should actually go to home
        printf("shell: cd: too few arguments\n");
    }

    else if (argCount == 2) {

        int status = chdir(parsedArgs[1]);
        if (status == -1) {
            int backup = errno;
            printf("shell: cd: %s: %s\n", parsedArgs[1], strerror(backup));
        } else {
            setenv("PWD", parsedArgs[1], true);

            if (getcwd(cwd, sizeof(cwd)) == NULL) {
                int backup = errno;
                printf("shell: cd: getcwd error: %s\n", strerror(backup));
            }
        }

    }

    else {
        printf("shell: cd: too many arguments\n");
    }
}

void cmdExport(unsigned int argCount, char** parsedArgs) {

    if (argCount == 1) {
        printf("shell: export: too few arguments\n");
    }

    else if (argCount == 2) {

        // Seperate by '='
        char* tok = strchr(parsedArgs[1], '=');

        // If tok is NULL, then value is ""
        if (tok == NULL) {
            tok = "";
        }
        else {
            *tok = '\0';
            tok++;
        }

        if (setenv(parsedArgs[1], tok, true) != 0)
            printf("shell: export: setenv error\n");
    }

    else if (argCount == 3) {
        if (strcmp(parsedArgs[1], "-n") == 0) {

            if (unsetenv(parsedArgs[2]) != 0)
                printf("shell: export: unsetenv error\n");

        } else {
            printf("shell: export: invalid flag '%s'\n", parsedArgs[1]);
        }
    }

    else {
        printf("shell: export: too many arguments\n");
    }
}

/*
void main(int argc, char* argv[]) {

    (void) argc;
    (void) argv;
*/
int main(void) {


    struct passwd* tmpPwdPtr;
    int error;

    if ((error = getpwuid_r(getuid(), &pwd, pwdBuf, sizeof(pwdBuf), &tmpPwdPtr)) != 0) {
        printf("shell: getpwuid error: %d:%s\n", error, strerror(error));
        exit(1);
    }

    setenv("USER", pwd.pw_name, true);
    setenv("HOME", pwd.pw_dir, true);
    setenv("SHELL", pwd.pw_shell, true);

    int status = chdir(pwd.pw_dir);
    if (status == -1) {
        int backup = errno;
        printf("shell: cd: %s: %s\n", pwd.pw_dir, strerror(backup));
    }

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        int backup = errno;
        printf("shell: cd: getcwd error: %s\n", strerror(backup));
    }

    while (true) {

        printf(PS1, PS1ARGS);

        char c;
        size_t index = 0;
        size_t cursor = 0;
        size_t historyScroll = 0;
        while ((c = getchar()) != '\n') {
            if (index >= MAX_LINE_LENGTH) {
                printf("shell: reached max line length\n");
                continue;
            }

            // Special character
            if (c == '\e') {
                c = getchar();

                // Left arrow
                if (c == 27) {
                    if (cursor == 0)
                        continue;

                    printf("\e[1D");
                    cursor--;
                }

                // Right arrow
                else if (c == 26) {
                    if (cursor >= index)
                        continue;

                    printf("\e[1C");
                    cursor++;
                }

                // Up arrow
                else if (c == 24) {
                    if (historyScroll == historyCount)
                        continue;

                    historyScroll++;

                    /*
                    for (size_t i = 0; i < index; i++) {
                        printf("\b \b");
                    }
                    */

                    strcpy(cmd, history[historyCount - historyScroll]);
                    printf(PS1"%s", PS1ARGS, cmd);

                    size_t len = strlen(cmd);
                    index = len;
                    cursor = len;
                }

                // Down arrow
                else if (c == 25) {
                    if (historyScroll == 0) {

                        for (size_t i = 0; i < index; i++) {
                            printf("\b \b");
                        }

                        index = 0;
                        cursor = 0;

                    } else {
                        historyScroll--;

                        /*
                        for (size_t i = 0; i < index; i++) {
                            printf("\b \b");
                        }
                        */

                        size_t len = strlen(history[historyCount - historyScroll]);
                        memcpy(cmd, history[historyCount - historyScroll], len);
                        cmd[len] = '\0';
                        printf(PS1"%s", PS1ARGS, cmd);

                        index = len;
                        cursor = len;
                    }

                }
            }

            // Backspace
            else if (c == '\b') {
                if (index != 0) {
                    index--;
                    cursor--;
                    printf("\b \b");
                }
            }

            // Tab (autocomplete)
            else if (c == '\t') {
            }

            else {
                historyScroll = 0;

                if (index == cursor) {
                    cmd[index++] = c;
                    cursor++;
                    putchar(c);
                } else {
                    for (size_t j = 0; j < index-cursor; j++) {
                        cmd[index-j+1] = cmd[index-j];
                    }
                    cmd[cursor] = c;
                    cursor++;
                    index++;

                    printf("%s", cmd+cursor-1);
                    for (size_t j = 0; j < index-cursor; j++) {
                        putchar('\b');
                    }
                }
            }
        }
        cmd[index] = '\0';

        if (index != 0) {
            if (historyCount >= HISTORY_SIZE) {
                printf("shell: max history reached\n");
            }
            else if(historyCount == 0 || strcmp(history[historyCount-1], cmd) != 0) {
                strcpy(history[historyCount++], cmd);
            }
        }

        putchar('\n');

        char* parsedArgs[MAX_ARGS+1];
        unsigned int argCount = 0;
        char* tok;

        tok = strtok(cmd, " ");

        while (tok != NULL)
        {
            if (argCount == MAX_ARGS) {
                printf("shell: reached max arguments\n");
                break;
            }

            parsedArgs[argCount++] = tok;
            tok = strtok(NULL, " ");
        }
        parsedArgs[argCount] = NULL;

        if (argCount == 0) {
            continue;
        }
        else if (strcmp(parsedArgs[0], "exit") == 0) {
            cmdExit(argCount, parsedArgs);
        }
        else if (strcmp(parsedArgs[0], "clear") == 0) {
            printf("\e[2J");
        }
        else if (strcmp(parsedArgs[0], "cd") == 0) {
            cmdCd(argCount, parsedArgs);
        }
        else if (strcmp(parsedArgs[0], "export") == 0) {
            cmdExport(argCount, parsedArgs);
        }
        else {
            execArgs(parsedArgs);
        }
    }

    return 0;
}



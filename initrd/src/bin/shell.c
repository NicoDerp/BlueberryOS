
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <sys/wait.h>
#include <unistd.h>


#define MAX_LINE_LENGTH 128
#define HISTORY_SIZE    8
#define MAX_ARGS        8


void execArgs(char** args) {

    pid_t pid = fork();

    if (pid == -1) {
        printf("[ERROR] Fork error!\n");
    } else if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            printf("%s: command not found\n", args[0]);
            return;
        }

        exit(0);
    } else {
        int status;
        wait(&status);

    }
}

char cmd[MAX_LINE_LENGTH+1];
char history[MAX_LINE_LENGTH][HISTORY_SIZE];
size_t historyCount = 0;

/*
void main(int argc, char* argv[]) {

    (void) argc;
    (void) argv;
*/
void main() {


    while (true) {

        printf(" $ ");

        char c;
        size_t index = 0;
        size_t cursor = 0;
        size_t historyScroll = 0;
        while ((c = getchar()) != '\n') {
            if (index >= MAX_LINE_LENGTH) {
                printf("[ERROR] Reached max line length\n");
                continue;
            }

            // Backspace
            if (c == '\b') {
                if (index != 0) {
                    index--;
                    cursor--;
                    printf("\b \b");
                }
            }

            // Left arrow
            else if (c == 27) {
                if (cursor != 0) {
                    putchar(27);
                    cursor--;
                }
            }

            // Right arrow
            else if (c == 26) {
                if (cursor < index) {
                    putchar(26);
                    cursor++;
                }
            }

            // Up arrow
            else if (c == 24) {
                if (historyScroll != historyCount) {
                    historyScroll++;

                    for (size_t i = 0; i < index; i++) {
                        printf("\b \b");
                    }

                    strcpy(cmd, history[historyCount - historyScroll]);
                    printf("%s", cmd);

                    size_t len = strlen(cmd);
                    index = len;
                    cursor = len;
                }
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

                    for (size_t i = 0; i < index; i++) {
                        printf("\b \b");
                    }

                    size_t len = strlen(history[historyCount - historyScroll]);
                    memcpy(cmd, history[historyCount - historyScroll], len);
                    cmd[len] = '\0';
                    printf("%s", cmd);

                    index = len;
                    cursor = len;
                }

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

                    printf(cmd+cursor-1);
                    for (size_t j = 0; j < index-cursor; j++) {
                        putchar('\b');
                    }
                }
            }
        }
        cmd[index] = '\0';

        if (index != 0) {
            if (historyCount >= HISTORY_SIZE) {
                printf("Max history reached\n");
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
                printf("Reached max args\n");
                break;
            }

            parsedArgs[argCount++] = tok;
            tok = strtok(NULL, " ");
        }

        parsedArgs[argCount] = NULL;


        if (argCount == 0) {
            continue;
        } else if (strcmp(parsedArgs[0], "exit") == 0) {

            if (argCount == 1) {
                exit(0);
            } else if (argCount == 2) {
                printf("");
                exit(0);
            } else {
                printf("exit: too many arguments\n");
            }

        } else if (strcmp(parsedArgs[0], "clear") == 0) {

        } else if (strcmp(parsedArgs[0], "echo") == 0) {

            for (size_t i = 0; i < argCount; i++) {
                printf("%s", parsedArgs[i]);
            }
            putchar('\n');

        } else if (strcmp(parsedArgs[0], "cd") == 0) {

            if (argCount == 1) {
                // Should actually go to home
                printf("too few arguments\n");
            } else if (argCount == 2) {

                int status = chdir(parsedArgs[1]);
                if (status == -1) {
                    printf("error: chdir failed\n");
                }

            } else {
                printf("too many arguments\n");
            }

        } else {
            execArgs(parsedArgs);
        }
    }

}




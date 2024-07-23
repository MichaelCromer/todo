#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

void print_all(char *fpath, int max_lines);
void print_done(char *fpath, int max_lines);
void print_todo(char *fpath, int max_lines);

int atoi_pedantic(char *str);

char *TODO_PATH = "~/.todo";

int main(int argc, char *argv[])
{
    if (argc < 2) {
        return EXIT_SUCCESS;
    }

    if (argc == 2) {
        // help flag
        // edit flag
    }

    if (argc == 3) {
        int arg = atoi_pedantic(argv[2]);
        printf("Flag was %s and arg was %d\n", argv[1], arg);
        if (arg == 0) {
            // TODO error msg
            return EXIT_FAILURE;
        }

        if ((strcmp(argv[1], "-a") == 0) || (strcmp(argv[1], "--all") == 0)) {
            print_all(TODO_PATH, arg);
        } 
        else if ((strcmp(argv[1], "-d") == 0) || (strcmp(argv[1], "--done") == 0)) {
            print_done(TODO_PATH, arg);
        } 
        else if ((strcmp(argv[1], "-t") == 0) || (strcmp(argv[1], "--todo") == 0)) {
            print_todo(TODO_PATH, arg);
        } 
        else {
            return EXIT_SUCCESS;
        }
        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;
}

void print_all(char *fpath, int max_lines)
{
    print_todo(fpath, max_lines);
    print_done(fpath, max_lines);
}

void print_todo(char *fpath, int max_lines) 
{
    // TODO implement
    printf("\n");
    printf("Printing %d todos from %s\n", max_lines, fpath);
}

void print_done(char *fpath, int max_lines) 
{
    // TODO implement
    printf("\n");
    printf("Printing %d dones from %s\n", max_lines, fpath);
}

int atoi_pedantic(char *str)
{
    int result = 0;
    for (int i=0; str[i] != '\0'; i++) {
        if (!isdigit((unsigned char) str[i])) { 
            return 0;
        }
        result = 10*result + (str[i] - '0');
    }
    return result;
}

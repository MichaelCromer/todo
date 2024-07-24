#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

void print_all(char *fpath, int max_lines);
void print_done(char *fpath, int max_lines);
void print_todo(char *fpath, int max_lines);
char *todo_path();
int atoi_pedantic(char *str);

int main(int argc, char *argv[])
{

    if (argc < 2) {
        // TODO default print behaviour
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
            // TODO error handling
            printf("Error: argument must be a positive integer\n");
            return EXIT_FAILURE;
        }

        if ((strcmp(argv[1], "-a") == 0) || (strcmp(argv[1], "--all") == 0)) {
            print_all(todo_path(), arg);
        }
        else if ((strcmp(argv[1], "-d") == 0) || (strcmp(argv[1], "--done") == 0)) {
            print_done(todo_path(), arg);
        }
        else if ((strcmp(argv[1], "-t") == 0) || (strcmp(argv[1], "--todo") == 0)) {
            print_todo(todo_path(), arg);
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
    FILE *fptr;
    char line[LINE_MAX];
    size_t len;
    int i = 1;

    if ((fptr = fopen(fpath, "r")) == NULL) {
        printf("Error finding todo file\n");
        return;
    }

    while ((i <= max_lines) && (fgets(line, LINE_MAX, fptr))) {
        len = strlen(line);
        if (len < 3) {
            continue;
        }
        if (line[0] == '[' && line[1] == ' ' && line[2] == ']') {
            printf("\t%d\t%s", i, line);
            i++;
        }
    }
}

void print_done(char *fpath, int max_lines)
{
    // TODO implement
    printf("\n");
    printf("Printing %d dones from %s\n", max_lines, fpath);
}

char *todo_path()
{
    char *pathbuf = malloc(PATH_MAX * sizeof(*pathbuf));
    char *homedir = getenv("HOME");
    if (homedir == NULL) {
        // TODO error handling
        printf("HOME environment variable not set\n");
        return NULL;
    }
    int chars_written = snprintf(pathbuf, PATH_MAX, "%s/.todo", homedir);
    pathbuf = realloc(pathbuf, (chars_written + 1)*sizeof(*pathbuf));
    return pathbuf;
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

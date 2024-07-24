#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

void print_all(char *fpath, int max_lines);
void print_todo(char *fpath, int max_lines);
void print_done(char *fpath, int max_lines);
void edit_todo_file(char *fpath);
char *todo_path();
int atoi_pedantic(char *str);

int main(int argc, char *argv[])
{
    char *todofile = todo_path();
    if (argc < 2) {
        print_todo(todofile, 10);
        return EXIT_SUCCESS;
    }

    if (argc == 2) {
        // help flag
        if ((strcmp(argv[1], "-E") == 0) || (strcmp(argv[1], "--edit") == 0)) {
            edit_todo_file(todofile);
        }
    }

    if (argc == 3) {
        char *flag = argv[1];
        int num_arg = atoi_pedantic(argv[2]);
        if (num_arg == 0) {
            // TODO error handling
            printf("Error: argument must be a positive integer\n");
            return EXIT_FAILURE;
        }

        if ((strcmp(flag, "-A") == 0) || (strcmp(flag, "--print-all") == 0)) {
            print_all(todofile, num_arg);
        }
        else if ((strcmp(flag, "-D") == 0) || (strcmp(flag, "--print-done") == 0)) {
            print_done(todofile, num_arg);
        }
        else if ((strcmp(flag, "-T") == 0) || (strcmp(flag, "--print-todo") == 0)) {
            print_todo(todofile, num_arg);
        }
        else {
            return EXIT_SUCCESS;
        }
        return EXIT_SUCCESS;
    }

    free(todofile);
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
    fclose(fptr);
}

void print_done(char *fpath, int max_lines)
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
        if (line[0] == '[' && line[1] == 'X' && line[2] == ']') {
            printf("\t%d\t%s", i, line);
            i++;
        }
    }
    fclose(fptr);
}

void edit_todo_file(char *fpath)
{
    char *editor = getenv("EDITOR");
    if (editor == NULL) {
        editor = "vi";
    }
    char command[PATH_MAX + 64];
    snprintf(command, sizeof(command), "%s %s", editor, fpath);
    system(command);
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

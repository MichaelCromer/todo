#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>

#define TODO_VERSION "1.0"

void print_help();
void print_version();
void print_all(char *fpath, int max_lines);
void print_todo(char *fpath, int max_lines);
void print_done(char *fpath, int max_lines);
void edit_todo_file(char *fpath);
void mark_done(char *fpath, int item_num);
void mark_todo(char *fpath, int item_num);
void add_item(char *fpath, char *item);
bool line_is_todo(char *line);
bool line_is_done(char *line);
char *todo_path();
char *concat_args(int argc, char *argv[]);
int atoi_pedantic(char *str);

int main(int argc, char *argv[])
{
    char *todofile = todo_path();
    char *flag;
    int num_arg;

    if (argc < 2) {
        print_todo(todofile, 10);
        return EXIT_SUCCESS;
    }

    flag = argv[1];

    if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)) {
        print_help();
        return EXIT_SUCCESS;
    }

    if ((strcmp(argv[1], "-e") == 0) || (strcmp(argv[1], "--edit") == 0)) {
        edit_todo_file(todofile);
        return EXIT_SUCCESS; 
    }

    if ((strcmp(argv[1], "-v") == 0) || (strcmp(argv[1], "--version") == 0)) {
        print_version();
        return EXIT_SUCCESS;
    }

    if ((strcmp(flag, "-a") == 0) || (strcmp(flag, "--print-all") == 0)) {
        num_arg = atoi_pedantic(argv[2]);
        print_all(todofile, num_arg);
    }
    else if ((strcmp(flag, "-d") == 0) || (strcmp(flag, "--print-done") == 0)) {
        num_arg = atoi_pedantic(argv[2]);
        print_done(todofile, num_arg);
    }
    else if ((strcmp(flag, "-t") == 0) || (strcmp(flag, "--print-todo") == 0)) {
        num_arg = atoi_pedantic(argv[2]);
        print_todo(todofile, num_arg);
    }
    else if ((strcmp(flag, "-x") == 0) || (strcmp(flag, "--done") == 0)) {
        num_arg = atoi_pedantic(argv[2]);
        mark_done(todofile, num_arg);
    }
    else if ((strcmp(flag, "-o") == 0) || (strcmp(flag, "--todo") == 0)) {
        num_arg = atoi_pedantic(argv[2]);
        mark_todo(todofile, num_arg);
    }
    else {
        char *message = concat_args(argc, argv);
        add_item(todofile, message);
        free(message);
    }

    free(todofile);
    return EXIT_SUCCESS;
}

void print_help()
{
    printf("Usage: todo [OPTION|OPTION N|MESSAGE]\n");
    printf("A command line tool to manage todo items\n");
    printf("\n");

    printf("  Option:\n");
    printf("\t-h --help\tDisplay this message and exit\n");
    printf("\t-e --edit\tOpen the todo file in $EDITOR\n");
    printf("\t-v --version\tPrint the current version\n");
    printf("\n");

    printf("  Option N:\n");
    printf("\t-x --done\tMark the Nth todo item as done\n");
    printf("\t-o --todo\tMark the Nth done item as todo\n");
    printf("\t-t --print-todo\tDisplay the first N todo items\n");
    printf("\t-d --print-done\tDisplay the first N done items\n");
    printf("\t-a --print-all\tDisplay the first N of all items\n");
    printf("\n");

    printf("Any arguments not matching OPTION [N] create a new todo item\n");
    printf("If no arguments are supplied, defaults to \"todo -t 10\"\n");
    printf("\n");
}

void print_version()
{
    printf("todo version %s\n", TODO_VERSION);
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
    int i = 1;

    if ((fptr = fopen(fpath, "r")) == NULL) {
        printf("Error finding todo file\n");
        return;
    }

    while ((i <= max_lines) && (fgets(line, LINE_MAX, fptr))) {
        if (line_is_todo(line)) {
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
    int i = 1;

    if ((fptr = fopen(fpath, "r")) == NULL) {
        printf("Error finding todo file\n");
        return;
    }

    while ((i <= max_lines) && (fgets(line, LINE_MAX, fptr))) {
        if (line_is_done(line)) {
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

void mark_done(char *fpath, int item_num)
{
    FILE *fptr = fopen(fpath, "r+");
    char line[LINE_MAX];
    int marker=0;

    if (fptr == NULL) {
        // TODO error handling
        printf("HOME environment variable not set\n");
        return;
    }

    while (fgets(line, LINE_MAX, fptr)) {
        if (line_is_todo(line)) {
            marker++;
            if (marker == item_num) {
                fseek(fptr, -strlen(line), SEEK_CUR);
                line[1] = 'X';
                fputs(line, fptr);
            }
        }

    }
    fclose(fptr);
}

void mark_todo(char *fpath, int item_num)
{
    FILE *fptr = fopen(fpath, "r+");
    char line[LINE_MAX];
    int marker=0;

    if (fptr == NULL) {
        // TODO error handling
        printf("HOME environment variable not set\n");
        return;
    }

    while (fgets(line, LINE_MAX, fptr)) {
        if (line_is_done(line)) {
            marker++;
            if (marker == item_num) {
                fseek(fptr, -strlen(line), SEEK_CUR);
                line[1] = ' ';
                fputs(line, fptr);
            }
        }

    }
    fclose(fptr);
}

void add_item(char *fpath, char *item)
{
    FILE *fptr = fopen(fpath, "a");

    if (fptr == NULL) {
        // TODO error handling
        printf("HOME environment variable not set\n");
        return;
    }

    fputs("[ ]", fptr);
    fputs(item, fptr);
    fputs("\n", fptr);
    fclose(fptr);
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

bool line_is_todo(char *line)
{
    return (strncmp(line, "[ ]", 3) == 0);
}

bool line_is_done(char *line)
{
    return (strncmp(line, "[X]", 3) == 0);
}

char *concat_args(int argc, char *argv[])
{
    int total_arglen = argc-1;
    for (int i=1; i < argc; i++) {
        total_arglen += strlen(argv[i]);
    }
    char *argbuf = malloc((total_arglen + 1) * sizeof(*argbuf));
    for (int i=1; i < argc; i++) {
        argbuf = strcat(argbuf, " ");
        argbuf = strcat(argbuf, argv[i]);
    }

    return argbuf;
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>

#define TODO_VERSION "1.0"

void print_help();
void print_version();
void print_all(int max_lines);
void print_todo(int max_lines);
void print_done(int max_lines);
void edit_todo_file();
void mark_done(int item_num);
void mark_todo(int item_num);
void add_item(char *item);
bool line_is_todo(char *line);
bool line_is_done(char *line);
FILE *todo_file(char *mode);
char *todo_path();
char *concat_args(int argc, char *argv[]);
int get_numeric_arg(int argc, char *argv[]);
int atoi_pedantic(char *str);

int main(int argc, char *argv[])
{
    char *flag;
    int num_arg;

    if (argc < 2) {
        print_todo(10);
        return EXIT_SUCCESS;
    }

    flag = argv[1];

    if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)) {
        print_help();
        return EXIT_SUCCESS;
    }

    if ((strcmp(argv[1], "-e") == 0) || (strcmp(argv[1], "--edit") == 0)) {
        edit_todo_file();
        return EXIT_SUCCESS; 
    }

    if ((strcmp(argv[1], "-v") == 0) || (strcmp(argv[1], "--version") == 0)) {
        print_version();
        return EXIT_SUCCESS;
    }

    if ((strcmp(flag, "-a") == 0) || (strcmp(flag, "--print-all") == 0)) {
        num_arg = get_numeric_arg(argc, argv);
        print_all(num_arg);
    }
    else if ((strcmp(flag, "-d") == 0) || (strcmp(flag, "--print-done") == 0)) {
        num_arg = get_numeric_arg(argc, argv);
        print_done(num_arg);
    }
    else if ((strcmp(flag, "-t") == 0) || (strcmp(flag, "--print-todo") == 0)) {
        num_arg = get_numeric_arg(argc, argv);
        print_todo(num_arg);
    }
    else if ((strcmp(flag, "-x") == 0) || (strcmp(flag, "--done") == 0)) {
        num_arg = get_numeric_arg(argc, argv);
        mark_done(num_arg);
    }
    else if ((strcmp(flag, "-o") == 0) || (strcmp(flag, "--todo") == 0)) {
        num_arg = get_numeric_arg(argc, argv);
        mark_todo(num_arg);
    }
    else {
        char *message = concat_args(argc, argv);
        add_item(message);
        free(message);
    }

    return EXIT_SUCCESS;
}

// send help string to stdout
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

// send version string to stdout
void print_version()
{
    printf("todo version %s\n", TODO_VERSION);
}

// list 1) todo items and 2) done items up to N items each
void print_all(int max_lines)
{
    print_todo(max_lines);
    print_done(max_lines);
}

// list N todo items
void print_todo(int max_lines)
{
    FILE *fptr = todo_file("r");
    char line[LINE_MAX];
    int i = 1;

    while ((i <= max_lines) && (fgets(line, LINE_MAX, fptr))) {
        if (line_is_todo(line)) {
            printf("\t%d\t%s", i, line);
            i++;
        }
    }
    fclose(fptr);
}

// list N done items
void print_done(int max_lines)
{
    FILE *fptr = todo_file("r");
    char line[LINE_MAX];
    int i = 1;

    while ((i <= max_lines) && (fgets(line, LINE_MAX, fptr))) {
        if (line_is_done(line)) {
            printf("\t%d\t%s", i, line);
            i++;
        }
    }
    fclose(fptr);
}

// open todo_file in $EDITOR:-vi
void edit_todo_file()
{
    char *fpath = todo_path();
    char *editor = getenv("EDITOR");
    if (editor == NULL) {
        editor = "vi";
    }
    char command[PATH_MAX + 64];
    snprintf(command, sizeof(command), "%s %s", editor, fpath);
    free(fpath);
    system(command);
}

// set the Nth todo item as done
void mark_done(int item_num)
{
    FILE *fptr = todo_file("r+");
    char line[LINE_MAX];
    int marker=0;

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

// set the Nth done item as todo
void mark_todo(int item_num)
{
    FILE *fptr = todo_file("r+");
    char line[LINE_MAX];
    int marker=0;

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

// append an appropriately formatted todo item
void add_item(char *item)
{
    FILE *fptr = todo_file("a");

    fputs("[ ]", fptr);
    fputs(item, fptr);
    fputs("\n", fptr);
    fclose(fptr);
}

// open the found todo file in the given mode
FILE *todo_file(char *mode)
{
    char *fpath = todo_path();
    FILE *fptr = fopen(fpath, mode);
    if (fptr == NULL) {
        // TODO error handling
        printf("Error: cannot open %s\n", fpath);
        exit(EXIT_FAILURE);
    }
    free(fpath);
    return fptr;
}

// return the path of the .todo file
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

// todo line should match an empty check box '[ ]' at pos 0,1,2
bool line_is_todo(char *line)
{
    return (strncmp(line, "[ ]", 3) == 0);
}

// done line should match a filled check box '[X]' at pos 0,1,2
bool line_is_done(char *line)
{
    return (strncmp(line, "[X]", 3) == 0);
}

// squash a list of strings (e.g., make args to todo into a new todo entry)
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

// safely capture a numeric argument from the parameters
int get_numeric_arg(int argc, char *argv[])
{
    if (argc < 3) {
        // TODO error handilng
        printf("Flag needs numeric argument\n");
        exit(EXIT_FAILURE);
    }

    return atoi_pedantic(argv[2]);
}

// don't want e.g., '123abc' to return 123
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

/*--------------------------------------------------------------------------------------
    todo - a command line tool to manage todo items
----------------------------------------------------------------------------------------
    mcromer, Aug 2024
========================================================================================
*/

/*--------------------------------------------------------------------------------------
    Setup
*/

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define TODO_VERSION "1.0"
#define TODO_MAX_ITEMLINES 64


/*--------------------------------------------------------------------------------------
    -h etc
*/

// send help string to stdout
void print_help()
{
    printf("Usage: todo [OPTION|OPTION N|MESSAGE]\n");
    printf("A command line tool to manage todo items\n");
    printf("\n");

    printf("  Option:\n");
    printf("\t-h --help\tDisplay this message and exit\n");
    printf("\t-e --edit\tOpen the todo file in $EDITOR\n");
    printf("\t-v --version\tDisplay the current version #\n");
    printf("\n");

    printf("  Option N:\n");
    printf("\t-x --done\tMark the Nth todo item as done\n");
    printf("\t-o --todo\tMark the Nth done item as todo\n");
    printf("\t-t --print-todo\tDisplay the first N todo items\n");
    printf("\t-d --print-done\tDisplay the first N done items\n");
    printf("\t-a --print-all\tDisplay -t N, -d N in sequence\n");
    printf("\n");

    printf("Arguments not matching [OPT|OPT N] create a new item\n");
    printf("If no arguments are supplied, default is todo -t 10\n");
}


// send version string to stdout
void print_version()
{
    printf("todo version %s\n", TODO_VERSION);
}


/*--------------------------------------------------------------------------------------
    Input
*/


// get numeric input carefully - don't want e.g., '123abc' to return 123
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


// safely capture a numeric argument from the parameters
// assumes something like 'todo -x 3'
int get_numeric_arg(int argc, char *argv[])
{
    if (argc < 3) {
        // TODO error handilng
        printf("Flag needs numeric argument\n");
        exit(EXIT_FAILURE);
    }

    return atoi_pedantic(argv[2]);
}


// squash a list of strings (e.g., make all args into a new todo entry)
char *concat_args(int nstr, char *strs[])
{
    int total_strlen = nstr; // start with #gaps for spaces
    for (int i = 0; i < nstr; i++) {
        total_strlen += strlen(strs[i]);
    }

    char *argbuf = malloc((total_strlen + 1) * sizeof(*argbuf));
    argbuf[0] = '\0';
    for (int i=0; i < nstr; i++) {
        argbuf = strcat(argbuf, " ");
        argbuf = strcat(argbuf, strs[i]);
    }

    return argbuf;
}


/*--------------------------------------------------------------------------------------
    File
*/

// file existence helper
bool file_exists(char *file_path)
{
    struct stat s;
    return (stat(file_path, &s) == 0);
}


// return the path of the .todo file in the home directory
char *todo_home_path()
{
    char *homedir = getenv("HOME");
    if (homedir == NULL) {
        // TODO error handling
        printf("HOME environment variable not set\n");
        return NULL;
    }
    char *pathbuf = malloc(strlen(homedir) + 7);
    sprintf(pathbuf, "%s/.todo", homedir);
    return pathbuf;
}


// dynamically source the right .todo file recursively in tree
char *todo_path()
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        // TODO error handling
        printf("Error: cannot open current working directory\n");
    }

    char *path = malloc(strlen(cwd) + 7);
    while (1) {
        sprintf(path, "%s/.todo", cwd);
        if (file_exists(path)) {
            return path;
        }

        char *parent_dir = strrchr(cwd, '/');
        if (parent_dir == NULL) { break; }
        *parent_dir = '\0';
        if (strlen(cwd) == 0) { break; }
    }

    return todo_home_path();
}


// dynamically open the right .todo file in the given mode
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


/*--------------------------------------------------------------------------------------
    Reading
*/


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


// list 1) todo items and 2) done items up to N items each
void print_all(int max_lines)
{
    print_todo(max_lines);
    print_done(max_lines);
}


/*--------------------------------------------------------------------------------------
    Writing
*/

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
// implementation of "sorting" algorithm:
//      newly done items are moved to the top
void mark_done(int item_num)
{
    FILE *fptr = todo_file("r");

    int line_num = 0;
    int count_todo = 0;
    int capacity = TODO_MAX_ITEMLINES;
    bool found = false;
    char line[LINE_MAX];
    char target[LINE_MAX];
    char **lines = malloc(capacity * sizeof(**lines));

    while (fgets(line, LINE_MAX, fptr)) {
        if (line_is_todo(line)) {
            count_todo++;
            if (count_todo == item_num) {
                strcpy(target, line);
                target[1] = 'X';
                found = true;
                continue;
            }
        }
        if (line_num > capacity) {
            capacity *= 2;
            lines = realloc(lines, capacity * sizeof(**lines));
        }
        lines[line_num] = malloc((strlen(line)+1) * sizeof(*line));
        lines[line_num] = strcpy(lines[line_num], line);
        line_num++;
    }

    fptr = freopen(NULL, "w", fptr);
    if (found) {
        fputs(target, fptr);
    }
    for (int i = 0; i < line_num; i++) {
        fputs(lines[i], fptr);
        free(lines[i]);
    }
    free(lines);
    fclose(fptr);
}


// set the Nth done item as todo
// implementation of "sorting" algorithm:
//      newest todo items are put/moved to the bottom
void mark_todo(int item_num)
{
    FILE *fptr = todo_file("r");

    int line_num = 0;
    int count_done = 0;
    int capacity = TODO_MAX_ITEMLINES;
    bool found = false;
    char line[LINE_MAX];
    char target[LINE_MAX];
    char **lines = malloc(capacity * sizeof(**lines));

    while (fgets(line, LINE_MAX, fptr)) {
        if (line_is_done(line)) {
            count_done++;
            if (count_done == item_num) {
                strcpy(target, line);
                target[1] = ' ';
                found = true;
                continue;
            }
        }
        if (line_num > capacity) {
            capacity *= 2;
            lines = realloc(lines, capacity * sizeof(**lines));
        }
        lines[line_num] = malloc((strlen(line)+1) * sizeof(*line));
        lines[line_num] = strcpy(lines[line_num], line);
        line_num++;
    }

    fptr = freopen(NULL, "w", fptr);
    for (int i = 0; i < line_num; i++) {
        fputs(lines[i], fptr);
        free(lines[i]);
    }
    if (found) {
        fputs(target, fptr);
    }
    free(lines);
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
        char *message = concat_args(argc-1, &argv[1]);
        add_item(message);
        free(message);
    }

    return EXIT_SUCCESS;
}

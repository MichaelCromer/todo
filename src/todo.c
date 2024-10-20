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
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define TODO_VERSION "1.0"
#define TODO_MAX_ITEMLINES 64
#define TODO_DEFAULT_PRINT_NUM 10

#define TODO_CHAR_MARK_DONE 'X'
#define TODO_CHAR_MARK_TODO ' '

#define FLAG_HELP_SHORT     "-h"
#define FLAG_HELP_LONG      "--help"
#define FLAG_EDIT_SHORT     "-e"
#define FLAG_EDIT_LONG      "--edit"
#define FLAG_VERSION_SHORT  "-v"
#define FLAG_VERSION_LONG   "--version"

#define FLAG_ALL_SHORT      "-a"
#define FLAG_ALL_LONG       "--print-all"
#define FLAG_TODO_SHORT     "-t"
#define FLAG_TODO_LONG      "--print-todo"
#define FLAG_DONE_SHORT     "-d"
#define FLAG_DONE_LONG      "--print-done"

#define FLAG_MARK_SHORT     "-x"
#define FLAG_MARK_LONG      "--done"
#define FLAG_UNMARK_SHORT   "-o"
#define FLAG_UNMARK_LONG    "--todo"

#define FLAG_CAPTURE        "--"

enum TODO_ACTION {
    ACTION_NONE,
    ACTION_HELP,
    ACTION_EDIT,
    ACTION_VERSION,
    ACTION_ALL,
    ACTION_TODO,
    ACTION_DONE,
    ACTION_MARK,
    ACTION_UNMARK,
    ACTION_CAPTURE
};


/*--------------------------------------------------------------------------------------
    -h etc
*/

// send help string to stdout
void print_help()
{
    printf("Usage: todo [OPTION|OPTION N| -- MESSAGE]\n");
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

    printf("Directories are searched upwards for a \'.todo\' file\n");
    printf("Input after -- is added as new lines, delimited by \\n\n");
    printf("If no arguments are supplied, default is todo -t 10\n");
}


// send version string to stdout
void print_version()
{
    printf("todo version %s by Michael Cromer\n", TODO_VERSION);
}


void print_error(const char *message, ...)
{
    va_list args;

    va_start(args, message);
    fprintf(stderr, "[TODO]  Error: ");
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    va_end(args);
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


// squash a list of strings (e.g., make all args into a new todo entry)
char *concat_args(int nstr, char *strs[])
{
    int total_strlen = nstr-1; // start with #gaps for spaces
    for (int i = 0; i < nstr; i++) {
        total_strlen += strlen(strs[i]);
    }

    char *argbuf = malloc((total_strlen + 1) * sizeof(*argbuf));
    argbuf[0] = '\0';
    for (int i=0; i < nstr; i++) {
        argbuf = strcat(argbuf, strs[i]);
        if (i == nstr-1) { break; }
        argbuf = strcat(argbuf, " ");
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
        print_error("HOME environment variable not set");
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
        print_error("cannot open working directory: %s", strerror(errno));
        return NULL;
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
    if (fpath == NULL) {
        print_error("cannot open todo file");
        return NULL;
    }
    FILE *fptr = fopen(fpath, mode);
    if (fptr == NULL) {
        print_error("cannot open %s: %s", fpath, strerror(errno));
        return NULL;
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


void print_with_filter(bool (*line_test)(char *line), int max_lines)
{
    FILE *fptr = todo_file("r");
    char line[LINE_MAX];
    int i = 1;

    while ((i <= max_lines) && (fgets(line, LINE_MAX, fptr))) {
        if (line_test(line)) {
            printf("\t%d\t%s", i, line);
            i++;
        }
    }
    fclose(fptr);
}


void print_todo(int max_lines)
{
    print_with_filter(line_is_todo, max_lines);
}


void print_done(int max_lines)
{
    print_with_filter(line_is_done, max_lines);
}


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
    if (fptr == NULL) {
        print_error("cannot open .todo for reading: %s", strerror(errno));
        return;
    }

    int line_num = 0;
    int count_todo = 0;
    int capacity = TODO_MAX_ITEMLINES;
    char line[LINE_MAX];
    char *target = NULL;
    char **lines = malloc(capacity * sizeof(char *));

    while (fgets(line, LINE_MAX, fptr)) {
        if (line_is_todo(line)) {
            count_todo++;
            if (count_todo == item_num) {
                target = malloc(strlen(line)+1);
                strcpy(target, line);
                target[1] = 'X';
                continue;
            }
        }
        if (line_num > capacity) {
            capacity *= 2;
            lines = realloc(lines, capacity * sizeof(char *));
        }
        lines[line_num] = malloc(strlen(line)+1);
        lines[line_num] = strcpy(lines[line_num], line);
        line_num++;
    }

    fclose(fptr);
    fptr = todo_file("w");
    if (fptr == NULL) {
        print_error("cannot open .todo for writing: %s", strerror(errno));
        return;
    }

    if (target) {
        fputs(target, fptr);
    }
    for (int i = 0; i < line_num; i++) {
        fputs(lines[i], fptr);
        free(lines[i]);
        lines[i] = NULL;
    }

    fclose(fptr);
    free(lines);
    fptr = NULL;
    lines = NULL;
}


// set the Nth done item as todo
// implementation of "sorting" algorithm:
//      newest todo items are put/moved to the bottom
void mark_todo(int item_num)
{
    FILE *fptr = todo_file("r");
    if (fptr == NULL) {
        print_error("cannot open .todo for reading: %s", strerror(errno));
        return;
    }

    int line_num = 0;
    int count_done = 0;
    int capacity = TODO_MAX_ITEMLINES;
    char line[LINE_MAX];
    char *target = NULL;
    char **lines = malloc(capacity * sizeof(char *));

    while (fgets(line, LINE_MAX, fptr)) {
        if (line_is_done(line)) {
            count_done++;
            if (count_done == item_num) {
                target = malloc(strlen(line)+1);
                strcpy(target, line);
                target[1] = ' ';
                continue;
            }
        }
        if (line_num > capacity) {
            capacity *= 2;
            lines = realloc(lines, capacity * sizeof(char *));
        }
        lines[line_num] = malloc(strlen(line)+1);
        lines[line_num] = strcpy(lines[line_num], line);
        line_num++;
    }

    fclose(fptr);
    fptr = todo_file("w");
    if (fptr == NULL) {
        print_error("Cannot open .todo for writing: %s", strerror(errno));
        return;
    }

    for (int i = 0; i < line_num; i++) {
        fputs(lines[i], fptr);
        free(lines[i]);
    }
    if (target) {
        fputs(target, fptr);
    }

    fclose(fptr);
    free(lines);
    fptr = NULL;
    lines = NULL;
}


// append an appropriately formatted todo item
void add_item(char *item)
{
    FILE *fptr = todo_file("a");

    fputs("[ ] ", fptr);
    fputs(item, fptr);
    fputs("\n", fptr);
    fclose(fptr);
}


/*
 *  Main and inputs
 */


int main(int argc, char *argv[])
{
    char *flag;
    int n;

    /* no args, default : display some todo items */
    if (argc < 2) {
        print_todo(TODO_DEFAULT_PRINT_NUM);
        return EXIT_SUCCESS;
    }

    flag = argv[1];

    /* exact match on particular flags */
    if ((strcmp(flag, "-h") == 0) || (strcmp(flag, "--help") == 0)) {
        print_help();
        return EXIT_SUCCESS;
    }

    if ((strcmp(flag, "-e") == 0) || (strcmp(flag, "--edit") == 0)) {
        edit_todo_file();
        return EXIT_SUCCESS; 
    }

    if ((strcmp(flag, "-v") == 0) || (strcmp(flag, "--version") == 0)) {
        print_version();
        return EXIT_SUCCESS;
    }

    /* require either '--option' (long), '-o' (short), or '--' (precedes todo item) */
    if ((flag[0] != '-') || (flag[1] == '\0') || (strchr("-atdox", flag[1]) == NULL)) {
        print_error("%s not recognised as a todo option.", flag);
        return EXIT_FAILURE;
    }

    /* can handle concatenated short/numeric options, e.g. -x3 == -x 3 */
    if ((argc == 2) && (flag[1] != '-')) {
        if (strlen(flag) < 3) {
            print_error("%s needs additional arguments.", flag);
            return EXIT_FAILURE;
        }
        n = atoi_pedantic(flag+2);
        if (n == 0) {
            print_error("%s not recognised as a todo option.", flag);
            return EXIT_FAILURE;
        }

        switch (flag[1]) {
            case 'a':
                print_all(n);
                break;
            case 't':
                print_todo(n);
                break;
            case 'd':
                print_done(n);
                break;
            case 'x':
                mark_done(n);
                break;
            case 'o':
                mark_todo(n);
                break;
            default:
                print_error("%s not recognised as a todo option.", flag);
                return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    if (strcmp(flag, "--") == 0) {
        /* now test to see what input source */
        if (argc > 2) {
            /* user has supplied msg, grab from args */
            char *message = concat_args(argc-2, &argv[2]);
            add_item(message);
            free(message);
        } else {
            /* user is piping, grab from stdin */
            if (isatty(fileno(stdin))) {
                print_error("no message supplied after '--'.");
                return EXIT_FAILURE;
            }
            char line[LINE_MAX];
            int len = 0;
            while (fgets(line, LINE_MAX, stdin)) {
                len = strlen(line);
                if (line[len-1] == '\n') {
                    line[len-1] = '\0';
                }
                add_item(line);
            }
        }
        return EXIT_SUCCESS;
    }

    n = atoi_pedantic(argv[2]);
    if (n == 0) {
        print_error("%s needs a numeric argument.", flag);
        return EXIT_FAILURE;
    }

    /* now check flags with parameters */
    if ((strcmp(flag, "-a") == 0) || (strcmp(flag, "--print-all") == 0)) {
        print_all(n);
    }
    else if ((strcmp(flag, "-d") == 0) || (strcmp(flag, "--print-done") == 0)) {
        print_done(n);
    }
    else if ((strcmp(flag, "-t") == 0) || (strcmp(flag, "--print-todo") == 0)) {
        print_todo(n);
    }
    else if ((strcmp(flag, "-x") == 0) || (strcmp(flag, "--done") == 0)) {
        mark_done(n);
    }
    else if ((strcmp(flag, "-o") == 0) || (strcmp(flag, "--todo") == 0)) {
        mark_todo(n);
    }
    else {

    }

    return EXIT_SUCCESS;
}

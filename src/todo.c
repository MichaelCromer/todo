/*--------------------------------------------------------------------------------------
    todo - a command line tool to manage todo items
----------------------------------------------------------------------------------------
    mcromer, Oct 2024
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

#define TODO_VERSION "1.5"
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
    printf("Input after -- is added as new items, delimited by \\n\n");
    printf("If no arguments are supplied, default is todo -t %d\n",
            TODO_DEFAULT_PRINT_NUM);
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
unsigned int atoi_pedantic(char *str)
{
    unsigned int result = 0;
    for (int i=0; str[i] != '\0'; i++) {
        if (!isdigit((unsigned char) str[i])) {
            return 0;
        }
        result = 10*result + (unsigned int)(str[i] - '0');
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


void print_lines(enum TODO_ACTION action, int max_lines)
{
    if (action == ACTION_TODO || action == ACTION_ALL) {
        print_with_filter(line_is_todo, max_lines);
    }
    if (action == ACTION_DONE || action == ACTION_ALL) {
        print_with_filter(line_is_done, max_lines);
    }

    return;
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


void mark_line(enum TODO_ACTION action, int target_line_num)
{
    FILE *fptr = todo_file("r");
    bool (*line_test)(char *line) = NULL;
    int curr_line_num = 0,
        target_count = 0,
        capacity = TODO_MAX_ITEMLINES;
    char mark = '\0',
         line[LINE_MAX],
         *target_line = NULL,
         **lines = malloc(capacity * sizeof(char *));

    switch (action) {
        case ACTION_MARK:
            line_test = line_is_todo;
            mark = TODO_CHAR_MARK_DONE;
            break;
        case ACTION_UNMARK:
            line_test = line_is_done;
            mark = TODO_CHAR_MARK_TODO;
            break;
        default:
            return;
    }

    while (fgets(line, LINE_MAX, fptr)) {
        if (line_test(line)) {
            target_count++;
            if (target_count == target_line_num) {
                target_line = malloc(strlen(line)+1);
                strcpy(target_line, line);
                target_line[1] = mark;
                continue;
            }
        }
        if (curr_line_num > capacity) {
            capacity *= 2;
            lines = realloc(lines, capacity * sizeof(char *));
        }
        lines[curr_line_num] = malloc(strlen(line)+1);
        strcpy(lines[curr_line_num], line);
        curr_line_num++;
    }

    fclose(fptr);
    fptr = todo_file("w");

    if (target_line && action == ACTION_MARK) {
        fputs(target_line, fptr);
    }
    for (int i = 0; i < curr_line_num; i++) {
        fputs(lines[i], fptr);
        free(lines[i]);
        lines[i] = NULL;
    }
    if (target_line && action == ACTION_UNMARK) {
        fputs(target_line, fptr);
    }

    fclose(fptr);
    free(lines);
    fptr = NULL;
    lines = NULL;

}


// append an appropriately formatted todo item
void add_line(char *line)
{
    FILE *fptr = todo_file("a");

    fputs("[ ] ", fptr);
    fputs(line, fptr);
    fputs("\n", fptr);
    fclose(fptr);
}


/*
 *  Main and inputs
 */


enum TODO_ACTION input_option_parse(char *option)
{
    if        ( (strncmp(option, FLAG_HELP_SHORT, 2) == 0) ||
                (strcmp( option, FLAG_HELP_LONG    ) == 0)) {
        return ACTION_HELP;
    } else if ( (strncmp(option, FLAG_EDIT_SHORT, 2) == 0) ||
                (strcmp( option, FLAG_EDIT_LONG    ) == 0)) {
        return ACTION_EDIT;
    } else if ( (strncmp(option, FLAG_VERSION_SHORT, 2) == 0) ||
                (strcmp( option, FLAG_VERSION_LONG    ) == 0)) {
        return ACTION_VERSION;
    } else if ( (strncmp(option, FLAG_ALL_SHORT, 2) == 0) ||
                (strcmp( option, FLAG_ALL_LONG    ) == 0)) {
        return ACTION_ALL;
    } else if ( (strncmp(option, FLAG_TODO_SHORT, 2) == 0) ||
                (strcmp( option, FLAG_TODO_LONG    ) == 0)) {
        return ACTION_TODO;
    } else if ( (strncmp(option, FLAG_DONE_SHORT, 2) == 0) ||
                (strcmp( option, FLAG_DONE_LONG    ) == 0)) {
        return ACTION_DONE;
    } else if ( (strncmp(option, FLAG_MARK_SHORT, 2) == 0) ||
                (strcmp( option, FLAG_MARK_LONG    ) == 0)) {
        return ACTION_MARK;
    } else if ( (strncmp(option, FLAG_UNMARK_SHORT, 2) == 0) ||
                (strcmp( option, FLAG_UNMARK_LONG    ) == 0)) {
        return ACTION_UNMARK;
    }

    return ACTION_NONE;
}


int input_numeric_parse(char *primary, char *fallback, int *fallback_ticker)
{
    int n = 0;

    if (primary) {
        n = atoi_pedantic(primary);
    }

    if (n == 0 && fallback) {
        n = atoi_pedantic(fallback);
        if (fallback_ticker) {
            (*fallback_ticker)++;
        }
    }

    return n;
}


void input_stdin()
{
    char line[LINE_MAX];
    int len = 0;
    while (fgets(line, LINE_MAX, stdin)) {
        len = strlen(line);
        if (line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        add_line(line);
    }
    return;
}

int main(int argc, char *argv[])
{
    /* no args, default : display some todo items */
    if (argc < 2) {
        print_lines(ACTION_TODO, TODO_DEFAULT_PRINT_NUM);
        return EXIT_SUCCESS;
    }

    for (int i=1; i < argc; i++) {
        char *curr_option = argv[i];
        enum TODO_ACTION curr_action = input_option_parse(curr_option);

        /* prepare to grab potential numeric input.
         * either from -oN or -o N or --option N */
        unsigned int curr_num = input_numeric_parse(
            ( strlen(curr_option) > 1 )    ? curr_option + 2      : NULL,
            ( (i+1)<argc )          ? argv[i+1]     : NULL,
            &i /* will do i++ if we needed to grab the 2nd argument */
        );

        switch (curr_action) {
            /* -h etc flags */
            case ACTION_HELP:
                print_help();
                return EXIT_SUCCESS;
            case ACTION_EDIT:
                edit_todo_file();
                return EXIT_SUCCESS;
            case ACTION_VERSION:
                print_version();
                return EXIT_SUCCESS;

            /* printing items from the file */
            case ACTION_ALL:
            case ACTION_TODO:
            case ACTION_DONE:
                if (!curr_num) {
                    print_error("%s needs a numeric argument", curr_option);
                    return EXIT_FAILURE;
                }
                print_lines(curr_action, curr_num);
                break;

            /* marking a line item todo/done */
            case ACTION_MARK:
            case ACTION_UNMARK:
                if (!curr_num) {
                    print_error("%s needs a numeric argument", curr_option);
                    return EXIT_FAILURE;
                }
                mark_line(curr_action, curr_num);
                break;

            /* capturing the remaining input */
            case ACTION_CAPTURE:
                if (!isatty(fileno(stdin))) {
                    input_stdin();
                    return EXIT_SUCCESS;
                }
                char *message = concat_args(argc-i, &argv[i+1]);
                add_line(message);
                free(message);
                return EXIT_SUCCESS;

            /* fail state */
            default:
                print_error("%s is not a recognised todo option", curr_option);
                return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

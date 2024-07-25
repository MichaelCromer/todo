# todo

A command line tool for managing todo lists.

## Usage

`todo` will accept the following patterns:

    todo [OPTION|OPTION N|MESSAGE]

Without any arguments, `todo` will default to displaying the top 10 todo items.

Arguments are parsed to match `OPTION`, then `OPTION N`, then `MESSAGE`, in that order.
In essence, anything that doesn't match an option flag known to `todo` will be recorded
as a new todo item. In the case of options requiring numeric arguments, an incomplete
match will result in an error message, and not fall through to the `MESSAGE` case.

### Options

    Option:
      -h --help       Display this message and exit
      -e --edit       Open the todo file in $EDITOR
      -v --version    Display the current version #

    Option N:
      -x --done       Mark the Nth todo item as done
      -o --todo       Mark the Nth done item as todo
      -t --print-todo Display the first N todo items
      -d --print-done Display the first N done items
      -a --print-all  Display -t N, -d N in sequence


## The .todo File

Currently, `todo` is hard-coded to look for `$HOME/.todo`, a plain text file, as the
source of all todo items. *It is up to you to ensure this file exists*, as `todo` will
bail out if it can't find it.

### Syntax

All that `todo` cares about is
    - lines starting with "[ ]" to represent todo items
    - lines starting with "[X]" to represent done items

All other lines in `.todo` are ignored.

### Maintenance

Updating of line items is done in-place. New todo items are appended to the end of the
file.  Therefore, the oldest todo items appear at the top of the todo list, and then
move to the top of the done list when marked off. This may not be desirable but it's how
it works (for now).

## Future Goals

- `todo` instead recursively climbs the dirtree from where it is invoked, looking for a
  `.todo` file, falling back to `$HOME/.todo` if nothing is found. This would allow for
  e.g., todo item management in a git repo or other 'project' directory.
- vaguely intelligent todo/done item sorting and updating to match the most sensible
  expected use case: todo items listed oldest-youngest (assumption is that older todo
  items are more urgent), done items are listed youngest-oldest, age of an item being
  determined from when it was marked completed (assumption is that older done items are
  more 'stale'/less interesting/less likely to be opened back up again with an -o call).

# todo

A command line tool for managing todo lists.

## Usage

`todo` will accept the following patterns:

    todo [OPTION|OPTION N|MESSAGE]

Without any arguments, `todo` will default to displaying the top 10 todo items.

Arguments are parsed to match the `OPTION`, the `OPTION N`, then the `MESSAGE` case, in
that order. If a match is found, trailing arguments are ignored. In the `OPTION N` case,
an incomplete match will result in an error, and will not fall through to the `MESSAGE`
case. 

In essence, any input that doesn't look like it begins with an option flag known to
`todo` will be concatenated and recorded as a new todo item. 

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
source of all todo items. **It is up to you to ensure this file exists**, and `todo`
will generally bail out if it can't find it.

### Syntax

All that `todo` cares about is

- lines starting with `[ ]` to represent todo items
- lines starting with `[X]` to represent done items

All other lines in `.todo` are ignored.

### Maintenance

Updating of line items occurs in the following way: 

- items marked 'done' are moved to the top of the file; and,
- items marked 'todo' are moved to the bottom of the file (as if added as a new item).

This behaviour provides a kind of passive "sorting" algorithm, which ensures that newly
comleted items appear at the top of the done list, and newly reopened items are treated
with the same priority as newly added ones.

## Future Goals

- `todo` instead recursively climbs the dirtree from where it is invoked, looking for a
  `.todo` file, falling back to `$HOME/.todo` if nothing is found. This would allow for
  e.g., todo item management in a git repo or other 'project' directory.

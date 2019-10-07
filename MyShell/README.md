# MyShell

This is a simple UNIX Shell made from scratch as part of a Systems programming exercise, implemented in C. This supports basic UNIX shell features and a few more custom commands.

## Screenshot

![](https://i.imgur.com/NFA9z5f.png)


## Compile and Run (For POSIX based Systems)
* Compile using :
`make`

* Run the program with :
`./shell`

## Features
* Basic Unix Shell Operations like Job Control, Process Groups, execution of `UNIX` commands.

* Can change the current working directory of the Shell using `cd` command. Home directory can also be interchanged with `~` when changing directories.

* Allows the user to add any directory to the `.myshellrc` configuration file to add to the local `PATH` variable for the Shell. The Shell looks into all these directories one by one, and tries executing each line until there is a success. Otherwise, a suitable error message is printed.

* Can execute `filename` having suitable execute permissions in the current working directory using `./filename`. (Dot Slash commands)

* Foreground and Background Job Control using `fg` and `bg` commands.

Usage (Both fg and bg):

```
$ fg 1000

$ fg %python

```

* Pipe `|` Operator for redirecting commands to another command.

* Redirection `>`, `>>` and `<` Operators for input / output redirection from / to a file respectively.

Usage :

```
ls | grep out > output.txt

```

* Support for System V Message Queues using `##` operator. This passes the output of an input command via Message Queues, which then pipes it to other commands.

* Support for Shared Memory Redirection using `SS` operator. This is similar to the `##` operator, but uses the system's Shared Memory for faster execution and does not have the limitation of a fixed buffer size like Message Queues.

Usage for `##` and `SS` :

```
ls ## wc , sort ## Uses System V Message Queues

ls SS wc , sort ## Uses Shared Memory

```

* Can start daemon processes with the `daemonize` command.

Usage (Executes `bash script.sh` as a daemon process) :

```
daemonize bash script.sh

```

## TODO
* Add some more builtin commands.
* Have implemented a basic AutoComplete Engine using Tries. Incorporate it into the Shell, possibly with the help of the `ncurses` library.

## Bugs
* Suspend signal does not work as intended, if the background job waits for `tty` as input.
* `&` does not allow support with pipes and redirection at present.
* `##` Operation for piping via System V Message Queues does not presently support chaining with pipes and redirection operators.
* `SS` Operation for piping via Shared Memory does not presently support chaining with pipes and redirection operators.
* As of now, any command executes as expected only if there is atleast one space between any two keywords. Even if you want to get the output to multiple commands using `,`, you still need to insert a space before and after the comma.
* There is a lot of memory requirement for building the Trie for the Autocomplete Engine incase of a larger autocomplete list. A better solution could be to sort the autocomplete words and load the indices into a Tree like data structure for fast range querying. Optimize the memory limits and improve it's performance.

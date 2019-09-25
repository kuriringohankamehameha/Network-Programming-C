# Features
* Basic Unix Shell Operations like Job Control, Process Groups, execution of `UNIX` commands.
* Foreground and Background Job COntrol using `fg` and `bg` commands.
* Pipe `|` Operator for redirecting commands to another command.
* Redirection `>` and `<` Operators for input / output redirection from / to a file respectively.
* Support for System V Message Queues using `##` operator. This passes the output of an input command via Message Queues, which then pipes it to other commands.
* Support for Shared Memory Redirection using `SS` operator. This is similar to the `##` operator, but uses the system's Shared Memory for faster execution and does not have the limitation of a fixed buffer size like Message Queues.

# TODO
* Support `SS` Operation using Shared Memory.
* Add some more builtin commands.
* Refactor Code.

# Bugs
* Running `fg` on a backgrounded process sometimes does not resolve by name.
* Suspend signal does not work as intended.
* `##` Operation does not presently support chaining with pipes and redirection operators.

# Repository of my Network Programming Assignments

## Sequence of the Assignments in order

### Load Balancer using Signals

* This Load Balancer was implemented via a process switcher using the `signal` API for corresponding system calls to send to `N` different Child Processes. 
* All child processses read from `Filename` and outputs the result from the buffer. 
* The load balancer keeps sending signals to all the Children, and any child processes the Signal from the load balancer only if it is free. 
* This program performs it's signal handling and batch process management via Round Robin Scheduling, using a Queue. 

### Shell Clone

* This is a basic clone of a UNIX Shell, made using processes and the Job Control API. 
* This shell facilitates basic Job Control via `fg` and `bg` commands. 
* It can also execute commands in a pipeline using `|`, or perform both input (`<`) and output (`>`) redirection on any command, which can be searched via a local `PATH` variable for it's validity before execution. This local `PATH` can be set or updated accordingly via the `.myshellrc` config file for the shell.
* The config file also supports basic colored output, that can also be configured accordingly.

### Cluster Shell

### Tee Command

### Voting System

### Large DataWrite Comparison

### Client Server Requests

### Fork Per Client

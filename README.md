# Repository of my Network Programming Assignments

**Disclaimer**
* Some of these programs may not actually run for you, since I may have missed ceratain files. If there are any problems, you could file an issue, indicating that a program wasn't working. I'll try to patch them whenever possible.

## Sequence of the Assignments in order

### 1. Load Balancer using Signals

* This Load Balancer was implemented via a process switcher using the `signal` API for corresponding system calls to send to `N` different Child Processes. 
* All child processses read from `Filename` and outputs the result from the buffer. 
* The load balancer keeps sending signals to all the Children, and any child processes the Signal from the load balancer only if it is free. 
* This program performs it's signal handling and batch process management via Round Robin Scheduling, using a Queue. 

### 2. Shell Clone

* This is a basic clone of a UNIX Shell, made using processes and the Job Control API. 
* This shell facilitates basic Job Control via `fg` and `bg` commands. 
* It can also execute commands in a pipeline using `|`, or perform both input (`<`) and output (`>`) redirection on any command, which can be searched via a local `PATH` variable for it's validity before execution. This local `PATH` can be set or updated accordingly via the `.myshellrc` config file for the shell.
* The config file also supports basic colored output, that can also be configured accordingly.

### 3. Cluster Shell

* This is an implementation of a Cluster Shell, which is a shell operated by multiple clients and can share output commands between each other.
* Each client can get the output from the shell of another connected client through suitable commands bridged through a concurrent multithreaded TCP Server.

### 4. Tee Command

* This is an implementation of the linux `tee` command, which pipes outputs to both the write end of the pipe as well as to `STDOUT`.

### 5. Voting System

* This program simulates a voting system using System V Message Queues as an IPC.

### 6. Large DataWrite Comparison

* This program compares the IO read/write performance between the system calls `mmap()` vs `write()` when writing to a large `1GB` file.

### 7. HTTP Requests

* This program implements a low level HTTP Requests using the Sockets API.

### 8. Fork Per Client Model

* This program implements a TCP Echo Server having a maximum limit of a certain number of concurrent clients, and makes further clients wait until any one of them disconnects. The Server calls `fork()` per client and a System V Semaphore is used for keep track of the number of clients for IPC.

### 9. N-Channel Stop and Wait Protocol

* This program simulates an N-channel stop and wait protocol as a File Transfer Mechanism between a client and a server. This utilizes concurrent TCP connections and packet retransmission based on sequence numbers.

### 10. UDPChatter

* This program implements UDP Multicasting. There is no requirement of an explicit server. This is used in local network application like **DC++**. This is a Chat application which has a single room, where participants can enter and exit. Common messages are displayed in the main box, while explicit messages can be sent using channels.

### 11. BigFileSystem

* This is an attempt to implement a basic distributed filesystem hierarchy. There are multiple servers which share the filesystem, and the client and read, write, or modify the files in the filsystem as if it were his own computer.

### 12. Packet Sniffer

* This is a very simple packet sniffer written using UNIX raw sockets. Run this program using `sudo`, as you cannot create raw sockets as a non-admin user.

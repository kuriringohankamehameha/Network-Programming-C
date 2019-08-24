# Repository of my Network Programming Assignments

## Sequence of the Assignments in order

### Load Balancer using Signals (Lab Exercise 1) :

* I had to implement a Load Balancer via a process switcher using the `signal` API for corresponding system calls to send to `N` different Child Processes. All child processses read from `Filename` and outputs the result from the buffer. The load balancer keeps sending signals to all the Children, and any child processes the Signal from the load balancer only if it is free. This program is a good example of signal handling and batch process management via Round Robin Scheduling, using a Queue. 

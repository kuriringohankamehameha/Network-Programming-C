# ClusterShell

## Design

* The Cluster Shell was implemented using a multithreaded TCP Client-Server model.

* The Server creates a thread everytime it's listening file descriptor is accepted by an incoming Client.

* Both the threads in the client and the server listen to incoming messages and act accordingly. Any assumption is made that the number of clients are restricted to `9` or less, but the logic can be extended towards more.

* Due to a limited buffer size, there may be cases when the Output limit may exceed the buffer size.

* Pipelining has not been implemented in this version of Clustershell.

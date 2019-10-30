# Fork per Client TCP Server
This program implements a TCP Echo Server in `tcp_server_N_clients.c`, which takes a maximum of upto `N` concurrent clients (`N` is passed as a Command Line Argument).
Any further clients are made to wait until one or more of the `N` clients exit.

The server operates in a fork per client basis, creating a child process per child. A System V Semaphore is used for maintaining the count of the number of clients and Interprocess Communication.

Start the server using : `./tcp_server_N_clients MAX_CLIENTS`

Client implementation is in `client.c`. Use with : `./client localhost 8080`

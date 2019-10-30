# HTTP Requests using TCP Sockets

* This program demonstrates a TCP Client sending a `HTTP` request to a server at a given URL, using PORT 80.

* The request length is obtained from the request header format, which is when the Client requests a connection to the Server.

* After successful connection, the Server sends the appropriate response, which the Client retrieves in byte streams from a fixed size buffer.

* The process is complete when all bytes have been read by the client, therby closing the connection.

Necessary code is at `client.c`.

## Compilation

Compile using : `make`

## Running the program

Run the program using :
```bash
./client example.com
```

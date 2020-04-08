# BigFileSystem
* This is a distributed filesystem which is spread across multiple servers.
* Any client can treat this as if this were their own computer, by opening, moving, and deleting files. The servers are implemented using TCP, with IO multiplexing and also multiple worker threads for concurrency and parallelism.

# Usage
* `client.c`
```
(By default, PORT_0 = 8080)
./client 1 IP_0 PORT_1 IP_1 IP_2 ... (as many as the number of servers)
```

* `filenameserver.c`
```
./filenameserver PORT_NO
```

* `dataserver.c`
```
./dataserver PORT_NO SERVER_NO
```

The TCP Servers are implemented using I/O Multiplexing with `select()` and are multithreaded. Everytime a file is to be read or written, multiple worker threads are spawned which copy/move the blocks with the help of the TCP Sockets. The client must connect to all servers since there is parallel access to/from the file descriptors.

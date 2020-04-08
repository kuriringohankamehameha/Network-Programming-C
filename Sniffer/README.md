# Packet Sniffer

* This is a simple packet sniffer using UNIX raw sockets. A raw socket is created which analyses packets going through the network and logs statistics about the packet information in a log file `log.txt`.

## Compilation
* Compile with : `make`

## Running the binary
* Since creation of raw sockets requires elevated priviliges, use `sudo` and run with :
```
sudo ./read_messages
```

## About
* This is a program which creates raw sockets and reads incoming packets to the system. It processes the header of every packet and logs the source and destination IP, and some TCP specific protocol information into `log.txt`.
* Another thread is created which simultaneously listens for any Ethernet packets for the source and destination Mac addresses.

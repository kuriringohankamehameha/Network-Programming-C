# N-Channel Stop and Wait Protocol Implementation
This is an implementation of a N-Channel Stop and Wait Protocol, using the TCP Sockets API on POSIX Systems.

The client makes `N` concurrent TCP Connections to a Server, and uploads an input file `input.txt` to it, using all `N` channels, in a stop-and-wait protocol.

The server has a tendency to drop packets randomly with a probability `PDR` (Packet Drop Rate), which is set to be `0.1`. This means that the client must retransmit unackowledged packets, using a timeout mechanism via `poll`.

To proceed further, every packet that the client sent must be `ACK`ed by the server. The packets may come out of order, due to packet retransmission. The server makes sure that all the packets are written to an output file `output.txt` in the proper ordering, using the Packet Sequence Numbers.

## Usage
Compile with `make`:

```
make
```

Run the server using : `./server.out <MAX_CLIENTS> <PORT_NO>`

Run the client using : `./client.out <IP_ADDRESS> <PORT_NO>`

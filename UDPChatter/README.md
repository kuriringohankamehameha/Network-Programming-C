# MultiCast Chat Application

* Whenever there is a test on any Course (there are `N` courses), one of the clients sends a message and broadcasts it.
* Any other connected client in the same LAN Network receives the message.
* There is no central server used in the implementation.

## Assumptions

* A UDP Multicast Chat is implemented, with the number of courses being predetermined.
* Every client has a list of enrolled groups which they want to get notifications from, and a global chat group, where everyone sends multicast messages
* '!subscribe "Chemistry"' -> Enrolls the client to the Chemistry group for receiving notifications
* '!sendto "Chemistry" Hello everyone. '-> Sends messages to all clients on the Chemistry group
* A thread is created for every new subscription, and it listens to all messages being forwarded to it's own group.

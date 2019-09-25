# Voting System using System V Message Queues

 A voting system is implemented using Message Queues. Here, a parent process creates `N` children. Whenever the parent sends a message to it's children, they reply with either `1` or `0`. Criterion for accepting is majority of `1`s, wheras a majority of `0`s imply rejection. The parent conveys the decision, as well as the individual messages of all the children. The child also prints it's response. The response is measured via an unbiased coin toss.

 ## Details

Type `vote` to perform the voting
Type `exit` to quit the program

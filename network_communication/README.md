# Network Communication
Description
- A client sends bash commands to a server. The server forks a shell process and executes the commands received from the client. The output from the shell process is forwarded back to client from the server.

Demsontrates the use of various system calls to allow interprocess communcation in the following ways
- a client communicating with a server via socket(7)
- the server forking a separate shell process and communicating with it via pipe(2)

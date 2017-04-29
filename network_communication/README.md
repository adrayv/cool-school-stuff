# Network Communication
Description
- A client sends bash commands to a server. The server forks a shell process and executes the commands received from the client. The output from the shell process is forwarded back to client from the server.
- Client and server use the mcrypt(3) API to encrypt and decrypt data coming to and from the socket.

Demsontrates the use of various system calls to allow interprocess communcation in the following ways
- a client communicating with a server via socket(7)
- the server forking a separate shell process and communicating with it via pipe(2)

Usage
- The Makefile that builds the necessary executables have been fully tested on my school's Red Hat Linux Servers
- Unfortunately, they do not build correectly on my local environment.
- I have provided a directory called "basic" that contains the files necessary to build the executables without the encryption/decryption functionality

# Try it out!
1) git clone https://github.com/adrayv/cool-school-stuff.git
2) cd basic
3) make
4) ./server --port=portno
5) ./client --port=portno

Enter some bash commands in your client window and check out the results!

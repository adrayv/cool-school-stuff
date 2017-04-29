$(CC) = gcc

all: client server

client: client_basic.c
	$(CC) client_basic.c -o client_basic

server: server_basic.c
	$(CC) server_basic.c -o server_basic

clean: all
	rm *_basic

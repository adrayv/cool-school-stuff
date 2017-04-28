CC = gcc
DEBUG = -g

all: client server

client: client.c
	$(CC) $(DEBUG) client.c -o client -lmcrypt

server: lab1b-server.c
	$(CC) $(DEBUG) server.c -o server -lmcrypt

dist:
	tar -czf networking.tar.gz server.c client.c Makefile \
	my.key README

clean: all
	touch temp.tar.gz; rm client server *.tar.gz

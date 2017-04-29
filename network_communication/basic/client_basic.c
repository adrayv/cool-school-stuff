#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <errno.h>

struct option args[] = {
	{"port", 1, NULL, 'p'},
	{"log", 1, NULL, 'l'},
	{"encrypt", 1, NULL, 'e'},
	{0,0,0,0}
};

void echo(char);
void print_usage_err();
void create_terminal_settings();
void restore_terminal_settings();
struct termios settings;
int old_iflag;
int old_oflag;
int old_lflag;

int main(int argc, char** argv) {
	int arg_item;
	int log_fd = -1; // assumes the file descriptor for log is not made
	char* port_arg = NULL; // defaults to no argument
	char* log_arg = NULL; // defaults to no argument
	char* encrypt_arg = NULL; // defaults to no argument

	/* this while loop is responsible for parsing through arguments */
	while((arg_item = getopt_long(argc, argv, "ple", args, NULL)) != -1) {
		switch(arg_item) {
			case 'p':
				port_arg = optarg;
				break;
			case 'l':
				log_arg = optarg;
				break;
			case 'e':
				encrypt_arg = optarg;
				break;
			default:
				print_usage_err();
				exit(1);
				break;
		}
	}
    int portno;
    struct sockaddr_in server_addr;

	/* establish portno */
	if(port_arg == NULL || strlen(port_arg) == 0) { // if user did not spcify an arg
		fprintf(stderr, "--port= is a necessarry argument\n");
		exit(1);
	}
	else {
		portno = atoi(port_arg); // assign the portno
	}

    /* initializes server */
    struct hostent *server = gethostbyname("localhost");
    if(server == NULL) { // error checking for establishing the host
    	fprintf(stderr, "Cannot establish host: %s\n", strerror(errno));
    	exit(1);
    }

	/* sets up sockfd */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) { // error checking for socket
    	fprintf(stderr, "Cannot create socket: %s\n" , strerror(errno));
    	exit(1);
    }

    /* sets the fields of server_addr */
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(portno);

    /* establish a connection to server */
    if(connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
    	fprintf(stderr, "Cannot connect to server: %s\n", strerror(errno));
    	exit(1);
    }
    /* AT THIS POINT WE HAVE CONNECTED TO THE SERVER */

	/* create non-canoical behavior */
	create_terminal_settings();

	/* Set up polling to listen to keyboard and socket */
	struct pollfd fds[2]; // create two things to listen to
	int poll_ret;
	char* lf = "\n";
	char* cr_lf = "\r\n";
	int *status;
	char curr_char[1]; // buffer to read characters
	int n; // number of characters written or read

	fds[0].fd = STDIN_FILENO;
	fds[1].fd = sockfd;
	fds[0].events = POLLIN;
	fds[1].events = POLLIN | POLLHUP | POLLERR; // FIX: Does socket have to poll for errors?

	/* poll to listen from keyboard and socket */
	while(1) { 
		poll_ret = poll(fds, 2, 0);

		/* data to be read from stdin */
		if(fds[0].revents & POLLIN) { 
			n = read(0, curr_char, 1);
			if(n < 0) { // error check reading from stdin
				fprintf(stderr, "error reading from stdin\n");
				restore_terminal_settings();
				exit(1);
			}
			echo(curr_char[0]);
			if(write(sockfd, curr_char, 1) < 0) { // write to socket
				fprintf(stderr, "Error writing to socket\n");
				restore_terminal_settings();
				exit(1);
			}
		}

		/* data to be read from socket */
		if(fds[1].revents & POLLIN) { 
			n = read(sockfd, curr_char, 1);
			if(n < 0) { // error check reading from socket
				fprintf(stderr, "error reading from socket\n");
				restore_terminal_settings();
				exit(1);
			}
			if(n == 0) break; // socket closed and we are done reading from server
			echo(curr_char[0]);
		}

		/* Some error with the socket */
		if(fds[1].revents & (POLLHUP+POLLERR)) {
			fprintf(stderr, "\r\nSocket Error\n");
			restore_terminal_settings();
			exit(1);
		}
	} // end of while loop

	restore_terminal_settings();
	exit(0);
}

void echo(char c) {
	int n;
	if(c == '\r' || c == '\n') { // map the proper values
		n = write(1, "\r\n", 2);
	}
	else if(c == 0x04 || c == 0x03) {} // do nothing with ^D ^C
	else { // echo to stdout
		n = write(1, &c, 1);
	}
	if(n < 0) { // error check writing to stdout
		fprintf(stderr, "error writing to stdout\n");
		restore_terminal_settings();
		exit(1);
	}
}

void create_terminal_settings() {
	int return_val;
	return_val = tcgetattr(0, &settings);
	if(return_val < 0) {
		fprintf(stderr, "CREATION: error in tcgetattr()\n");
		exit(1);
	}
	// saves the old settings to restore before exit
	old_iflag = settings.c_iflag;
	old_oflag = settings.c_oflag;
	old_lflag = settings.c_lflag;
	
	// sets new settings
	settings.c_iflag = ISTRIP;
	settings.c_oflag = 0;
	settings.c_lflag = 0;
	tcsetattr(0, TCSANOW, &settings);
	return_val = tcgetattr(0, &settings);
	if(return_val < 0) {
		fprintf(stderr, "CREATION: error in tcsetattr()\n");
		exit(1);
	}
}

void restore_terminal_settings() { // restore old settings
	int return_val;
	settings.c_iflag = old_iflag;
	settings.c_oflag = old_oflag;
	settings.c_lflag = old_lflag;
	tcsetattr(0, TCSANOW, &settings);
	return_val = tcgetattr(0, &settings);
	if(return_val < 0) {
		fprintf(stderr, "RESTORATION: error in tcsetattr()\n");
		exit(1);
	}
}

void print_usage_err() {
	printf("usage: ./lab1b-client [options]\n\t");
	printf("options:\n");
	printf("\t--port=\t\t\tspecify the port\n");
	printf("\t--log=filename\t\tspecify file which records data sent over socket\n");
	printf("\t--encrypt=filename\tflag to specify encryption\n");
	return;
}

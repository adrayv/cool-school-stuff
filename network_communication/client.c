#include <mcrypt.h>
#include <sys/stat.h>
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

void write_log(int, char*, int);
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

	MCRYPT td, td2; // one for encrypt and one for decrypt
	char* IV; // initialization vector
	char* IV2;
	char *key; // the key obtained from my.key FIX LATER
	int keysize = 16; // size (in bytes) of key
	char block_buffer;
	int key_fd = -1;// file descriptor of my.key
	int i;
	struct stat key_stat; // to obtain info about my.key file (like file size)

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

	/* set log file descriptor */
	if(log_arg != NULL && strlen(log_arg) != 0) {
		log_fd = open(log_arg, O_CREAT|O_RDWR|O_TRUNC, 0644);
		if(log_fd == -1) {
			fprintf(stderr, "Cannot open log file: %s\n", strerror(errno));
			exit(1);
		}
	}

	/* set up ENCRYPTION and DECRYPTION */
	if(encrypt_arg != NULL && strlen(encrypt_arg) != 0) {
		key_fd = open(encrypt_arg, O_RDONLY);
		if(key_fd < 0) {
			fprintf(stderr, "Error opening key file specified by --encrypt\n");
			exit(1);
		}
		if(fstat(key_fd, &key_stat) == -1) { // call fstat on key_fd
			fprintf(stderr, "Error calling fstat on key_fd\n");
			exit(1);
		}
		key = calloc(1, key_stat.st_size);
		if(read(key_fd, key, key_stat.st_size) < 0) { // read from my.key into key
			fprintf(stderr, "Error reading from my.key into key\n");
			exit(1);
		}

		td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
		if(td == MCRYPT_FAILED) {
			fprintf(stderr, "MCRYPT FAILED");
			exit(1);
		}
		IV = malloc(mcrypt_enc_get_iv_size(td));
		for(i = 0; i < mcrypt_enc_get_iv_size(td); i++) {
			IV[i] = 'A';
		}
		i = mcrypt_generic_init(td, key, keysize, IV);
		if(i < 0) {
			mcrypt_perror(i);
			exit(1);
		}
		
		td2 = mcrypt_module_open("twofish", NULL, "cfb", NULL);
		if(td2 == MCRYPT_FAILED) {
			exit(1);
		}
		IV2 = malloc(mcrypt_enc_get_iv_size(td2));
		for(i = 0; i < mcrypt_enc_get_iv_size(td2); i++) {
			IV2[i] = 'A';
		}
		i = mcrypt_generic_init(td2, key, keysize, IV2);
		if(i < 0) {
			mcrypt_perror(i);
			exit(1);
		}
	}
	/* td: encryption, td2: decryption */
	
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
			if(key_fd != -1) { // valid encrytion is set
				/* encrypt buffer before sending it over socket */
				block_buffer = curr_char[0];
				mcrypt_generic(td, &block_buffer, 1);
				curr_char[0] = block_buffer;
			}
			if(write(sockfd, curr_char, 1) < 0) { // write to socket
				fprintf(stderr, "Error writing to socket\n");
				restore_terminal_settings();
				exit;
			}
			if(log_fd != -1) { // write to log if specified
				write_log(log_fd, curr_char, 1);
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
			if(n == 0) { // Socket closed
				//fprintf(stderr, "\r\nSOCKET CLOSED\r\n");
				break;
			}
			if(log_fd != -1) { // write to log if specified
				write_log(log_fd, curr_char, 0);
			}
			if(key_fd != -1) {
				/* decrypt the character received from server */
				block_buffer = curr_char[0];
				mdecrypt_generic(td2, &block_buffer, 1);
				curr_char[0] = block_buffer;
			}
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


void write_log(int log_fd, char *buffer, int mode) {
	char *message = malloc(strlen(buffer));
	char* lf = "\n";
	int n, m;

	if(mode == 1) n = sprintf(message, "SENT %d byte: ", strlen(buffer));
	else n = sprintf(message, "RECEIVED %d byte: ", strlen(buffer));
	if(n < 0) {
		fprintf(stderr, "Error calling sprintf\n");
		restore_terminal_settings();
		exit(1);
	}
	m = write(log_fd, message, strlen(message));
	n = write(log_fd, buffer, strlen(buffer));
	if(n < 0 || m < 0) {
		fprintf(stderr, "Cannot write to log file\n");
		restore_terminal_settings();
		exit(1);
	}
	write(log_fd, lf, 1);
	return;
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

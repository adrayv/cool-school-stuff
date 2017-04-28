#include <termios.h>
#include <mcrypt.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <errno.h>
#include <fcntl.h>

struct option args[] = {
	{"port", 1, NULL, 'p'},
	{"encrypt", 1, NULL, 'e'},
	{0,0,0,0}
};

void print_usage_err();

int main(int argc, char** argv) {
	int arg_item;
	char* port_arg = NULL; // defaults to no argument
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
	while((arg_item = getopt_long(argc, argv, "pe", args, NULL)) != -1) {
		switch(arg_item) {
			case 'p':
				port_arg = optarg;
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
    int portno, sockfd, newsockfd, client_len;
    struct sockaddr_in server_addr, client_addr;

	/* establish portno */
	if(port_arg == NULL || strlen(port_arg) == 0) { // if user did not spcify an arg
		fprintf(stderr, "--port= is a necessarry argument\n");
		exit(1);
	}
	else {
		portno = atoi(port_arg); // assign the portno
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

	/* sets up sockfd */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) { // error checking for socket
    	fprintf(stderr, "Cannot create socket: %s\n" , strerror(errno));
    	exit(1);
    }
    
    /* sets the fields of server_addr */
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portno);
    server_addr.sin_addr.s_addr = INADDR_ANY;

	/* binds socket to address */
    if(bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
    	fprintf(stderr, "Cannot bind to socket: %s\n", strerror(errno));
    	exit(1);
    }

	/* listen for connection and assign a name to the socket */
    listen(sockfd,5);
    client_len = sizeof(client_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, &client_len);
    if(newsockfd < 0) {
    	fprintf(stderr, "Error calling accept()\n");
    	exit(1);
    }

    /* AT THIS POINT, A CONNECTION HAS BEEN MADE */

    /* Set up pipes */
	int pipe1[2]; // parent will write on pipe1
	int pipe2[2]; // parent will read from pipe2
	int ret = pipe(pipe1);
	if(ret == -1) {
		fprintf(stderr, "error creating first pipe\n");
		exit(1);
	}
	ret = pipe(pipe2);
	if(ret == -1) {
		fprintf(stderr, "error creating second pipe\n");
		exit(1);
	}

	/* fork to child */
	int pid = fork();
	if(pid < 0) {
		fprintf(stderr, "Error forking\n");
		exit(1);
	}
	else if(pid == 0) { // child process
		/* set up the child to read from pipe1 */
		close(pipe1[1]); // close the write end of this pipe1
		dup2(pipe1[0],0); // change stdin of child to read of pipe1
		close(pipe1[0]); // close the redundant fd to the read of pipe1

		/* set up the child to write to pipe2 */
		close(pipe2[0]); // close the read end of pipe2
		dup2(pipe2[1],1); // change the stdout of child to the write of pipe2
		dup2(pipe2[1],2); // change the stderr of child to the write of pipe2
		close(pipe2[1]); // close the redundant fd to the write of pipe2
	
		/* call exec to run /bin/bash */
		char *args[2];
		args[0] = "/bin/bash";
		args[1] = NULL;
		int exec_ret = execvp(args[0], args);
		if(exec_ret < 0) { // this line should never execute
			fprintf(stderr, "Error Calling execvp()\n");
			exit(1);
		}
	}
	else { // parent process
		/* IMPORTANT USAGE INFO 
		* parent writes to child through pipe1[1]
		* parent reads from child through pipe2[0]
		*/
		close(pipe2[1]); // parent will never write through pipe2
		close(pipe1[0]); // parent will never read from pipe1
	}
	/* AT THIS POINT, THE REST OF THE CODE IS THE PARENT */

	/* Set up polling to listen to client and shell */
	struct pollfd fds[2]; // create two things to listen to
	int poll_ret;
	char* lf = "\n";
	char* cr_lf = "\r\n";
	int status;
	char curr_char[1]; // buffer to read characters
	int n; // number of characters written or read

	fds[0].fd = newsockfd; // input from socket
	fds[1].fd = pipe2[0]; // input from shell
	fds[0].events = POLLIN | POLLHUP | POLLERR;
	fds[1].events = POLLIN | POLLHUP | POLLERR;

	/* poll to listen from client and shell */
	while(1) { 
		poll_ret = poll(fds, 2, 0);

		/* data to be read from client */
		if(fds[0].revents & POLLIN) { 
			n = read(newsockfd, curr_char, 1);
			if(n < 0) { // error check reading from client
				fprintf(stderr, "error reading from client\n");
				exit(1);
			}
			if(key_fd != -1) {
				block_buffer = curr_char[0];
				mdecrypt_generic(td2, &block_buffer, 1);
				curr_char[0] = block_buffer;
			}

			/* AT THIS POINT, every character is normal, not encrypted */

			if(curr_char[0] == '\r' || curr_char[0] == '\n') {
				n = write(pipe1[1], lf, 1); // send \n to shell
				if(n < 0) { // error check writing to socket
					fprintf(stderr, "Error writing to pipe\n");
					exit(1);
				}
			}
			else if(curr_char[0] == 0x03) { // send SIGINT to child
				int kill_ret = kill(pid, SIGINT);
				if(kill_ret == -1) {
					fprintf(stderr, "error sending SIGINT through kill\n");
					exit(1);
				}
			}
			else if(curr_char[0] == 0x04) { // upon receiving ^D
				close(pipe1[1]); //send EOF to shell
				/* after closing pipe, keep processing input from child */
			}
			else { // forward client message to shell
				n = write(pipe1[1], curr_char, 1);
				if(n < 0) { // error check writing to pipe
					fprintf(stderr, "Error writing to pipe\n");
					exit(1);
				}
			}
		}

		/* received EOF from client */
		if(fds[0].revents & (POLLHUP+POLLERR)) { 
			int kill_ret = kill(pid, SIGTERM);
			fprintf(stderr, "SIGTERM sent to shell\n");
			if(kill_ret == -1) {
				fprintf(stderr, "error sending SIGINT through kill\n");
				exit(1);
			}
		}

		/* data to be read from shell */
		if(fds[1].revents & POLLIN) { 
			n = read(pipe2[0], curr_char, 1); // read from pipe
			if(n < 0) { // error check reading from shell
				fprintf(stderr, "error reading from shell\n");
				exit(1);
			}
			if(key_fd != -1) { // valid encrytion is set
				/* encrypt before sending back to client */
				block_buffer = curr_char[0];
				mcrypt_generic(td, &block_buffer, 1);
				curr_char[0] = block_buffer;
			}
			n = write(newsockfd, curr_char, 1); // forward to client
			if(n < 0) {
				fprintf(stderr, "Error writing to client\n");
				exit(1);
			}
		}

		/* shell has exited */
		if(fds[1].revents & (POLLHUP+POLLERR)) { 
			//fprintf(stderr, "SHELL EXITED\n");
			break;
		}
	} // end of while loop

	int w, exit_sig, exit_status;
	do {
		w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
		if(w == -1) {
			fprintf(stderr, "waitpid error\n");
			exit(1);
		}
		if(WIFEXITED(status)) {
			exit_status = WEXITSTATUS(status);
		}
		else if(WIFSIGNALED(status)) {
			exit_sig = WTERMSIG(status);
		}
	} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	fprintf(stderr, "SHELL EXIT SIGNAL= %d STATUS= %d\n\r", exit_sig, exit_status);
	close(newsockfd);
	
	exit(0);
}

void print_usage_err() {
	printf("usage: ./lab1b-server [options]\n\t");
	printf("options:\n");
	printf("\t--port=\t\t\tspecify the port\n");
	printf("\t--encrypt=filename\tflag to specify encryption\n");
	return;
}

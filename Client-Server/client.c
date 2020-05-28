#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h> 
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include<signal.h>
#include <unistd.h>
#include <errno.h>

//defines
#define ERRORCODE 1

int sockfd = -1;


// displays error messages from system calls
void error(char *msg){
    perror(msg);
    exit(ERRORCODE);
}

int main(int argc, char *argv[]){

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sockfd, s;

    //check arguments
    if (argc != 3) {
	fprintf(stderr, "usage %s <hostname> <port>\n", argv[0]);
	exit(ERRORCODE);
    }

        /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* TCP socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    s = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (s != 0) {
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
	exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next) {
	sockfd = socket(rp->ai_family, rp->ai_socktype,
		     rp->ai_protocol);
	if (sockfd == -1)
	    continue;

	if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
	    break;                  /* Success */

	close(sockfd);
    }

    if (rp == NULL) {               /* No address succeeded */
	fprintf(stderr, "Could not connect\n");
	exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);           /* No longer needed */


    
    //get input from stdin
    char* buffer = NULL;
    size_t size = 0;
    int count;

    while(getline(&buffer, &size, stdin) > 0){
	int dataWritten =0; 
	int n;
	char *tmp;
	
	// write size
	tmp = buffer;
	count = strlen(buffer) + 1;
	if (write(sockfd, &count, sizeof(int)) < 0) {
	    fprintf (stderr, "Could not write size to socket\n");
	    exit (1);
	}
	//send input
	while (dataWritten < count) {
	    n = write(sockfd, tmp, count - dataWritten);
	    if (n < 0) {
		fprintf(stderr, "ERROR: couldn't write string %s to socket\n", buffer);
		exit(ERRORCODE);
	    }
	    dataWritten = dataWritten +n;
	    tmp = tmp +n;
	}
	free(buffer);
	buffer = NULL;
	size = 0;
    }

    count = -1;
    if (write(sockfd, &count, sizeof(int)) < 0) {
	fprintf (stderr, "Could not write size to socket\n");
	exit (1);
    }
    //close connection
    close (sockfd);
    return 0;
}

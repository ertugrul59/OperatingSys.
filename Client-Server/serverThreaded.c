#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>

#define ERRORCODE 1

// --- Data strcutures ---
typedef struct threadArgs{
    int socket;
} threadArgs;

unsigned int lineNo = 0;
char *logFileName;
pthread_mutex_t mut_lineNo; //lock for lineNo

// --- Implementation ---
void error(char *msg)
{
    perror(msg);
    exit(ERRORCODE);
}

char *readLine(int sockfd, int count) {
    char *tmp;
    char *buffer;
    int dataRead = 0;
    ssize_t size;
    
    buffer = malloc(count);
    if (buffer == NULL) {
	fprintf(stderr, "Cannot allocate memory\n");
	return NULL;
    }
    tmp = buffer;
    while (dataRead < count) {
	size = read(sockfd, tmp, count - dataRead);
	if (size < 0) {
	    fprintf(stderr, "Cannot read from socket\n");
	    free(buffer);
	    return NULL;
	}
	dataRead = dataRead + size;
	tmp = tmp + size;
    }
    return buffer;
}		

/* the procedure called for each request */
void *processRequest (void *in){
    
    char* buffer = NULL;
    int count;

    //parse arguments
    assert(in != NULL);
    threadArgs* args = (threadArgs*)in;


    //open file for append
    //opening a file multiple times is thread-safe
    //however, writing is not!
    FILE* f = fopen(logFileName, "a");
    if(f == NULL){
	fprintf(stderr, "Cannot open logfile - not processing request\n");
	goto fileout;
    }
  

    //read input

    if (read(args->socket, &count, sizeof(int)) < 0) {
	fprintf(stderr, "Cannot read from socket\n");
	goto out;
    }

    while(count != -1) {
	buffer = readLine(args->socket, count);
	if (buffer == NULL) {
	    goto out;
	}
	
	//lock to make getting the lineNo and fwrite atomic
	pthread_mutex_lock(&mut_lineNo);
	{
	    if (fprintf(f, "%u %s", lineNo, buffer) < 0) {
		pthread_mutex_unlock(&mut_lineNo);
		fprintf(stderr, "Could not write to logfile!\n");
		free(buffer);
		goto out;
	    }
	    lineNo++;
	    if (fflush(f)) {
		pthread_mutex_unlock(&mut_lineNo);
	        fprintf(stderr, "Could not write to logfile!\n");
		free(buffer);
		goto out;
	    }
	}
	pthread_mutex_unlock(&mut_lineNo);
	free(buffer);

	if (read(args->socket, &count, sizeof(int)) < 0) {
	    fprintf(stderr, "Cannot read from socket\n");
	    goto out;
	}

    }
    
    
    //avoid memory leaks
 out:    fclose(f);
 fileout:    assert(args != NULL);
    close(args->socket);
    free(args);
    pthread_exit (NULL);
}


int main(int argc, char *argv[]){

    //checks
    if (argc != 3) {
	fprintf(stderr, "usage %s <port> <logfile>\n", argv[0]);
	exit(ERRORCODE);
    }

    logFileName = argv[2];
    
    //get port
    errno = 0;
    unsigned long portno = strtoul(argv[1], NULL, 10);
    if(errno != 0 || portno > 65535){
	error("ERROR: Invalid port number");
    }
    //create server socket
    int sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if(sockfd < 0){
	error("ERROR: opening socket");
    }
    
    struct sockaddr_in6 serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_addr= in6addr_any;
    serv_addr.sin6_port = htons(portno);
  
    //bind socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
	error("ERROR: binding socket");
    }

    //ready to accept connections
    listen (sockfd, 5);
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof (cli_addr);
   
    //wait for connections
    while(1){
	//create thread arguments
    threadArgs* targs;
	targs = (threadArgs*)malloc(sizeof(threadArgs));
	if(!targs) {
	    fprintf (stderr, "ERROR: memory allocation failed\n");
	    exit(ERRORCODE);
	}
	//accept new connection
	targs->socket = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if(targs->socket < 0){
	    error("ERROR: accept");
	}

	//create separate thread for processing
	pthread_t server_thread;
	pthread_attr_t pthread_attr; //attributes for thread
	if (pthread_attr_init (&pthread_attr)) {
	    fprintf(stderr, "ERROR: creating initial thread attributes failed\n");
	    exit(ERRORCODE);
	}
	if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED)) {
	    fprintf(stderr, "ERROR: setting thread attributes failed\n");
	    exit(ERRORCODE);
	}
	int result = pthread_create(&server_thread, &pthread_attr, processRequest, (void*)targs);
	if (result != 0) {
	    fprintf(stderr, "ERROR: thread creation failed!\n");
	    exit(ERRORCODE);
	}
    }

    // not reached
    return 0; 
}

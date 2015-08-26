/**

**/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <strings.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

/**
Some constants
MAXCLIENTS:	maximum number of clients that can connect at a time
MAXMSG:		maximum message length
PORTNO:		server port number
**/
#define MAXCLIENTS 8
#define MAXMSG 256
#define PORTNO 12345

/**
Data type to hold all information regarding a client connection
index: 	position in the clients array
sd: 	socket descriptor for this client
tid: 	ID of the thread handling this client
name: 	hostname/IP of the client 
**/

/** prototypoes **/
void broadcast_msg(char *);
void * handle_client (void *);
int setup_server(void);
void cleanup (int );
int next_free(void);


int listenfd;

typedef struct {
	int index;
	int sd;
	pthread_t tid;
	char *name;
} client_t;



/**
Details of all clients currently connected to the server.
**/
static client_t *clients[MAXCLIENTS]={NULL};

static volatile sig_atomic_t quit = 0;

 

/**
Send a received message to each client currently connected 
**/
void broadcast_msg(char *message)  
{
	
	int i = 0;
	/** message recieved is sent to all clients including the sender also **/
	while(i<MAXCLIENTS){
		
		/*printf("%s\n",clients[i]->name);*/
		
		if(clients[i]!= NULL){
			
			send(clients[i]->sd,message,strlen(message),0);
		}
		i++;
	}

}



/**
Thread function that handles an individual client
**/
void * handle_client (void *arg)
{
	int connfd;
	char buffer[MAXMSG];
	int i=0, n;
	while (!quit)
	{
		
	/*read a message from this client*/ 
	connfd = *(int*)arg; 	/*should cast the "connfd_arg" to an integer to use in recv function */ 
	
	/* if EOF quit, otherwise broadcast it using broadcast_msg() */ 
			
	
	n = recv(connfd,buffer,MAXMSG,0);
	
	if(n==0){
		perror("A client has sihned out");
		break;
		}
	
	buffer[n] = 0;
	/** lock the broadcast_msg() function **/
	pthread_mutex_lock(&mutex);
	broadcast_msg(buffer);
	pthread_mutex_unlock(&mutex);	
	
	}
	/* Cleanup resources and free the client_t memory used by this client */
	
	while(i<MAXCLIENTS){
		if(clients[i]!=NULL){
			if(clients[i]->sd==connfd){
				free(clients[i]);
					
				clients[i]=NULL;
				}
			i++;
			}
		}
	perror("Client disconnected");
	close(connfd);
	return NULL;
}

/**
Initialise the server socket and return it
**/
int setup_server(void)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in servaddr;
	
	/* Initialize socket structure  */
	
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(PORTNO);

	
	/* Now bind the host address using bind() call */
	
	if(bind(sockfd ,(struct sockaddr *)&servaddr,sizeof(servaddr))){
		exit(1);
	}
		
	return sockfd; 
}

/** 
Find the next unused slot in the clients array and return its index 
Returns -1 if MAXCLIENTS is exceeded
**/
int next_free(void){	
	
	int i=0;
	
	while(i<MAXCLIENTS){
		if(clients[i] == NULL){
			return i;	
		}
		i++;
	}
	
	return -1;
}

/**
Signal handler to clean up on SIGINT (Ctrl-C)
**/
void cleanup (int signal)
{
	if(signal){
		close(listenfd);
		puts("Caught interrupt. Exiting...\n");
		quit = 1;
	}
}

int main( void )
{
	
	int* connfd;
	struct sockaddr_in cliaddr;
	socklen_t clilen;
	pthread_t mythread;
	int n, index;
	char buffer[1000];	
	char* banner = "Hello This is the server Please enter your name: "; 
	char* banner2 = "SORRY....chat room is full, So you have to wait until you get the chance....\n "; 
	
	/* Install signal handler for SIGINT */
	signal(SIGINT,cleanup);
	
	listenfd = setup_server();						

	listen(listenfd,MAXCLIENTS);

	clilen = sizeof(cliaddr);
	
	/* Initialise any synchronisation variables like mutexes, attributes and memory */
		

	while(!quit){
				
		/* Accept an incoming connection  */
		
		connfd = malloc(sizeof(int));
		*connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);		
		
		/** find the next free index **/
		index = next_free(); 
		
		if(index == -1){
			send(*connfd,banner2,strlen(banner2),0);
			close(*connfd);
			
		}
		
		else{
		
		send(*connfd,banner,strlen(banner),0);
		
		n = recv(*connfd,buffer,MAXMSG,0);
		
		buffer[n]=0;
			
		/* Allocate and set up new client_t struct in clients array */
		
		clients[index] = malloc(sizeof(client_t));
		clients[index]->index = index;
		clients[index]->sd = *connfd;
		clients[index]->name = buffer;
		
		/** threads are created here **/	
		if ( pthread_create( &mythread, NULL, handle_client, connfd) ){
				printf("error creating thread.");
				abort();
		}
		
		clients[index]->tid = mythread;
				
		
		/* Create a DETACHED thread to handle the new client until the quit is set */
		}
	}

	puts("Shutting down client connections...\n");
	close(listenfd);
	
	return 0;
}


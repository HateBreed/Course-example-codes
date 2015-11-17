/*
* CT30A5001 Network Programming
* server.c, TCP server and client example
*
* Contains simple TCP server that waits for a connection from some client and
* prints available addresses.
* 
* Author:
*   Jussi Laakkonen
*   1234567
*   jussi.laakkonen@lut.fi
*/

#include "server.h"
#include <errno.h>

#define SERVICE "6543"

volatile int running = 1;
volatile int NI_FLAGS = NI_NUMERICHOST;
volatile int AI_FLAGS = AI_CANONNAME;
volatile int ni_changes = 0;
volatile int ai_changes = 0;

void sighandler(int signal) {
	if(signal == SIGUSR1) {
		printf("\nChanging NI_FLAG, ");
		if (ni_changes == 4) {
			ni_changes = 0;
			NI_FLAGS = 0;
		}
		else {
			NI_FLAGS = (int)pow(2,ni_changes);
			ni_changes++;
		}
		printf("NI_FLAGS=%d, AI_FLAGS=%d\n\n",NI_FLAGS,AI_FLAGS);
	}
	else if (signal == SIGUSR2) {
		printf("\nChanging AI_FLAG, ");
		if(ai_changes > 4) ai_changes = 0; 
		AI_FLAGS = (int)pow(2,ai_changes)*2;//2,4,8,16,32
		ai_changes++;
		printf("NI_FLAGS=%d, AI_FLAGS=%d\n\n",NI_FLAGS,AI_FLAGS);
	}
	else {
		printf("Got some other signal, quitting.\n");
		running = 0;
	}
}

int main(void)
{
	int listen_sock = -1, accept_sock = -1;
	
	struct addrinfo hints = { .ai_flags = AI_PASSIVE, 	/* Get addresses suitable for bind */
				.ai_family = PF_INET6,		/* Only IPv6 */
				.ai_socktype = SOCK_STREAM,	/* Stream socket - TCP */
				.ai_protocol = IPPROTO_TCP};	/* TCP protocol */
							
	struct addrinfo cl_hints = { 	.ai_flags = AI_CANONNAME,
					.ai_family = PF_UNSPEC,		/* Not specified */
					.ai_protocol = 0,
					.ai_socktype = 0 };
																	
	struct addrinfo *result = NULL, *iter = NULL;
	struct sockaddr_storage client_addr;
	
	// Buffers for storing hostname and port
	char hostbuffer[NI_MAXHOST] = { 0 };
	char portbuffer[NI_MAXSERV] = { 0 };
	
	int sopt_yes = 1;
	
	struct sigaction signals = { 	.sa_handler = sighandler,		/* Handling function */
					.sa_flags = 0 };			/* Additional flags */
	sigemptyset(&signals.sa_mask);	/* Create new empty signal mask */
		
	// Add some signals
	sigaction(SIGINT, &signals,NULL);
	sigaction(SIGTERM,&signals,NULL);
	sigaction(SIGUSR1, &signals,NULL);
	sigaction(SIGUSR2, &signals,NULL);
	
	// Get my (AI_PASSIVE) addresses that are suitable for bind
	if(getaddrinfo(NULL,SERVICE,&hints,&result)) {
		perror("cannot get addresses for server");
		return -1;
	}
	
	// Go through all addresses returned with specified hints and use the first one
	for(iter = result; iter != NULL; iter = iter->ai_next) {
	
		// Create socket, if it fails attempt with next address
		if ((listen_sock = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol)) < 0) continue;
		
		// Set socket option SO_REUSEADDR
		if (setsockopt(listen_sock, SOL_SOCKET,SO_REUSEADDR,&sopt_yes,sizeof(sopt_yes)) < 0) perror("Cannot set SO_REUSEADDR");
		
		//Try to bind to this address
		if (bind(listen_sock,iter->ai_addr, iter->ai_addrlen) < 0 ) {
			perror("bind failed");
			close(listen_sock);
			listen_sock = -1;
			continue; // Try the next address
		}
		else {
			// Bind was succesfull, see what address we bound to
			if((getnameinfo(iter->ai_addr,iter->ai_addrlen,hostbuffer,sizeof(hostbuffer),NULL,0,0)) < 0 ) perror("cannot get nameinfo");
			else printf("Bound to %s address: %s\n",(iter->ai_family == PF_INET6 ? "IPv6" : "IPv4"), hostbuffer);
		}
		break;
	}
	
	// We got through the list and not single of the addresses was used or the listening socket does not exist
	if(!iter || listen_sock < 0 ) {
		printf("problems with listening socket");
		return -1;
	}
	
	freeaddrinfo(result);
	result = NULL;

	// Start to listen
	if(listen(listen_sock,1) < 0) {
		perror("listening failed");
		return -1;
	}
	
	printf("Starting to accept connections\n");

	while(running) {
	
		// Erase contents
		memset(&hostbuffer,0,NI_MAXHOST);
		memset(&portbuffer,0,NI_MAXSERV);
	
		// Initialize struct sockaddr to point to sockaddr_storage defined earlier
		socklen_t salen = sizeof(client_addr);
		struct sockaddr* cl_addr = (struct sockaddr*) &client_addr;

		// Accept a connection - note that this is a blocking function!
		if ((accept_sock = accept(listen_sock,cl_addr,&salen)) < 0) {
			if(errno != EINTR) perror("cannot accept");
			
			if(!running) break; // Stopped, do not proceed further
			else printf("Flags changed:\n");
		}

		// Who connected
		if(getnameinfo(cl_addr,salen,hostbuffer,sizeof(hostbuffer),portbuffer,sizeof(portbuffer),NI_FLAGS) < 0) {
			if(errno != EINTR) perror("cannot get client address");
		}
		else printf("Connection from: [%s]:%s\n",hostbuffer,portbuffer);
		
		// "Re"set the hints flag
		cl_hints.ai_flags = AI_FLAGS;
		
		// Get all addresses of the peer
		if(getaddrinfo(hostbuffer,portbuffer,&cl_hints,&result) < 0) {
			if(errno != EINTR) perror("cannot get other addresses for the host");
		}
		else {
			printf("The host has other addresses available:\n");
			// Print out these addresses with current configuration of NI_FLAGS
			for(iter = result; iter != NULL; iter = iter->ai_next) {
				memset(&hostbuffer,0,NI_MAXHOST);
				if(getnameinfo(iter->ai_addr,iter->ai_addrlen,hostbuffer,sizeof(hostbuffer),NULL,0,NI_FLAGS) < 0) perror("cannot get address");
				else printf("\t(%s) %s\n",(iter->ai_family == PF_INET6 ? "IPv6" : "IPv4"),hostbuffer);
			}
		}
		freeaddrinfo(result);
		result = NULL;
		
		close(accept_sock);
		accept_sock = -1;
	}
	
	printf("Quitting\n");
	
	close(listen_sock);
	
	return 0;
}

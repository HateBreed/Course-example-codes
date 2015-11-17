/*
* CT30A5001 Network Programming
* client.c, TCP server and client example
*
* Contains simple TCP client that connects to given IP/host and then quits.
* 
* Author:
*   Jussi Laakkonen
*   1234567
*   jussi.laakkonen@lut.fi
*/

#include "client.h"

int main(int argc, char* argv[]) {
	int sock = -1, optc = -1;
	
	struct addrinfo hints = { .ai_flags = 0,
					.ai_family = PF_UNSPEC,				/* Now we do not specify */
					.ai_socktype = SOCK_STREAM,
					.ai_protocol = IPPROTO_TCP};
														
	struct addrinfo *result = NULL, *iter = NULL;
	char *host = NULL, *port = NULL;
	extern char *optarg;
	extern int optopt;
	
	// Check command line options
	while ((optc = getopt(argc,argv,":64h:p:")) != -1) {
		switch (optc) {
			case 'h':
				host = optarg;
				break;
			case 'p':
				port = optarg;
				break;
			case '4':
				hints.ai_family=PF_INET; 		// IPv4
			case '6':
				hints.ai_family=PF_INET6; 	// IPv6
				break;
			case ':':
				printf("Parameter -%c is missing a operand\n", optopt);
				return -1;
			case '?':
				printf("Unknown parameter\n");
				break;
		}
	}
	
	// Are both given?
	if(!host || !port) {
		printf("Some parameter is missing\n");
		return -1;
	}
	else printf("Trying host [%s]:%s\n",host,port);
	
	// Get address info for the host
	if(getaddrinfo(host,port,&hints,&result) < 0) perror("Cannot resolve address");
	else {
		// Go through every returned address and attempt to connect to each
		for (iter = result; iter != NULL; iter = iter->ai_next) {
		
			// Create socket
			sock = socket(iter->ai_family,iter->ai_socktype,iter->ai_protocol);
			
			// Try to connect
			if(connect(sock,iter->ai_addr,iter->ai_addrlen) < 0) perror("Cannot connect");
			else printf("Connection to %s was successful!\n",host);
			
			// Clean
			close(sock);
			sock = -1;
		}
	}
	
	freeaddrinfo(result);

	return 0;
}

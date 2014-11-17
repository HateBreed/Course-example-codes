/* 
* CT30A5001 Network Programming
* sctpc.c, STCP server and client example
*
* Contains simple STCP client example code.
* Sends a 513 byte packet to server, awaits for reply and sends new
* data packet to server and awaits for second reply.
* 
* Author:
*   Jussi Laakkonen
*   1234567
*   jussi.laakkonen@lut.fi
*/

#define USE_CONNECT 0

#include "sctpc.h"

int main(int argc, char* argv[])
{
#ifdef __RUN_TESTS
	return run_test_client(argc,argv);
#else
	// Call your code here
	return 0;
#endif
}

int run_test_client(int argc, char* argv[])
{
	int sctp_sock = -1, gai_stat = 0;		// socket
	struct sockaddr_storage server_addr;		// server address structure
	struct sockaddr_storage peer_addr;		// peer address structure
	
	// Lengths for address structures
	socklen_t server_addr_len = 0;
	socklen_t peer_addr_len = 0;
	
	struct sctp_event_subscribe sctp_events;	// sctp events
	struct sctp_sndrcvinfo infodata;		// sctp sender info
	struct addrinfo hints,*ai = NULL,*iter = NULL;	// for getaddrinfo
	
	// invalid amount of parameters
	if(argc != 3) return testclient_input_error((argc == 1) ? argv[0] : NULL);
	
	// Use stroull for larger numbers, atoi() can be used for short ints
	unsigned int port = strtoull(argv[2],NULL,10);
	
	printf("connecting to: %s %u\n",argv[1],port);
		
	memset(&server_addr,0,sizeof(struct sockaddr_storage));
	
	// Get the server address structure with getaddrinfo()
	memset(&hints,0,sizeof(hints));
	hints.ai_family = PF_UNSPEC; 			// To allow IPv4 or IPv6
	hints.ai_protocol = IPPROTO_SCTP;		// SCTP protocol supported
	hints.ai_socktype = SOCK_SEQPACKET;
	
	// Get (and set) server info
	if((gai_stat = getaddrinfo(argv[1],argv[2],&hints,&ai)) < 0) {
		fprintf(stderr, "Cannot resolve: ");
 		print_addr_type(argv[1]);
		fprintf(stderr, "Error: %s\n",gai_strerror(gai_stat));
		if(ai) freeaddrinfo(ai);
		return -1;
	}
	
	// Use the first of the returned addresses
	for(iter = ai; iter != NULL; iter = iter->ai_next) {
	
	  // To allow the usage of both use IPv6 type for socket, create socket for SCTP
	  if((sctp_sock = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol)) < 0) {
	  	perror("socket()");
	  	if(ai) freeaddrinfo(ai);
	  	return -1;
	  }
	
	// Note that we do not need connect() with SCTP
#ifdef USE_CONNECT
#if USE_CONNECT == 1
	  if(connect(sctp_sock,(SA*)&server_addr,server_addr_len) < 0 ) perror("connect");
	  else printf("connected\n");
#endif
#endif

	memcpy(&server_addr,iter->ai_addr,iter->ai_addrlen);
	server_addr_len = iter->ai_addrlen;
	break;
	}
	freeaddrinfo(ai); // No need for this anymore
  
	// Allow to receive events from SCTP stack
	memset(&sctp_events,0,sizeof(sctp_events));
	set_sctp_events(&sctp_events);
	if(setsockopt(sctp_sock, IPPROTO_SCTP, SCTP_EVENTS, &sctp_events, sizeof(sctp_events)) < 0) perror("setsockopt");

	int byte_count = 513;
	char* data_to_send = testclient_fill_random_data(byte_count);	// create some data to send
	
	int sent_bytes = sctp_sendmsg(sctp_sock, 		// socket file descriptor
					data_to_send, 		// Data to send
					byte_count,     	// Amount of data to send in bytes
					(SA*)&server_addr,	// address of receiver
					server_addr_len,	// length of address structure
					666,			// Protocol payload identifier
					0,			// flags
					2,			// stream id
					0, 			// time to live, 0 = infinite
					0);
											
	printf("Sent %d bytes\n", sent_bytes);
	
	int recv_flags = 0;	
	int read_bytes = 0;
	
	/* Read until something is received from peer (server) just for demonstrative purposes
	   Use select() in your code
	*/
	while(1)
	{
		memset(data_to_send,0,byte_count);	// clear the buffer so new data can be written to it
		memset(&infodata,0,sizeof(infodata));
		recv_flags = 0;
		memset(&peer_addr,0,sizeof(peer_addr));
		
		read_bytes = sctp_recvmsg(sctp_sock,
						data_to_send,
						byte_count,
						(SA*)&peer_addr,
						&peer_addr_len,
						&infodata,
						&recv_flags);
		if(read_bytes > 0)
		{
			// Is a SCTP notification
			if(recv_flags & MSG_NOTIFICATION)
			{
				printf("Got notification: ");
				check_sctp_event(data_to_send,read_bytes);
				printf("\n");
			}
			// Otherwise data is received from peer
			else
			{
				printf("Got %d bytes on stream %d\n", read_bytes,infodata.sinfo_stream);
				break; // Got data, expecting 1 packet (ignore further notifications) -> break
			}
		}
		else break;
	}
	
	free(data_to_send);
	
	sleep(5);
	
	data_to_send = testclient_fill_random_data(byte_count);
	
	// We can send data also with sendto() when SCTP is used !
	sent_bytes = sendto(sctp_sock, 			// socket file descriptor
				data_to_send, 		// Data to send
				byte_count,     	// Amount of data to send in bytes
				0,			// Flags
				(SA*)&server_addr,	// address of receiver
				server_addr_len		// length of address structure
				);
	printf("Sent %d bytes\n", sent_bytes);
	
	
	
	while(1)
	{
		memset(&infodata,0,sizeof(infodata));
		recv_flags = 0;
		memset(&peer_addr,0,sizeof(peer_addr));
		memset(data_to_send,0,byte_count);
		
		read_bytes = sctp_recvmsg(sctp_sock,
						data_to_send,
						byte_count,
						(SA*)&peer_addr,
						&peer_addr_len,
						&infodata,
						&recv_flags);
																	
		if(read_bytes > 0)
		{
			// Is a SCTP notification
			if(recv_flags & MSG_NOTIFICATION)
			{
				printf("Got notification: ");
				check_sctp_event(data_to_send,read_bytes);
				printf("\n");
			}
			// Otherwise data is received from peer
			else
			{
				printf("Got %d bytes on stream %d\n", read_bytes,infodata.sinfo_stream);
				break; // Got data, expecting 1 packet (ignore further notifications) -> break
			}
		}
		else break;
	}
	
	free(data_to_send);
	close(sctp_sock);
	return 0;
}

int testclient_input_error(char* prgrm)
{
	printf("Not enough parameters");
	if(prgrm) printf(", use: %s <server ipv4/ipv6> <serverport>",prgrm);
	printf("\n");
	
	return -1;
}

char* testclient_fill_random_data(int max_bytes)
{
	char *data = NULL;
	
	if(max_bytes <= 0) return NULL;
	
	data = (char*)malloc(max_bytes);
	srand(time(NULL));
	
	for(int i = 0; i < max_bytes - 1 ; i++)
	{
		uint8_t r = random() % 125;
		if(r < 33) r+= 33;
		
		data[i] = (char)r;
	}
	data[max_bytes-1] = '\0';
#ifdef __DEBUG_EN
	printf("Created %d bytes:\n%s\n",max_bytes,data);
#endif
	return data;
}

void print_addr_type(char* address) {
	// Check that given address is either IPv4 or IPv6 type
	switch(check_addr_type(address,strlen(address)))
	{
		case 0:
#ifdef __DEBUG_EN
			printf("invalid address\n");
#endif
    break;
		case 1:
#ifdef __DEBUG_EN
			printf("interface not provided along IPv6 link local address\n");
#endif
      break;
		case 2:
#ifdef __DEBUG_EN
			printf("Invalid IPv6 address (invalid length)\n");
#endif
      break;
		case 4:
#ifdef __DEBUG_EN
			printf("IPv4 address\n");
#endif
			break;
		case 6:
#ifdef __DEBUG_EN
			printf("IPv6 address\n");
#endif
			break;
		case 10:
#ifdef __DEBUG_EN
			printf("IPv6 loopback address\n");
#endif
			break;
	}
}

int check_addr_type(char* addr,int len)
{
	if(!addr) return -1;
	
	// Assume that ':' exist multiple times and is IPv6 addr
	if(strchr(addr,':'))
	{
		
		if(len < 8) return 10; 	// Loopback IPv6 (0000::1 / 0000:: / ::1) 
		else if((len >= 14) && (len <= INET6_ADDRSTRLEN))	// Should be at least IPv4-mapped ::ffff:0.0.0.0
		{
			char ipv6prefix[5];
			memset(&ipv6prefix,0,5);
			
			// Copy 4 first chars and check for known prefixes (link local needed atm)
			memcpy(ipv6prefix,addr,4);
			ipv6prefix[4] = '\0';
			
			if(strcmp(ipv6prefix,"fe80") == 0) // Link local IPv6 address
			{
				if(strchr(addr,'%')) return 6; // Interface defined for link local IPv6 addr
				else return 1;
			}
			else return 6; // Otherwise assume that it is an IPv6 address, getaddrinfo reports validity
		}
		else return 2; // The length for IPv6 address is not valid
	}
	else
	{
		if(len < 7) return 0; // should be at least 0.0.0.0

		int pos = 0;
		
		for(int i = 0; i < 3; i++) // at least 3 '.'
		{
			char* tmp = strchr(&addr[pos],'.');
			if(tmp) pos = len - (strlen(tmp)) + 1;
			else return 0; // No more '.' found
		}
		return 4;
	}
}

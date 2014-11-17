/* 
* CT30A5001 Network Programming
* sctps.c, STCP server and client example
*
* Contains simple STCP server that the client connects to. Echoes the sent data
* back to client and shows all available addresses for it. Actively listens for
* notificatioins from SCTP kernel.
* 
* Author:
*   Jussi Laakkonen
*   1234567
*   jussi.laakkonen@lut.fi
*/


#include "sctps.h"

#define __BINDX_SEPARATELY

// Main loop global - Ctrl-C changes this to 0
static volatile int running = 1;

int main(int argc, char* argv[])
{
#ifdef __RUN_TESTS
	return run_test_server(argc,argv);
#else
	// Call your server code here
	return 0;
#endif
}

int run_test_server(int argc, char* argv[])
{
	int sctp_otm_sock = -1, port_otm = -1;		// socket and port for one to many sctp
	int count = 0;
	char mbuffer[MBUFFERLEN];			// static message buffer
	struct sockaddr* addresses = NULL;
	struct sctp_event_subscribe sctp_events;  	// sctp events
	struct sctp_sndrcvinfo infodata;		// information about sender
	struct sctp_initmsg sctp_init;			// sctp socket option initialization
	struct sockaddr *packedlist = NULL;		// packed list of addresses
	
	// Add listening for 2 interruption signals
	signal(SIGTERM,sighandler);
	signal(SIGINT,sighandler);
	
	// not enough or too many parameters
	if(argc > 3 || argc < 2)
		return testserver_input_error((argc == 1) ? argv[0] : NULL);
	
	// Port is not valid
	if((port_otm = atoi(argv[1])) <= 0x0400 || port_otm > 0xffff)
		return testserver_input_error(NULL);
	
	printf("starting server on port:\t%d\n",port_otm);
	
	// Socket creation
	if((sctp_otm_sock = socket(PF_INET6, SOCK_SEQPACKET,IPPROTO_SCTP)) == -1)
		return error_situation();
	
	if(argc == 3) {
		if(strcasecmp(argv[2],"ifaddrs") == 0)
			addresses = get_own_addresses_gia(&count,port_otm,NOLOOPBACK,ONLYRUNNING);
		else if(strcasecmp(argv[2],"addrinfo") == 0)
			addresses = get_own_addresses_gai(&count,argv[1]);
		else if(strcasecmp(argv[2],"combined") == 0)
			addresses = get_own_addresses_combined(&count,argv[1],NOLOOPBACK,ONLYRUNNING);
		else {
			printf("Invalid parameter for address search function selection.\n \
				\tifaddrs = getifaddrs()\n\taddrinfo = getaddrinfo()\n");
			exit(1);
    		}
	}
	else
		addresses = get_own_addresses_gia(&count,port_otm,NOLOOPBACK,ONLYRUNNING);

	print_packed_addresses(count,(struct sockaddr_storage*)addresses);
	
	if(!addresses) printf("No addresses to bind\n");
#ifdef __BINDX_SEPARATELY
	else {
		int i = 0, storlen = 0, slen = 0;
	  
		// Start from the beginning
		struct sockaddr* temp = addresses;
	  
	 	while (i < count) {
	    
	    		// We need to know the size
			slen = (temp->sa_family == AF_INET ? 
				sizeof(struct sockaddr_in) : 
				temp->sa_family == AF_INET6 ? 
				sizeof(struct sockaddr_in6) : 
				0 );
	    
			// Attempt to bind one address we're pointing at the moment
			if(sctp_bindx(sctp_otm_sock,temp,1,SCTP_BINDX_ADD_ADDR) != 0) {
				perror("Bindx failed");
				break;
			}
			else {
				printf("Added address");
				print_packed_addresses(1,(struct sockaddr_storage*)temp);
			}
	    
			i++;
			storlen += slen; // Add the size of current struct
			temp = (struct sockaddr*)((char*)temp + slen); // Move pointer to next struct
		}
	}
#else
	else if (sctp_bindx(sctp_otm_sock,addresses,count,SCTP_BINDX_ADD_ADDR) != 0)
	perror("Bindx failed");
#endif
	
	if(addresses) free(addresses); // No need for this anymore
	
	// Set to receive events
	memset(&sctp_events,0,sizeof(sctp_events));
	set_sctp_events(&sctp_events);
	if(setsockopt(sctp_otm_sock,IPPROTO_SCTP,SCTP_EVENTS,&sctp_events,sizeof(sctp_events)) != 0)
		perror("Setsockopt failed");
	
	// Set stream count and attempt counter
	memset(&sctp_init,0,sizeof(sctp_init));
	
	sctp_init.sinit_num_ostreams = 3; 	// out streams
	sctp_init.sinit_max_instreams = 3; 	// in streams
	sctp_init.sinit_max_attempts = 2;	// maximum number for connection attempts
	
	if(setsockopt(sctp_otm_sock,IPPROTO_SCTP,SCTP_INITMSG,&sctp_init,sizeof(sctp_init)))
		perror("Setsockopt failed");
	
	// Start listening
	if(listen(sctp_otm_sock,10) != 0)
		perror("Listen failed");
	
	// For select()
	fd_set incoming;
	struct timeval to;
	memset(&to,0,sizeof(to));
	to.tv_usec = 100*5000;
	to.tv_sec = 0;
	
	// For sender address check
	char str_addr[NI_MAXHOST] = { 0 };
	struct sockaddr_storage sender = { 0 };
	
	while(running)
	{
		FD_ZERO(&incoming);
		
		FD_SET(sctp_otm_sock,&incoming);
		
		switch(select(sctp_otm_sock+1,&incoming,NULL,NULL,&to))
		{
			case -1:
				perror("Select failed, terminating");
				running = 0;
				break;
			case 0:
				break;
			default:
			{
				memset(&sender,0,sizeof(sender));
				memset(&infodata,0,sizeof(infodata));
				memset(&mbuffer,0,MBUFFERLEN);
				
				unsigned int salen = sizeof(sender);
				int mlen = MBUFFERLEN;
				int mflags = 0;
				
				// Receive the message
				int bytes = sctp_recvmsg(sctp_otm_sock,				// socket
								mbuffer,			// receive buffer for message
								mlen,				// length of message
								(struct sockaddr*)&sender,	// sender address info
								&salen,				// length of sender address info
								&infodata,			// sctp information about sender
								&mflags);			// message flags
				
				memset(&str_addr,0,NI_MAXHOST);
				
				// Received a SCTP notification
				if(mflags & MSG_NOTIFICATION)
				{
					printf("A notification from association %d (%d bytes): ",
						infodata.sinfo_assoc_id,bytes);
					check_sctp_event(mbuffer,bytes);
				}
				// Something else from a client
				else
				{
					if(getnameinfo((struct sockaddr*)&sender,
				  		salen,
						str_addr,
						NI_MAXHOST,
						NULL,
						0,
						0) < 0)
						perror("cannot get address nameinfo");
					else {
						printf("Got something from ");
						switch(sender.ss_family) {
							case AF_INET:
								printf("IPv4");
								break;
							case AF_INET6:
								printf("IPv6");
								break;
							default:
								printf("unknown address family %d",sender.ss_family);
								break;
						}
						printf(" address: %s",str_addr);
					}
					
					printf("\n\t%d bytes from association %u\n",bytes, infodata.sinfo_assoc_id );

					// Get all addresses of the peer
					int addr_count = sctp_getpaddrs(sctp_otm_sock,infodata.sinfo_assoc_id,&packedlist);
					if(addr_count < 0) perror("sctp_getpaddrs failed");
					else
					{
						printf("\tAddresses (%d):\n",addr_count);
						salen = print_packed_addresses(addr_count,(struct sockaddr_storage*)packedlist);
					}

					// Send the message back to client
					bytes = sctp_sendmsg(sctp_otm_sock,		// socked fd
								&mbuffer,		// message
								bytes,			// message length
								packedlist,		// list of addresses
								salen,			// length of sender data
								infodata.sinfo_ppid,	// process payload identifier
								infodata.sinfo_flags,	// flags
								infodata.sinfo_stream++,  // stream number
								0,			// ttl
								0);			// context
				
					printf("Sent %d bytes\n",bytes);

					// Free the addresses of the peer
					sctp_freepaddrs(packedlist);
					packedlist = NULL;
				}
				break;
			}
		}
		usleep(1000);
	}
	
	close(sctp_otm_sock);
	return 0;
}


void sighandler(int sig)
{
	printf("\nCaught an interrupt (%s), doing shutdown in 1 second\n", (sig == SIGTERM) ? "SIGTERM" : "SIGINT" );
	sleep(1);
	running = 0;
}

int testserver_input_error(char* prgrm)
{
	printf("Not enough parameters or invalid parameters");
	if(prgrm) printf(", use: %s <server_one_to_many_port>",prgrm);
	printf("\n");
	
	return -1;
}


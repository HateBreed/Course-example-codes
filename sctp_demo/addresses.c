/* 
* CT30A5001 Network Programming
* addresses.c, STCP server and client example
*
* Contains functions for getting own addresses for the SCTP example
* 
* Author:
*   Jussi Laakkonen
*   1234567
*   jussi.laakkonen@lut.fi
*/

#include "addresses.h"

struct sockaddr* get_own_addresses_gai(int* count, char* port)
{
	int status = 0, icount = 0, strsizetotal = 0;
	struct addrinfo searchinfo = { 	.ai_flags = AI_PASSIVE|AI_NUMERICHOST|AI_NUMERICSERV,
					.ai_family = PF_UNSPEC,
					.ai_socktype = SOCK_SEQPACKET,
					.ai_protocol = IPPROTO_SCTP};
	struct addrinfo *addresses = NULL, *iter = NULL;
  	char* addrbuffer = NULL;
#ifdef __DEBUG_EN
  	char ipstr[NI_MAXHOST] = { 0 };
#endif
	
	if((status = getaddrinfo(NULL,port,&searchinfo,&addresses) != 0))
	{
		printf("%s\n",gai_strerror(status));
		error_situation();
	}
	else
	{
		for(iter = addresses; iter != NULL; iter = iter->ai_next)
		{
			strsizetotal += iter->ai_addrlen; // The new total size for the buffer
			
			if(!addrbuffer)	addrbuffer = (char*)malloc(iter->ai_addrlen); // Not allocated
			else addrbuffer = (char*)realloc(addrbuffer,strsizetotal);    // Increase allocation
							
			// Copy the address structure into the buffer
			memcpy(&addrbuffer[strsizetotal-(iter->ai_addrlen)],iter->ai_addr, iter->ai_addrlen);
#ifdef __DEBUG_EN
			memset(&ipstr,0,NI_MAXHOST);
			if(getnameinfo(iter->ai_addr,
				iter->ai_addrlen,
				ipstr,
				NI_MAXHOST,
				NULL,
				0,
				NI_NUMERICHOST) < 0 )
				print_host(icount,iter->ai_family,NULL,NULL);
			else print_host(icount,iter->ai_family,NULL,ipstr);
#endif
 			icount++;
		}
	}
	if(addresses) freeaddrinfo(addresses);
	*count = icount;
	return (struct sockaddr*)addrbuffer;
}

struct sockaddr* get_own_addresses_gia(int* count, int port, int noloopback, int onlyrunning)
{
	struct ifaddrs *ifaddr = NULL, *ifa = NULL;
	int family = 0, strsizetotal = 0;
#ifdef __DEBUG_EN
	int s = 0;
	char host[NI_MAXHOST] = {0};
#endif
	char *addrbuffer = NULL;

// Get all addresses for every interface
	if (getifaddrs(&ifaddr) == -1)
	{
		error_situation();
		return NULL;
	}
	
	int icount = 0;

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		family = ifa->ifa_addr->sa_family;

		// Only addresses supporting IPv4 or IPv6
		if (family == AF_INET || family == AF_INET6)
		{
			// Allow only active interfaces, ignore local
			int runningflag = 0, loopbackflag = 0;
			
			if(onlyrunning) runningflag |= IFF_RUNNING;
			else runningflag |= IFF_UP;
			
			if(noloopback) loopbackflag |= IFF_LOOPBACK;
			
			if(!(ifa->ifa_flags & loopbackflag) && (ifa->ifa_flags & runningflag))
			{
				icount++;
				
				// Size of the current structure
				int nsize = (family == AF_INET) ? 
					sizeof(struct sockaddr_in) : 
					(family == AF_INET6) ? 
					sizeof(struct sockaddr_in6) :
					0 ;
				
				strsizetotal += nsize; // The new total size for the buffer
				
				if(!addrbuffer)	addrbuffer = (char*)malloc(nsize); // Not allocated
				else addrbuffer = (char*)realloc(addrbuffer,strsizetotal);
				
				// IPv4
				if(family == AF_INET)
				{
					struct sockaddr_in* v4addr = (struct sockaddr_in*)ifa->ifa_addr;
					
					// Set port as sctp_bindx requires port to be same in each structure
					v4addr->sin_port = htons(port);
					v4addr->sin_family = family;
					
					// Copy the IPv4 address structure into the buffer
					memcpy(&addrbuffer[strsizetotal-nsize],v4addr, nsize);
				}
				// Otherwise IPv6
				else if(family == AF_INET6)
				{
					struct sockaddr_in6* v6addr = (struct sockaddr_in6*)ifa->ifa_addr;
					
					// Set port as sctp_bindx requires port to be same in each
					v6addr->sin6_port = htons(port);
					v6addr->sin6_family = family;
								
					// Copy the IPv6 address structure into the buffer
					memcpy(&addrbuffer[strsizetotal-nsize],v6addr, nsize);
				}
				else printf("Unknown address family: %d IGNORED\n", family);

#ifdef __DEBUG_EN				
				// Get and print the IP address
				if((s = getnameinfo(ifa->ifa_addr,
				(family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
				host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST)) == 0)
					print_host(icount,family,ifa->ifa_name,host);
#endif
			}
		}
	}

	freeifaddrs(ifaddr);
	*count = icount;

	return (struct sockaddr *)addrbuffer;
}

struct sockaddr* get_own_addresses_combined(int* count, char* port, int noloopback, int onlyrunning)
{
	struct ifaddrs *ifaddr = NULL, *ifa = NULL;
	struct addrinfo *result = NULL;
	struct addrinfo hints = { 	.ai_flags = AI_NUMERICHOST,
					.ai_family = PF_UNSPEC,
					.ai_socktype = SOCK_SEQPACKET,
					.ai_protocol = IPPROTO_SCTP };
	int family = 0, strsizetotal = 0;
	char host[NI_MAXHOST] = {0};
	char *addrbuffer = NULL;

	// Get all addresses for every interface
	if (getifaddrs(&ifaddr) == -1)
	{
		error_situation();
		return NULL;
	}
	
	int icount = 0;

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		family = ifa->ifa_addr->sa_family;

		// Allow only active interfaces, ignore local
		int runningflag = 0, loopbackflag = 0;
		
		if(onlyrunning) runningflag |= IFF_RUNNING;
		else runningflag |= IFF_UP;
		
		if(noloopback) loopbackflag |= IFF_LOOPBACK;
		
		if(!(ifa->ifa_flags & loopbackflag) && (ifa->ifa_flags & runningflag))
		{	
			// Size of the current structure, this is protocol dependant
			int nsize = (family == AF_INET) ?
				sizeof(struct sockaddr_in) :
				(family == AF_INET6) ?
				sizeof(struct sockaddr_in6) :
				0;
			
			memset(&host,0,NI_MAXHOST);
			
			// Get the IP for this structure
			if(getnameinfo(ifa->ifa_addr,
				nsize,
				host,
				NI_MAXHOST,
				NULL,
				0,
				NI_NUMERICHOST) < 0)
				continue;
			
			// Fill in struct for this address
			if(getaddrinfo(host,port,&hints,&result) < 0) { 
			  freeaddrinfo(result);
			  result = NULL;
			  continue;
			}
			
			// Use the first
			if(result) {
			  strsizetotal += result->ai_addrlen;
			  
			if(!addrbuffer)	addrbuffer = (char*)malloc(result->ai_addrlen); // Not allocated
  			else addrbuffer = (char*)realloc(addrbuffer,strsizetotal);
  			
  			memcpy(&addrbuffer[strsizetotal-(result->ai_addrlen)],
				result->ai_addr,
				result->ai_addrlen);
  			
  			icount++; // One was added, increase counter
			}
			
			freeaddrinfo(result);
			result = NULL;

#ifdef __DEBUG_EN				
			// Print the IP address
			print_host(icount,family,ifa->ifa_name,host);
#endif
		}
	}

	freeifaddrs(ifaddr);
	*count = icount;

	return (struct sockaddr *)addrbuffer;
}

int error_situation()
{
	perror("Error");
	return -1;
}

void print_host(int id,int family,char* iface, char* host) {
	printf("%d:\t[%s]\t%s\t%s\n",
		id,
		(family == PF_INET ? "IPv4" : "IPv6"),
		(iface == NULL ? "<?>" : iface),
		(host == NULL ? "no IP" : host));
}


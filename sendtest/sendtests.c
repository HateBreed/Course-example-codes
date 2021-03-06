#include "sendtest.h"

#define INITIAL_GUESS 65536
//#define USE_OLD_ABI
int read_from_socket(int,char*, unsigned int, unsigned int);

int main(int argc, char* argv[])
{
	if(argc != 2) return -1;
	
	int sockfdu = -1;
	int sockfd = -1;
	int port = atoi(argv[1]);
	
	socklen_t alen = sizeof(struct sockaddr_in);

#ifdef USE_OLD_ABI
	sockfdu = socket(AF_INET,SOCK_DGRAM,0);
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd < 0) perror("tcp socket error");
	if(sockfdu < 0) perror("udp socket error");

	// Set own addr struct
	struct sockaddr_in own;
	memset(&own,0,sizeof(own));
	own.sin_family = AF_INET;
	own.sin_addr.s_addr = INADDR_ANY;
	own.sin_port = htons(port);

	if(bind(sockfd,(struct sockaddr*)&own,alen) < 0) perror("bind tcp");
	own.sin_port = htons(port+1);
	if(bind(sockfdu,(struct sockaddr*)&own,alen) < 0) perror("bind udp");

#else
	struct addrinfo hints = {	.ai_flags = AI_PASSIVE,
					.ai_family = PF_UNSPEC,
					.ai_socktype = SOCK_STREAM,
					.ai_protocol = IPPROTO_TCP};
  
	struct addrinfo *result = NULL, *iter = NULL;
	
	char port_str[NI_MAXSERV] = { 0 };
	
	memcpy(&port_str,argv[1],NI_MAXSERV);
	
	// Create and bind TCP socket
	if(getaddrinfo(NULL,port_str,&hints,&result) < 0)
	{
		perror("getaddrinfo() TCP");
		return -1;
	}
		
	for(iter = result; iter != NULL; iter = iter->ai_next)
	{
		if ((sockfd = socket(iter->ai_family,iter->ai_socktype,iter->ai_protocol)) < 0)
		{
			perror("socket() TCP");
			return -1;
		}
			
		//Try to bind to this address
		if (bind(sockfd,iter->ai_addr, iter->ai_addrlen) < 0)
		{
			close(sockfd);
			perror("bind() TCP");
			return -1;
      		}
		break;
	}
	
	printf("Created and bound TCP socket using port %s\n",port_str);
	
	freeaddrinfo(result);
	result = NULL;
	
	// Create and bind UDP socket
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	
	memset(&port_str,0,NI_MAXSERV);
	snprintf(port_str,NI_MAXSERV,"%d",port+1);
	
	if(getaddrinfo(NULL,port_str,&hints,&result))
	{
		perror("getaddrinfo() UDP");
		return -1;
	}
		
	for(iter = result; iter != NULL; iter = iter->ai_next)
	{
		if ((sockfdu = socket(iter->ai_family,iter->ai_socktype,iter->ai_protocol)) < 0)
		{
			perror("socket() UDP");
			return -1;
		}
			
		//Try to bind to this address
		if (bind(sockfdu,iter->ai_addr, iter->ai_addrlen) < 0)
		{
			close(sockfdu);
			perror("bind() UDP");
			return -1;
      		}
		break;
	}
	
	printf("Created and bound UDP socket using port %s\n",port_str);
	
	freeaddrinfo(result);
#endif

	listen(sockfd,1);
	 
	struct sockaddr_in conn;
	memset(&conn,0,sizeof(conn));
	
	int newsock = accept(sockfd,(struct sockaddr*)&conn,&alen);
	 
	if(newsock < 0) perror("accept");

	unsigned int total = INITIAL_GUESS; // Initial guess
	unsigned int remaining = total; // We're missing as much data
	int first_read = 1, data_read = 0;
	char* buf = (char*)calloc(total,sizeof(char)); // Initial buffer

	do
	{
		// Read 0 or less -> we're done here
		if((data_read = read_from_socket(newsock,buf,total,remaining)) < 1) break;		
		
		// New packet
		if(first_read)
		{
			// read enough to get the packet size
			if(data_read >= sizeof(uint32_t))
			{
				total = ntohl(*(uint32_t*)&buf[0]); // size
				printf("first read (%d bytes), packet total size = %u\n",data_read,total);
				
				// increase the buffer allocation
				if(total > INITIAL_GUESS) buf = realloc(buf,total);
				
				remaining = total - data_read; // Update remaining amount
				first_read = 0;
			}
			// otherwise update the remaining amount
			else remaining -= data_read;
		}
		else
		{
			remaining -= data_read; // Update remaining amount
			printf("read\t\t%u\tbytes\nremaining\t%u\tbytes\n",
				data_read, remaining);
		}
	
	} while (remaining > 0 || first_read == 1);
	if(!remaining) printf("Got all data over TCP\n");
	
	printf("Waiting on UDP on port %d\n",port+1);
	first_read = 1;
	unsigned int udata_read = 0;
	uint32_t packetsize = 0;
	uint32_t packets = 0;
	uint32_t utotal = INITIAL_GUESS;
	uint32_t uremaining = 0;
	uint32_t receivedpackets = 0;
	uint32_t receivedamount = 0;

	char* ubuf = (char*)calloc(utotal,sizeof(char)); // Initial buffer
	
	fd_set readset;
	long time_to_wait = 2;

	do
	{
		// New packet
		if(first_read)
		{
			int initialsize = sizeof(uint32_t)*2;
			char initial[initialsize];
			memset(&initial,0,initialsize);
			// read enough to get the packet size
			if((udata_read = recvfrom(sockfdu,&initial,initialsize,0,(struct sockaddr*)&conn,&alen)) == initialsize)
			{
				packetsize = ntohl(*(uint32_t*)&initial[0]); // one packet size
				packets = ntohl(*(uint32_t*)&initial[sizeof(uint32_t)]); // packet count
				
				utotal = packetsize * packets; // Estimate, last might be less?

				printf("first read, data total size = %u (%u bytes per packet * %u packets\n",
						utotal,packetsize,packets);

				// increase the buffer allocation
				ubuf = realloc(ubuf,utotal);
				uremaining = utotal; // Remaining estimated data, actually remaining the reported amount of packets
				first_read = 0;
			}
			else
			{
				printf("Read less than %d bytes (header size), cannot proceed\n",data_read);
				return 1;		
			}
		}
		else
		{
			FD_ZERO(&readset);
			FD_SET(sockfdu,&readset);
			struct timeval timeset;
			timeset.tv_sec = time_to_wait;
			timeset.tv_usec = 0;
			
			if(select(sockfdu+1,&readset,NULL,NULL,&timeset) < 1)
			{
				printf("Did not get data from UDP in %lu seconds\n",time_to_wait);
				break;
			}
			else
			{
				long time_spent_sec = time_to_wait - timeset.tv_sec == 1 && timeset.tv_usec > 0 ? 0 : time_to_wait - timeset.tv_sec;
				long time_spent_usec = 1000000 - timeset.tv_usec;
				printf("Waited %lu seconds and %lu microseconds in select()\n",time_spent_sec,time_spent_usec);
			}
			
			if(FD_ISSET(sockfdu,&readset))
			{
				// Read data from udp socket and continue where last time left off (receivedamount)
				udata_read = recvfrom(sockfdu,&ubuf[receivedamount],packetsize,0,(struct sockaddr*)&conn,&alen);

				uremaining -= udata_read; // Update remaining estimated amount

				if(udata_read == packetsize) receivedpackets++; // Got full packet, increase counter
				else if (udata_read < packetsize && receivedpackets+1 == packets) receivedpackets++; // last packet?
				else printf("read less and it is not last packet\n");
				receivedamount += udata_read;
				printf("read\t\t%u\tbytes\nremaining (estimated)\t%u\tbytes (p%d, total=%d) \n",
					udata_read,uremaining,receivedpackets,receivedamount);
			}
		}

	} while (receivedpackets < packets);

	if(remaining == 0) 
	{
		printf("got full TCP packet:\nprint %u bytes of data? (y/n):",total);
		switch(getc(stdin))
		{
			case 'y':
				for(int i = sizeof(uint32_t); i < total; i++) printf("%c%s",buf[i], i+1 == total ? "\n" : "");
				break;
			case 'n':
				break;
			default:
				printf("invalid char.\n");
				break;
		}
	}
	else printf("got incomplete TCP packet, %u/%u bytes received\n",total-remaining,total);

	getc(stdin);

	if(receivedpackets == packets)
	{
		printf("got full UDP packet:\nprint %u bytes of data? (y/n):",receivedamount);
		switch(getc(stdin))
		{
			case 'y':
				for(int i = sizeof(uint32_t); i < utotal; i++) printf("%c%s",ubuf[i], i+1 == utotal ? "\n" : "");
				break;
			case 'n':
				break;
			default:
				printf("invalid char.\n");
				break;
		}
	}
	else printf("got incomplete UDP packet, %u/%u bytes received\n",utotal-uremaining,utotal);
	
	free(buf);
	free(ubuf);
	close(newsock);
	close(sockfd);
	close(sockfdu);

	printf("freed all, quitting\n");
	return 0;
}

int read_from_socket(int sockfd,char* buf, unsigned int total, unsigned int remaining)
{
	int data_read = 0;
	
	// Read 
	if((data_read = recv(sockfd,&buf[total-remaining],remaining,0)) < 0)
	{
		perror("recv");
		return -1;
	}

	return data_read;
}

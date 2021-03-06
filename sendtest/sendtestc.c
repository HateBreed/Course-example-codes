#include "sendtest.h"

int write_to_tcp_socket(int,char*,unsigned int, unsigned int*);
int write_to_udp_socket(int,char*,unsigned int, unsigned int*,
	struct sockaddr_in,socklen_t);
//#define USE_OLD_ABI 1
int main(int argc, char* argv[])
{
	if(argc < 3 || argc > 4) return -1;
	
	unsigned int len = argc == 4 ? atoi(argv[3]) : BLEN, wrote = 0;
	char buf[len];
	
	// Initialize the packet
	memset(&buf,0,len);
	*(uint32_t*)&buf[0] = htonl(len); // Size at the front
	memcpy(&buf[sizeof(uint32_t)],"<START>",7); // Start tag
	for(int i = sizeof(uint32_t)+7; i < len-5; i++) buf[i] = 65+(i%20); // data
	memcpy(&buf[len-5],"<END>",5); // end tag
	
	int sockfd = -1;
	int sockfdu = -1;
	
	struct sockaddr_in peer_tcp,peer_udp;
	memset(&peer_tcp,0,sizeof(peer_tcp));
	memset(&peer_udp,0,sizeof(peer_udp));
	socklen_t alen = sizeof(struct sockaddr_in);

#ifdef USE_OLD_ABI
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	sockfdu = socket(AF_INET,SOCK_DGRAM,0);
	
	if(sockfd < 0 || sockfdu < 0)
	{
		perror("sock");
		return -1;
	}

	// Set the server info, no checks in this demo
	// argv[1] = addr, argv[2] = port
	peer_tcp.sin_family = AF_INET;

	if(inet_pton(AF_INET,argv[1],&(peer_tcp.sin_addr)) <= 0)
		perror("pton");

	peer_tcp.sin_port = htons(atoi(argv[2]));
	memcpy(&peer_udp,&peer_tcp,alen);
	peer_udp.sin_port = htons(atoi(argv[2])+1);
#else
	struct addrinfo hints = {	.ai_flags = 0,
					.ai_family = PF_INET, // IPv4 only
					.ai_socktype = SOCK_STREAM,
					.ai_protocol = IPPROTO_TCP};
	
	struct addrinfo *result = NULL, *iter = NULL;
	
	char port_str[NI_MAXSERV] = { 0 };
	memcpy(&port_str,argv[2],NI_MAXSERV);
	
	if(getaddrinfo(argv[1],port_str,&hints,&result) < 0) 
	{
		perror("Cannot resolve address");
		return -1;
	}
	
	// Create TCP socket
	for(iter = result; iter != NULL; iter = iter->ai_next) 
	{
		if((sockfd = socket(iter->ai_family,iter->ai_socktype,iter->ai_protocol)) < 0) 
			return -1;
		break;
	}
	// Copy the address info for TCP
	if(iter->ai_addrlen == alen) memcpy(&peer_tcp,iter->ai_addr,iter->ai_addrlen);
	else printf("Mismatching struct size\n");
	
	freeaddrinfo(result);
	result = NULL;
	
	// Establish information for UDP
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	
	memset(&port_str,0,NI_MAXSERV);
	snprintf(port_str,NI_MAXSERV,"%d",atoi(argv[2])+1); // port+1
	
	if(getaddrinfo(argv[1],port_str,&hints,&result) < 0)
	{
		perror("Cannot resolve address");
		return -1;
	}
	
	// Create UDP socket
	for(iter = result; iter != NULL; iter = iter->ai_next) 
	{
		if((sockfdu = socket(iter->ai_family,iter->ai_socktype,iter->ai_protocol)) < 0)
			return -1;
		break;
	}
	
	// Copy the address info for UDP
	if(iter->ai_addrlen == alen) memcpy(&peer_udp,iter->ai_addr,iter->ai_addrlen);
	else printf("Mismatching struct size\n");
	
	freeaddrinfo(result);

#endif
	
	if(connect(sockfd,(struct sockaddr*)&peer_tcp,alen) < 0)
		printf("connect error\n");

	// Do the write loop
	if(write_to_tcp_socket(sockfd,buf,len,&wrote) <= 0) 
		printf("failure, wrote only %u / %u bytes\n",wrote,len);
	else if(wrote == len) 
		printf("wrote all data: %u bytes \n",wrote);
	
	wrote = 0;

	printf("Sending data to UDP port %d\n",ntohs(peer_udp.sin_port));
	// Do the write loop
        if(write_to_udp_socket(sockfdu,buf,len,&wrote,peer_udp,alen) <= 0)
                printf("failure, wrote only %u / %u bytes\n",wrote,len);
        else if(wrote == len)
                printf("wrote all data: %u bytes \n",wrote);

	close(sockfd);
	close(sockfdu);

	return 0;
}

int write_to_udp_socket(int sockfd, char* data, unsigned int data_len, 
	unsigned int *data_written, struct sockaddr_in receiver, socklen_t addrlen) 
{
	unsigned int buffer_size = 0;
	unsigned int blen = sizeof(buffer_size);
	int wrote = 0;

        if(getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,&buffer_size,&blen)) 
        	perror("Cannot get UDP buffer length\n");
        else if (buffer_size > 0x10000)
	{
		printf("UDP buffer size = %u bytes is too big, reducing", buffer_size);
#ifdef __BUFTENTH
		buffer_size = 0x10000 / 10;
#else
		// just to be safe, reduce by 60 bytes, with IPv6 48 (40+8) but IP options
		// can increase the size but still with this value, fragmentation happens
		buffer_size = 0x10000 - 60; 
#endif
		printf(" to %u\n",buffer_size);

	}
	else printf("UDP buffer size = %d bytes\n", buffer_size);

	uint32_t blocks = ((uint32_t)data_len / buffer_size);
	if(blocks*buffer_size < data_len) blocks++;

	// Size of block + amount of blocks
	int initialsize = sizeof(uint32_t)*2;
	char initial[initialsize];
	memset(&initial,0,initialsize);
	*(uint32_t*)&initial[0] = htonl(buffer_size);
	*(uint32_t*)&initial[sizeof(uint32_t)] = htonl(blocks);

	if((wrote = sendto(sockfd,&initial,initialsize,0,
		(struct sockaddr*)&receiver,addrlen)) != initialsize)
	{
		perror("Sending initial packet failed");
		return -1;
	}

	uint32_t current_block = 0;
	int to_write = 0;

	while(current_block < blocks)
	{
		// If writing less than buffer size = last packet
		to_write = data_len - *data_written < buffer_size ?
			data_len - *data_written : buffer_size;
		if((wrote = sendto(sockfd, &data[*data_written], to_write, 0,
					(struct sockaddr*)&receiver,addrlen)) < 0)
		{
			perror("writing to socket failed");
			return -1;
		}
		if(wrote == 0) return 0;
		if(wrote != buffer_size) 
			printf("p%d: Wrote less than buffer size, buffer:%d, wrote:%d\n",
				current_block+1,buffer_size,wrote);

		*data_written += wrote; // Increase written amount
		printf("p%d: %u\tbytes written (+%d)\n",
			current_block+1,*data_written,wrote);
		current_block++;
	}
	return 1;

}

int write_to_tcp_socket(int sockfd, char* data, unsigned int data_len, 
	unsigned int *data_written)
{
	int wrote = 0;
	
	// Initialize the pointer value to 0
	*data_written = 0;

	int buffer_size = 0;
	unsigned int blen = sizeof(buffer_size);
	
	unsigned int data_to_write = 0;
	unsigned int flags = 0;

	if(getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,&buffer_size,&blen)) 
		perror("Cannot get TCP buffer length\n");
	else printf("TCP buffer size = %d bytes\n", buffer_size);

	// While we have something to write
	while(*data_written < data_len)
	{
		// Write remaining data with do not wait flag - fill the send buffer and continue
#ifdef __ALL_ONCE_DONTWAIT
		flags = MSG_DONTWAIT;
		data_to_write = data_len - *data_written;
		//if((wrote = send(sockfd,&data[*data_written],
		//	data_len - *data_written,MSG_DONTWAIT)) < 0)
#elif __ALL_ONCE
		data_to_write = data_len - *data_written;
		//if((wrote = send(sockfd,&data[*data_written],
		//	data_len - *data_written,0)) < 0)
#elif __BUFTENTH
		data_to_write = buffer_size/10 > data_len - *data_written ? 
			data_len - *data_written : buffer_size/10;
		//if((wrote = send(sockfd,&data[*data_written],buffer_size/10 > 
		//	data_len - *data_written ? data_len - *data_written : buffer_size/10,0)) < 0)
#else
		data_to_write = buffer_size > data_len - *data_written ? 
			data_len - *data_written : buffer_size/10;
		//if((wrote = send(sockfd,&data[*data_written],buffer_size > 
		//	data_len - *data_written ? data_len - *data_written : buffer_size ,0)) < 0)
#endif
		if((wrote = send(sockfd,&data[*data_written],data_to_write,flags)) < 0)
		{
			perror("writing to socket failed");
			return -1;
		}
 
		if(wrote == 0) return 0;
	  
		// Update the written amount
	  	*data_written += wrote;
	  	printf("%u\tbytes of %u written (+%d)\n",*data_written,data_len,wrote);
	}
	
	return 1;
}

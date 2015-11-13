#include "sendtest.h"

int write_to_tcp_socket(int,char*,unsigned int, unsigned int*);
int write_to_udp_socket(int,char*,unsigned int, unsigned int*,
	struct sockaddr_in,socklen_t);

int main(int argc, char* argv[])
{
	if(argc != 3) return -1;
	
	unsigned int len = BLEN, wrote = 0;
	char buf[len];
	
	// Initialize the packet
	memset(&buf,0,len);
	*(uint32_t*)&buf[0] = htonl(len); // Size at the front
	memcpy(&buf[sizeof(uint32_t)],"<START>",7); // Start tag
	for(int i = sizeof(uint32_t)+7; i < len-5; i++) 
		buf[i] = 65+(i%20); // data
	memcpy(&buf[len-5],"<END>",5); // end tag
	
	int sockfd = socket(AF_INET,SOCK_STREAM,0); 
	int sockfdu = socket(AF_INET,SOCK_DGRAM,0);
	
	if(sockfd < 0) perror("sock");
	if(sockfdu < 0) perror("sock");

	// Set the server info, no checks in this demo
	// argv[1] = addr, argv[2] = port
	struct sockaddr_in peer;
	memset(&peer,0,sizeof(peer));
	peer.sin_family = AF_INET;

	if(inet_pton(AF_INET,argv[1],&(peer.sin_addr)) <= 0)
		perror("pton");

	peer.sin_port = htons(atoi(argv[2]));
	socklen_t alen = sizeof(struct sockaddr_in);
	
	if(connect(sockfd,(struct sockaddr*)&peer,alen) < 0)
		printf("connect error\n");

	// Do the write loop
	if(write_to_tcp_socket(sockfd,buf,len,&wrote) <= 0) 
		printf("failure, wrote only %u / %u bytes\n",wrote,len);
	else if(wrote == len) 
		printf("wrote all data: %u bytes \n",wrote);
	
	sleep(2);
	close(sockfd);
	wrote = 0;
	
	peer.sin_port = htons(atoi(argv[2])+1);
	printf("Sending data to UDP port %d\n",ntohs(peer.sin_port));
	// Do the write loop
        if(write_to_udp_socket(sockfdu,buf,len,&wrote,peer,alen) <= 0)
                printf("failure, wrote only %u / %u bytes\n",wrote,len);
        else if(wrote == len)
                printf("wrote all data: %u bytes \n",wrote);
	close(sockfdu);

	return 0;
}

int write_to_udp_socket(int sockfd, char* data, unsigned int data_len, unsigned int *data_written, 
	struct sockaddr_in receiver, socklen_t addrlen) 
{
	unsigned int buffer_size = 0;
	unsigned int blen = sizeof(buffer_size);
	int wrote = 0;

        if(getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,&buffer_size,&blen)) perror("Cannot get UDP buffer length\n");
        else if (buffer_size > 0x10000)
	{
		printf("UDP buffer size = %u bytes is too big, reducing", buffer_size);
		buffer_size = 0x10000 / 10;
		printf(" to %u\n",buffer_size);

	}
	else printf("UDP buffer size = %d bytes\n", buffer_size);

	uint32_t blocks = data_len / buffer_size + 1;

	// Size of block + amount of blocks
	int initialsize = sizeof(uint32_t)*2;
	char initial[initialsize];
	memset(&initial,0,initialsize);
	*(uint32_t*)&initial[0] = htonl(buffer_size);
	*(uint32_t*)&initial[sizeof(uint32_t)] = htonl(blocks);

	if((wrote = sendto(sockfd,&initial,initialsize,0,(struct sockaddr*)&receiver,addrlen)) != initialsize)
	{
		perror("Sending initial packet failed");
		return -1;
	}

	uint32_t current_block;
	//uint32_t current = 0;
	//while(*data_written < data_len)
	while(current_block < blocks)
	{
		if((wrote = sendto(sockfd,&data[*data_written],data_len - *data_written < buffer_size ? data_len - *data_written : buffer_size,0,(struct sockaddr*)&receiver,addrlen)) < 0)
		{
			perror("writing to socket failed");
			return -1;
		}
		if(wrote == 0) return 0;
		if(wrote != buffer_size) printf("Wrote less than buffer size, buffer:%d, wrote:%d\n",buffer_size,wrote);
		*data_written += wrote;
		printf("%u\tbytes written (+%d)\n",*data_written,wrote);
		current_block++;
	}
	printf("wrote?\n");
	return 1;

}

int write_to_tcp_socket(int sockfd, char* data, unsigned int data_len, unsigned int *data_written)
{
	int wrote = 0;
	
	// Initialize the pointer value to 0
	*data_written = 0;

	int buffer_size = 0;
	unsigned int blen = sizeof(buffer_size);

	if(getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,&buffer_size,&blen)) perror("Cannot get TCP buffer length\n");
	else printf("TCP buffer size = %d bytes\n", buffer_size);

	// While we have something to write
	while(*data_written < data_len)
	{
		// Write remaining data with do not wait flag - fill the send buffer and continue
		//if((wrote = send(sockfd,&data[*data_written],data_len - *data_written,MSG_DONTWAIT)) < 0)
		if((wrote = send(sockfd,&data[*data_written],buffer_size/10,0)) < 0)
		{
			perror("writing to socket failed");
			return -1;
		}
 
		if(wrote == 0) return 0;
	  
		// Update the written amount
	  	*data_written += wrote;
	  	printf("%u\tbytes written (+%d)\n",*data_written,wrote);
	}
	
	return 1;
}

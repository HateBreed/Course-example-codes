/*
* CT30A5000 Network Programming
* udpexample, UDP server and client example
*
* Contains simple UDP server that waits for a char from some client and
* quits after it has been received.
*
* Contains simple UDP client that connects to given IP address and sends
* characters then quits.
* 
* Author:
*   Jussi Laakkonen
*   1234567
*   jussi.laakkonen@lut.fi
*/

#include "udpexample.h"
#define SIZE 1

int main(int argc, char *argv[])
{
  /* server */
  if (argc == 2)
  {
    if (server(argv[1]) == 0) printf("Server: exited with success\n");
    else
    {
      printf("Errors with server\n");
      return -1;
    }
  }
  /* client */
  else if (argc == 3)
  {
    if (client(argv[1],argv[2]) == 0) printf("Client: exited with success\n");
    else
    {
      printf("Errors with client\n");
      return -1;
    }
  }
  /* error */
  else
  {
    printf("Invalid amount of arguments.\nUsage:\n\
      server: %s <portnumber>\n\
      client: %s <portnumber> <server ip>\n",argv[0],argv[0]);
    return -1;
  }
  return 0;
}

int server(char* port) {
  int socketfd = -1;
  
  struct addrinfo hints = { .ai_flags = AI_PASSIVE,	/* Get addresses suitable for bind */
                            .ai_family = PF_UNSPEC,				
                            .ai_socktype = SOCK_DGRAM,	/* Datagram socket - UDP */
                            .ai_protocol = IPPROTO_UDP};/* UDP protocol */
  
  struct addrinfo *result = NULL, *iter = NULL;
  struct sockaddr_storage client_addr;
	
  char hostbuffer[NI_MAXHOST] = { 0 };
  char portbuffer[NI_MAXSERV] = { 0 };
  char recvbuffer[SIZE] = { 0 };
  
  socklen_t addrlen = 0;
  unsigned int optval = 0;
  int dgramlen = 0;
  
  printf("| - - - - - S E R V E R - - - - - |\n");

  if (atoi(port) >= 1024 && atoi(port) <= 65000) {
    // Get my (AI_PASSIVE) addresses that are suitable for bind
    if(getaddrinfo(NULL,port,&hints,&result)) {
      perror("cannot get addresses for server");
      return -1;
    }
		
    // Go through all addresses returned with specified hints and use the first one
    for(iter = result; iter != NULL; iter = iter->ai_next) {
      if ((socketfd = socket(iter->ai_family,iter->ai_socktype,iter->ai_protocol)) < 0) {
        perror("socket()");
        return -1;
      }
			
      //Try to bind to this address
      if (bind(socketfd,iter->ai_addr, iter->ai_addrlen) < 0 ) {
        close(socketfd); /* Even when bind fails, socket remains, close it */
        perror("bind()");
        return -1;
      }
      break;
    }

    freeaddrinfo(result);
    
    /* Try to get the maximum length for read buffer */
    socklen_t optlen = sizeof(optval);
    if(getsockopt(socketfd,SOL_SOCKET,SO_RCVBUF,&optval,&optlen) == -1) perror("Cannot get read buffer size getsockopt()");
    else printf("Server: Read buffer in bytes: %u\n",optval);
   
    memset(&recvbuffer,0,SIZE);
    printf("Server: Waiting for datagram..\n");

    addrlen = sizeof(client_addr);
    struct sockaddr* client_address = (struct sockaddr*) &client_addr;
		
    /* Try to receive something (expecting a char - length = 1 byte).. */
    dgramlen = recvfrom(socketfd,&recvbuffer,SIZE,0,client_address,&addrlen);

    /* TASK: Get the sender address and port and print it*/
    memset(&hostbuffer,0,NI_MAXHOST);
    memset(&portbuffer,0,NI_MAXSERV);
		
    printf("Server: Got %d bytes from %s:%s\n",dgramlen,hostbuffer,portbuffer);

/* Question:
* Since we know the address and port client is using, it should be easy to send something
* back. Think about the operation and reverse it without initializing new address.
* What can be changed in sendto()-function parameters if message is going to be sent to
* someone else?
*/

    close(socketfd); /* REMEMBER ME! */   
  }
  else {
    printf("Server: Invalid port. Choose something between 1 - 65000\n");
    return -1;
  }

  return 0;
}

int client(char* port, char *serverip)
{
  int socketfd = -1, length = 0, rval = 0;
  char dgram[SIZE];
  
  struct addrinfo hints = { .ai_flags = AI_NUMERICHOST|AI_NUMERICSERV,
                            .ai_family = PF_UNSPEC,
                            .ai_socktype = SOCK_DGRAM,
                            .ai_protocol = IPPROTO_UDP};
														
  struct addrinfo *result = NULL, *iter = NULL;

  memset(&dgram,1,SIZE);
  
  printf("| - - - - - C L I E N T - - - - - |\n");

  /* Why only IP address works ? */
  if(getaddrinfo(serverip,port,&hints,&result) < 0) perror("Cannot resolve address");
  else {
    // Go through every returned address and attempt to connect to each
    for (iter = result; iter != NULL; iter = iter->ai_next) {
		
      /* Can socket be created? */
      if ((socketfd = socket(iter->ai_family, iter->ai_socktype,iter->ai_protocol)) < 0) {
        perror("socket()");
        rval = -1;
        break;
      }
			
      /* Try to send data to server:
      * sendto(socket, data , data length, flags, destination, struct length)
      * see 'man sendto'
      */
      if((length = sendto(socketfd,&dgram,SIZE,0,iter->ai_addr,iter->ai_addrlen)) < 0) {
        perror("sendto()");
        rval = -1;
        break;
      }
      else printf("Client: Sent datagram length = %d\n", length);

    }
  }
	
  freeaddrinfo(result);

  close(socketfd); /* REMEMBER ME! */

  return rval;
}


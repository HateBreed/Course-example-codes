/*
* CT30A5000 Network Programming
* udpexample, UDP server and client example
*
* Contains simple UDP server that waits for a char from some client and
* quits after it has been received.
*
* Contains simple UDP client that connects to given IP address and sends
* a char then quits.
* 
* Author:
*   Jussi Laakkonen
*   1234567
*   jussi.laakkonen@lut.fi
*/

#ifndef __HELLOUDP_H
#define __HELLOUDP_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* Main function, accepts list of arguments*/
int main(int, char*[]);

/* Server function
Parameter:
  char* - port number to use.
Returns:
  int - 0 if success, <0 if errors
*/
int server(char*);


/* Client function
Parameters:
  char* - port number used by server
  char* - IP address of the server
Returns:
  int - 0 if success, <0 if errors
*/
int client(char*,char*);

#endif

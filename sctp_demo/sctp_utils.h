/* 
* CT30A5001 Network Programming
* sctp_utils.h, STCP server and client example
*
* Contains headers for both client and server to use. Also the prototypes for
* utility functions are defined here. Note the defined abbreviation for struct 
* sockaddr.
* 
* Author:
*   Jussi Laakkonen
*   1234567
*   jussi.laakkonen@lut.fi
*/

#ifndef __SCTP_UTILS_H_
#define __SCTP_UTILS_H_

#define SA  struct sockaddr

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <signal.h>
#include <sys/select.h>

unsigned int print_packed_addresses(int,struct sockaddr_storage*);
void check_sctp_event(char*,int);
void set_sctp_events(struct sctp_event_subscribe*);

#endif

/* 
* CT30A5001 Network Programming
* addresses.h, STCP server and client example
*
* Contains headers for the addresses.c functions.
* 
* Author:
*   Jussi Laakkonen
*   1234567
*   jussi.laakkonen@lut.fi
*/

#ifndef __SCTP_ADDRESSES_H_
#define __SCTP_ADDRESSES_H_

#include "sctp_utils.h"

struct sockaddr* get_own_addresses_gai(int*, char*);
struct sockaddr* get_own_addresses_gia(int*, int, int, int);
struct sockaddr* get_own_addresses_combined(int*, char*, int, int);

int error_situation();
void print_host(int,int,char*,char*);

#endif

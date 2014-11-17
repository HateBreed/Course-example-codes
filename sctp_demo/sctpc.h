/* 
* CT30A5001 Network Programming
* sctpc.h, STCP server and client example
*
* Contains header inclusions and function prototypes for SCTP client example.
* 
* Author:
*   Jussi Laakkonen
*   1234567
*   jussi.laakkonen@lut.fi
*/

#ifndef __SCTP_CLIENT_H_
#define __SCTP_CLIENT_H_

#include <time.h>

#include "sctp_utils.h"

int run_test_client(int argc,char* argv[]);
int testclient_input_error(char*);
char* testclient_fill_random_data(int);
void print_addr_type(char*);
int check_addr_type(char*,int);

#endif

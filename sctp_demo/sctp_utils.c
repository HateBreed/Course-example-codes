/* 
* CT30A5001 Network Programming
* sctp_utils.c, STCP server and client example
*
* Contains utility functions for SCTP example.
* 
* Author:
*   Jussi Laakkonen
*   1234567
*   jussi.laakkonen@lut.fi
*/

#include "sctp_utils.h"

/**
 * print_packed_addresses:
 * @count: Amount of addresses in the packedlist
 * @addrs: Packed list of addresses
 *
 * Print the @count number of addresses from the packed list: @addrs
 *
 * Returns: The actual combined length of addresses in bytes
 */
unsigned int print_packed_addresses(int count, struct sockaddr_storage* addrs)
{
	int i = 0, slen = 0;
	unsigned int storage_len = 0;
	char hostname[NI_MAXHOST] = {0};
	
	// Take the pointer to the first one in the array
	struct sockaddr_storage* temp = addrs;

	if(!addrs) return 0;
	
	while(i < count)
	{
		// Get the length of the structure by checking the family, 0 if unknown
		slen = (temp->ss_family == AF_INET ?
			sizeof(struct sockaddr_in) : 
			temp->ss_family == AF_INET6 ? 
			sizeof(struct sockaddr_in6) : 
			0);
		
		if (slen > 0) {
			memset(&hostname,0,NI_MAXHOST);
			// Get the numeric representation of hostname into buffer
			if (getnameinfo((struct sockaddr*)temp,
				slen,
				hostname,
				NI_MAXHOST,
				NULL,
				0,
				NI_NUMERICHOST) < 0)
				perror("getnameinfo");
			else printf("\t[%s] %s\n", 
				(temp->ss_family == PF_INET ? "IPv4" : "IPv6"),
				hostname);
		
			storage_len += slen; // Add the size of current struct
			temp = (struct sockaddr_storage*)((char*)temp + slen); // Move pointer to next struct
		}
		i++;
	}
	
	return storage_len;
}

/**
 * check_sctp_event:
 * @data: data buffer containing the event
 * @datalen: length of the data buffer
 *
 * Check the event in the given @data buffer. Does not react in any way and
 * only prints out the event and related data. 
 *
 * Handles following events:
 * SCTP_SHUTDOWN_EVENT
 * SCTP_ASSOC_CHANGE
 * SCTP_REMOTE_ERROR
 * SCTP_SENT_FAILED
 * SCTP_PEER_ADDR_CHANGE
 *
 * Returns: void
 */
void check_sctp_event(char* data, int datalen)
{
	if(datalen < 8) return; // At least the common header for events must exist
	if(!data) return;
	
	uint16_t sctp_event_type = *(uint16_t*)&data[0];
	switch(sctp_event_type)
	{
		case SCTP_SHUTDOWN_EVENT:
			printf("shutdown event\n\t");
			struct sctp_shutdown_event sd;
			memset(&sd,0,sizeof(sd));
			memcpy(&sd,data,sizeof(sd));
			printf("association\t%u\n",sd.sse_assoc_id);
			break;
			
		case SCTP_ASSOC_CHANGE:
			printf("association event\n\t");
			struct sctp_assoc_change ac;
			memset(&ac,0,sizeof(ac));
			memcpy(&ac,data,sizeof(ac));
			unsigned int infolength = ac.sac_length - sizeof(u_int16_t)*6 - sizeof(u_int32_t) - sizeof(sctp_assoc_t);
			printf("association:\t%u\n\ti/o streams:\t%d/%d\n",
				ac.sac_assoc_id,
				ac.sac_inbound_streams,
				ac.sac_outbound_streams);

			printf("\tconn state:\t");
			switch(ac.sac_state)
			{
				case SCTP_COMM_UP:
					printf("comm up");
					break;
				case SCTP_COMM_LOST:
					printf("comm lost");
					break;
				case SCTP_RESTART:
					printf("restart");
					break;
				case SCTP_SHUTDOWN_COMP:
					printf("shutdown");
					break;
				case SCTP_CANT_STR_ASSOC:
					printf("setup fail");
					break;
				default:
					break;
			}
			printf("\n\tsac_info[%u]:\t",infolength);
			for(unsigned int i = 0; i < infolength; i++) printf("%c",ac.sac_info[i]);
			printf("\n");
			break;
			
		case SCTP_REMOTE_ERROR:
			printf("peer error event\n\t:");
			struct sctp_remote_error re;
			memset(&re,0,sizeof(re));
			memcpy(&re,data,sizeof(re));
			printf("error id:\t%d\n\terror:\t",re.sre_error);
			unsigned int errlength = re.sre_length - sizeof(u_int16_t)*3 - sizeof(u_int32_t) - sizeof(sctp_assoc_t);
			for(unsigned int i = 0; i < errlength ; i++) printf("%c",re.sre_data[i]);
			printf("\n");
			break;
			
		case SCTP_SEND_FAILED:
			printf("sending failed event\n");
			struct sctp_send_failed sf;
			memset(&sf,0,sizeof(sf));
			memcpy(&sf,data,sizeof(sf));
			if(sf.ssf_flags & SCTP_DATA_UNSENT) printf("\tdata cannot be transmitted\n");
			if(sf.ssf_flags & SCTP_DATA_SENT) printf("\tdata was sent but not acknowledged\n");
			printf("\tassociation id:\t%d\n\terror code:\t%u\n\tstream id:\t%d\n",sf.ssf_assoc_id,sf.ssf_error, sf.ssf_info.sinfo_stream);
			// The sf.ssf_data[] would contain the undelivered message, not printed in the demo
			// the sf.ssf_info would contain the information about connection to association incl. stream number
			break;
		case SCTP_PEER_ADDR_CHANGE:
			printf("changes in peer address event\n");
			struct sctp_paddr_change pc;
			memset(&pc,0,sizeof(pc));
			memcpy(&pc,data,sizeof(pc));
			printf("\taddress ");
			switch(pc.spc_state)
			{
				case SCTP_ADDR_ADDED:
					printf("added ");
					break;
				case SCTP_ADDR_AVAILABLE:
					printf("available ");
					break;
				case SCTP_ADDR_CONFIRMED:
					printf("confirmed ");
					break;
				case SCTP_ADDR_MADE_PRIM:
					printf("primary ");
					break;
				case SCTP_ADDR_REMOVED:
					printf("removed ");
					break;
				case SCTP_ADDR_UNREACHABLE:
					printf("unreachable ");
					break;
				default:
					break;
			}
			printf("\n");
			print_packed_addresses(1,&pc.spc_aaddr); // structure contains only one address
		default:
			break;
	}
}

void set_sctp_events(struct sctp_event_subscribe *sctp_events)
{
	sctp_events->sctp_data_io_event = 1; 		// Allow to get sctp_sndrcvinfo in sctp_recvmsg
	sctp_events->sctp_shutdown_event = 1;		// Get shutdown event of connection: SCTP_SHUTDOWN_EVENT
	sctp_events->sctp_peer_error_event = 1;		// Get peer error event: SCTP_REMOTE_ERROR
	sctp_events->sctp_send_failure_event = 1;	// Get transmission failure event: SCTP_SEND_FAILED
	sctp_events->sctp_association_event = 1;	// Get association related events: SCTP_ASSOC_CHANGE
	sctp_events->sctp_address_event = 1;		// Get address change events: SCTP_PEER_ADDR_CHANGE
}


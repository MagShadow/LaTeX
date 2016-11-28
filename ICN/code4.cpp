/*
* THIS FILE IS FOR TCP TEST
*/

/*
struct sockaddr_in {
        short   sin_family;
        u_short sin_port;
        struct  in_addr sin_addr;
        char    sin_zero[8];
};
*/

#include "sysInclude.h"

extern void tcp_DiscardPkt(char *pBuffer, int type);

extern void tcp_sendReport(int type);

extern void tcp_sendIpPkt(unsigned char *pData, UINT16 len, unsigned int  srcAddr, unsigned int dstAddr, UINT8	ttl);

extern int waitIpPacket(char *pBuffer, int timeout);

extern unsigned int getIpv4Address();

extern unsigned int getServerIpv4Address();

int gSrcPort = 2005;
int gDstPort = 2006;
int gSeqNum = 0;
int gAckNum = 0;

#define SYN_SENT 0
#define ESTABLISHED 1
#define FIN_WAIT_1 2
#define FIN_WAIT_2 3
#define TIME_WAIT 4
#define CLOSED 5

struct TCB
{
	unsigned dstAddr, srcAddr;
	unsigned short srcPort, dstPort;
	unsigned char state;
	unsigned seq, ack;
	TCB *next;
	
	TCB(unsigned n_dstAddr, unsigned n_srcAddr, unsigned short n_srcPort, unsigned short n_dstPort,
		unsigned char n_state, unsigned n_seq, unsigned n_ack):
		dstAddr(n_dstAddr), srcAddr(n_srcAddr), srcPort(n_srcPort), dstPort(n_dstPort), 
		state(n_state), seq(n_seq), ack(n_ack), next(NULL) {}
};

TCB *tcb = NULL;

TCB *findTCB(unsigned n_dstAddr, unsigned n_srcAddr, unsigned short n_srcPort, unsigned short n_dstPort)
{
	TCB *curr = tcb;
	while(curr)
	{
		if(curr->dstAddr==n_dstAddr && curr->srcAddr==n_srcAddr 
		&& curr->srcPort==n_srcPort && curr->dstPort==n_dstPort)
			return curr;
		curr = curr->next;
	}
	return NULL;
}

int stud_tcp_input(char *pBuffer, unsigned short len, unsigned int srcAddr, unsigned int dstAddr)
{
	unsigned char *p = (unsigned char *)pBuffer;
	
	// checkSum begin

	unsigned char *b = new unsigned char[12 + len];
	memset(b, 0, 12 + len);
	memcpy(b + 12, p, len);
	*(unsigned *)b = srcAddr;
	*(unsigned *)(b + 4) = dstAddr;
	b[9] = 6;
	*(unsigned short *)(b + 10) = htons(len);
	unsigned sum = 0;
	for(unsigned i = 0; i < (len + 12)/2; ++i)
		sum += ((unsigned short *)b)[i];
	if(len%2 == 1)
		sum += 256 * b[len + 11];
	delete b;
	while(sum >> 16)
		sum = (sum & 0x0000ffff) + (sum >> 16);
	if(sum != 0x0000ffff)
		return -1;
	// end CheckSum

	unsigned short srcPort = ntohs(*(unsigned short *)p);
	unsigned short dstPort = ntohs(*(unsigned short *)(p+2));
	unsigned seq = ntohl(*(unsigned *)(p + 4));
	unsigned ack = ntohl(*(unsigned *)(p + 8));
	unsigned char flags = p[13];

	TCB *currTCB = findTCB(ntohl(dstAddr), ntohl(srcAddr), srcPort, dstPort);
	if(!currTCB) return -1;

	if(ack != currTCB->seq + 1) {tcp_DiscardPkt(pBuffer, STUD_TCP_TEST_SEQNO_ERROR); }

	if(currTCB->state == SYN_SENT && (flags & 0x10) && (flags & 0x02) )
	{
		currTCB->ack = seq + 1;
		currTCB->state = ESTABLISHED;
		currTCB->seq = ack;
		stud_tcp_output(NULL, 0, PACKET_TYPE_ACK, currTCB->srcPort, currTCB->dstPort,
			currTCB->srcAddr, currTCB->dstAddr);
	}
	else if(currTCB->state == FIN_WAIT_1 && (flags & 0x10))
	{
		currTCB->state = FIN_WAIT_2;
	}
	else if(currTCB->state == FIN_WAIT_2 && (flags & 0x10) && (flags & 0x01) )
	{
		currTCB->ack = seq + 1;
		currTCB->seq = ack;
		currTCB->state = TIME_WAIT;
		stud_tcp_output(NULL, 0, PACKET_TYPE_ACK, currTCB->srcPort, currTCB->dstPort,
			currTCB->srcAddr, currTCB->dstAddr);
		currTCB->state = CLOSED;
	}
	return 0;
}

void stud_tcp_output(char *pData, unsigned short len, unsigned char flag, 
unsigned short srcPort, unsigned short dstPort, unsigned int srcAddr, unsigned int dstAddr)
{
	unsigned length = len + 20;
	unsigned char *packet = new unsigned char[length];
	memset(packet, 0, length);
	memcpy(packet + 20, pData, len);
	
	TCB *currTCB = findTCB(dstAddr, srcAddr, srcPort, dstPort);
	if(!currTCB)
 		return;

	*(unsigned short *)packet = htons(currTCB->srcPort);
	*(unsigned short *)(packet + 2) = htons(currTCB->srcPort);
	*(unsigned *)(packet + 4) = htonl(currTCB->seq);
	*(unsigned *)(packet + 8) = htonl(currTCB->ack);
	packet[12] = 20 << 2;
	if(flag == PACKET_TYPE_DATA)
	{ }
	else if(flag == PACKET_TYPE_SYN)
	{
		packet[13] = 0x02;
		currTCB->state = SYN_SENT;
	}
	else if(flag == PACKET_TYPE_SYN_ACK)
	{
		packet[13] = 0x12;
	}
	else if (flag == PACKET_TYPE_ACK)
	{
		packet[13] = 0x10;
	}
	else if (flag == PACKET_TYPE_FIN)
	{
		packet[13] = 0x01;
	}
	else if (flag == PACKET_TYPE_FIN_ACK )
	{
		packet[13] = 0x11;
		currTCB->state = FIN_WAIT_1;
	}

	*(unsigned short*)(packet + 14) = htons(1024);
	
	// checkSum begin

	unsigned char *b = new unsigned char[12 + length];
	memset(b, 0, 12 + length);
	memcpy(b + 12, packet, len);
	*(unsigned *)b = htonl(currTCB->srcAddr);
	*(unsigned *)(b + 4) = htonl(currTCB->dstAddr);
	b[9] = 6;
	*(unsigned short *)(b + 10) = htons(len);
	unsigned sum = 0;
	for(unsigned i = 0; i < (len + 12)/2; ++i)
		sum += ((unsigned short *)b)[i];
	if(len%2 == 1)
		sum += 256 * b[len + 11];
	delete b;
	while(sum >> 16)
		sum = (sum & 0x0000ffff) + (sum >> 16);
	// end CheckSum

	*(unsigned short *)(packet + 16) = ~sum;

	tcp_sendIpPkt(packet, length, tcb->srcAddr, tcb->dstAddr, 255);
}

int stud_tcp_socket(int domain, int type, int protocol)
{
	return 2;
}

int stud_tcp_connect(int sockfd, struct sockaddr_in *addr, int addrlen)
{
	
	return 0;
}

int stud_tcp_send(int sockfd, const unsigned char *pData, unsigned short datalen, int flags)
{
	return 0;
}

int stud_tcp_recv(int sockfd, unsigned char *pData, unsigned short datalen, int flags)
{
	return 0;
}

int stud_tcp_close(int sockfd)
{
	return 0;
}

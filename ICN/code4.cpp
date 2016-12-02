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

extern void tcp_sendIpPkt(unsigned char *pData, UINT16 len, unsigned int  srcAddr, unsigned int dstAddr, UINT8 ttl);

extern int waitIpPacket(char *pBuffer, int timeout);

extern unsigned int getIpv4Address();

extern unsigned int getServerIpv4Address();

int gSrcPort = 2005;
int gDstPort = 2006;
int gSeqNum = 1;
int gAckNum = 0;
int socketSeq = 1;

#define SYN_SENT 0
#define ESTABLISHED 1
#define FIN_WAIT_1 2
#define FIN_WAIT_2 3
#define TIME_WAIT 4
#define CLOSED 5
#define TTL 64

struct TCB
{
	unsigned dstAddr, srcAddr;
	unsigned short srcPort, dstPort;
	unsigned char state;
	unsigned seq, ack;
	int socket;
	TCB *next;
	
	TCB(unsigned n_dstAddr, unsigned n_srcAddr, unsigned short n_srcPort, unsigned short n_dstPort,
		unsigned char n_state, unsigned n_seq, unsigned n_ack, unsigned char n_socket = 0):
		dstAddr(n_dstAddr), srcAddr(n_srcAddr), srcPort(n_srcPort), dstPort(n_dstPort), 
		state(n_state), seq(n_seq), ack(n_ack), next(NULL), socket(n_socket) { }

	TCB(unsigned char n_socket):socket(n_socket), next(NULL) {}
};

TCB *tcb = NULL;

TCB *Search(unsigned n_dstAddr, unsigned n_srcAddr, unsigned short n_srcPort, unsigned short n_dstPort)
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

TCB *Search(int n_socket)
{
	TCB *curr = tcb;
	while(curr)
	{
		if(curr->socket == n_socket)
			return curr;
		curr = curr->next;
	}
	return NULL;
}

void deleteTCB(TCB *del)
{
	if(!tcb) return;
	if(tcb == del)
	{
		TCB *tmp = tcb;
		tcb = tcb->next;
		delete tmp;
		return;
	}
	TCB *curr = tcb;
	while(curr && curr->next != del) curr = curr->next;
	curr->next = del->next;
	delete del;
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

	TCB *currTCB = Search(ntohl(srcAddr), ntohl(dstAddr), dstPort, srcPort);
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
	else if(currTCB->state == FIN_WAIT_2 && (flags & 0x01) )
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

	if(!tcb)
	{
		tcb = new TCB(dstAddr, srcAddr, srcPort, dstPort, 
			CLOSED, gSeqNum, gAckNum);
	}

	TCB *currTCB = Search(dstAddr, srcAddr, srcPort, dstPort);
	if(!currTCB)
 		return;

	*(unsigned short *)packet = htons(currTCB->srcPort);
	*(unsigned short *)(packet + 2) = htons(currTCB->dstPort);
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
	memcpy(b + 12, packet, length);
	*(unsigned *)b = htonl(currTCB->srcAddr);
	*(unsigned *)(b + 4) = htonl(currTCB->dstAddr);
	b[9] = 6;
	*(unsigned short *)(b + 10) = htons(length);
	unsigned sum = 0;
	for(unsigned i = 0; 2 * i < length + 12; ++i)
		sum += *(unsigned short *)(b + 2*i);
	if(len%2 == 1)
		sum += 256 * b[length + 11];
	delete b;
	while(sum >> 16)
		sum = (sum & 0x0000ffff) + (sum >> 16);
	// end CheckSum

	*(unsigned short *)(packet + 16) = ~((unsigned short)sum);

	tcp_sendIpPkt(packet, length, currTCB->srcAddr, currTCB->dstAddr, TTL);
}

bool b = true;

int stud_tcp_socket(int domain, int type, int protocol)
{
	if(b) { b = false; deleteTCB(tcb); }
	TCB *curr = tcb;
	if(curr)
	{
		while(curr->next)
			curr = curr->next;
		curr->next = new TCB(socketSeq);
	}
	else
	{
		tcb = new TCB(socketSeq);
	}
	++socketSeq;
	return socketSeq - 1;
}

int stud_tcp_connect(int sockfd, struct sockaddr_in *addr, int addrlen)
{

	TCB *currTCB = Search(sockfd);
	if(!currTCB)
		return -1;
	currTCB->srcAddr = getIpv4Address();
	currTCB->srcPort = gSrcPort;
	currTCB->dstAddr = ntohl(addr->sin_addr.s_addr);
	currTCB->dstPort = ntohs(addr->sin_port);
	
	stud_tcp_output(NULL, 0, PACKET_TYPE_SYN, tcb->srcPort, currTCB->dstPort, currTCB->srcAddr, currTCB->dstAddr);
	
	unsigned char *packet = new unsigned char[1024];
	if(waitIpPacket((char *)packet, 10) == -1) return -1;
	
	currTCB->seq = ntohl(*((unsigned *)(packet + 8)));
	currTCB->ack = ntohl(*((unsigned *)(packet + 4))) + 1; 
	currTCB->state = ESTABLISHED;	

	delete []packet;

	stud_tcp_output(NULL, 0, PACKET_TYPE_ACK, currTCB->srcPort, currTCB->dstPort, currTCB->srcAddr, currTCB->dstAddr);

	return 0;
}

int stud_tcp_send(int sockfd, const unsigned char *pData, unsigned short datalen, int flags)
{
	TCB *currTCB = Search(sockfd);
	if(!currTCB) return -1;

	stud_tcp_output((char *)pData, datalen, PACKET_TYPE_DATA, currTCB->srcPort, currTCB->dstPort, currTCB->srcAddr, currTCB->dstAddr);
	
	unsigned char *packet = new unsigned char[1024];
	if(waitIpPacket((char *)packet, 10) == -1) return -1;

	currTCB->seq = ntohl(*((unsigned *)(packet + 8)));
	currTCB->ack = ntohl(*((unsigned *)(packet + 4))) + 1; 

	delete []packet;

	return 0;
}

int stud_tcp_recv(int sockfd, unsigned char *pData, unsigned short datalen, int flags)
{
	TCB *currTCB = Search(sockfd);
	if(!currTCB) return -1;

	unsigned char *packet = new unsigned char[10000];
	int length = waitIpPacket((char *)packet, 10);
	if(length == -1)	return -1;

	int IHL = packet[12] >> 4;
	IHL *= 4;

	currTCB->seq = ntohl(*((unsigned *)(packet + 8)));
	currTCB->ack = ntohl(*((unsigned *)(packet + 4))) + (length - IHL); 	

	memcpy(pData, packet + IHL, length - IHL); 

	delete []packet;

	stud_tcp_output(NULL, 0, PACKET_TYPE_ACK, currTCB->srcPort, currTCB->dstPort, currTCB->srcAddr, currTCB->dstAddr); 

	return 0;
}

int stud_tcp_close(int sockfd)
{
	TCB *currTCB = Search(sockfd);
	if(!currTCB) return -1;
	if(currTCB->state == SYN_SENT)
	{
		delete(currTCB);
		return -1;
	}

	stud_tcp_output(NULL, 0, PACKET_TYPE_FIN_ACK, currTCB->srcPort, currTCB->dstPort, currTCB->srcAddr, currTCB->dstAddr);
	
	unsigned char *packet = new unsigned char[2000];

	if(waitIpPacket((char *)packet, 10) == -1) return -1;

	if(!(packet[13] & 0x10)) return -1;

	currTCB->state = FIN_WAIT_2;
	currTCB->seq = ntohl(*((unsigned *)(packet + 8)));
	currTCB->ack = ntohl(*((unsigned *)(packet + 4))) + 1;

	if(waitIpPacket((char *)packet, 10) == -1) return -1;

	if(!(packet[13] & 0x01)) return -1;

	tcb->state = TIME_WAIT;
	currTCB->seq = ntohl(*((unsigned *)(packet + 8)));
	currTCB->ack = ntohl(*((unsigned *)(packet + 4))) + 1;

	stud_tcp_output(NULL,  0,  PACKET_TYPE_ACK,  currTCB->srcPort,  currTCB->dstPort, currTCB->srcAddr, currTCB->dstAddr);

	deleteTCB(currTCB);
	delete []packet;

	return 0;
}


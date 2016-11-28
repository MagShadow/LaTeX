/*
* THIS FILE IS FOR IP TEST
*/
// system support
#include "sysInclude.h"

extern void ip_DiscardPkt(char* pBuffer,int type);

extern void ip_SendtoLower(char*pBuffer,int length);

extern void ip_SendtoUp(char *pBuffer,int length);

extern unsigned int getIpv4Address();

// implemented by students

int stud_ip_recv(char *pBuffer,unsigned short length)
{
	unsigned char *PBuffer = (unsigned char *)pBuffer;

	unsigned char version = PBuffer[0] & 0xf0; version >>= 4;
	unsigned char IHL = PBuffer[0] & 0x0f;
	unsigned char ServiceType = PBuffer[1];
	unsigned short len = ntohs(*(unsigned short *)(PBuffer + 2));
	unsigned char TTL = PBuffer[8];
	unsigned Des = ntohl(*(unsigned *)(PBuffer + 16));
	unsigned Sum = 0;
	for(unsigned i = 0; i < length/2; ++i)
	{
		unsigned tmp = PBuffer[2 * i];
		tmp <<= 8;
		tmp |= PBuffer[2 * i + 1];
		Sum += tmp;
	}

	if(length%2 == 1)
		Sum += ((unsigned short)PBuffer[length - 1] << 8);

	while(Sum >> 16)
		Sum = ( Sum & 0xFFFF ) + (Sum >> 16);

	if(version != '\004') {ip_DiscardPkt(pBuffer, STUD_IP_TEST_VERSION_ERROR); return 1;}
	if(TTL == '\000' ) {ip_DiscardPkt(pBuffer, STUD_IP_TEST_TTL_ERROR ); return 1;}
	if(IHL < '\005' ) {ip_DiscardPkt(pBuffer, STUD_IP_TEST_HEADLEN_ERROR ); return 1;}
	if(Des != getIpv4Address()) {ip_DiscardPkt(pBuffer, STUD_IP_TEST_DESTINATION_ERROR ); return 1;}
	if(Sum  != 0xFFFF) {ip_DiscardPkt(pBuffer, STUD_IP_TEST_CHECKSUM_ERROR ); return 1;}

	ip_SendtoUp(pBuffer, length);
	return 0;
}

int stud_ip_Upsend(char *pBuffer,unsigned short len,unsigned int srcAddr,
				   unsigned int dstAddr,byte protocol,byte ttl)
{
	unsigned short Len = len + 20;
	unsigned char *package = new unsigned char[Len];
	for(int i = 0; i < Len; ++i)
		package[i] = 0;
	
	package[0] = 0x45;
	package[3] = Len & 0x00FF;
	package[2] = ((Len & 0xFF00) >> 8);
	package[8] = ttl;
	package[9] = protocol;
	*(unsigned *)(package + 12) = htonl(srcAddr);
	*(unsigned *)(package + 16) = htonl(dstAddr);

	unsigned Sum = 0;
	for(int i = 0; i < 10; ++i)
	{
		unsigned tmp = package[2 * i];
		tmp <<= 8;
		tmp |= package[2 * i + 1];
		Sum += tmp;
	}

	while (Sum >> 16)
	{
		Sum = (Sum & 0xFFFF) + (Sum >> 16);
	}

	Sum = ~Sum;

	package[11] = Sum & 0x00FF;
	package[10] = (Sum & 0xFF00) >> 8;

	for(int i = 20; i < Len; ++i)
		package[i] = pBuffer[i - 20];
 	
	ip_SendtoLower((char *)package ,Len);
	return 0;
}

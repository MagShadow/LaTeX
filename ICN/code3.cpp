/*
* THIS FILE IS FOR IP FORWARD TEST
*/
#include "sysInclude.h"

// system support
extern void fwd_LocalRcv(char *pBuffer, int length);

extern void fwd_SendtoLower(char *pBuffer, int length, unsigned int nexthop);

extern void fwd_DiscardPkt(char *pBuffer, int type);

extern unsigned int getIpv4Address( );

// implemented by students

struct bintree
{
	//data
public:
	unsigned des;
	unsigned masklen;
	unsigned nexthop;
	bintree *m_L;//left
	bintree *m_R;//rgiht
	bool isFir;

	//function
public:

	bintree(): m_L(NULL), m_R(NULL), isFir(true) {}
	bintree(unsigned n_des, unsigned n_masklen, unsigned n_nexthop) :
		des(n_des), masklen(n_masklen), nexthop(n_nexthop), isFir(false) ,
		m_L(NULL), m_R(NULL){}

	~bintree()
	{
		if (m_R != NULL) delete m_R;
		if (m_L != NULL) delete m_L;
	}

	int height() {
		if (m_R == NULL && m_L == NULL)
			return 1;
		else
		{
			int L_H(0), R_H(0);
			if (m_R != NULL) R_H = m_R->height();
			if (m_L != NULL) L_H = m_L->height();
			if (R_H > L_H) 
				return R_H + 1;
			else
				return L_H + 1;
		}
	}

	int diff() {
		int L_H(0), R_H(0);
		if (m_L != NULL)
			L_H = m_L->height();
		if (m_R != NULL)
			R_H = m_R->height();
		return L_H - R_H;
	}

	bool _add(unsigned n_des, unsigned n_masklen, unsigned n_nexthop) {
		if (isFir == true) {
			isFir = false;
			des = n_des; masklen = n_masklen; nexthop = n_nexthop;
			return false;
		}
		bintree* tmp = this;
		while (true) {
			if (tmp->des > n_des) {
				if (tmp->m_L == NULL) {
					tmp->m_L = new bintree(n_des, n_masklen, n_nexthop);
					return true;
				}
				else
					tmp = tmp->m_L;
			}
			else if (tmp->des < n_des) {
				if (tmp->m_R == NULL) {
					tmp->m_R = new bintree(n_des, n_masklen, n_nexthop);
					return true;
				}
				else
					tmp = tmp->m_R;
			}
			else {
				tmp->des = n_des;
				tmp->nexthop = n_nexthop;
				return true;
			}
		}
	}

	void LL() {
		bintree *ptr_a = m_R, *ptr_b = m_L->m_R, *ptr_c = m_L->m_L;
		unsigned A_des = des, A_masklen = masklen, A_nexthop = nexthop;
		des = m_L->des;
		masklen = m_L->masklen;
		nexthop = m_L->nexthop;
		m_L->m_L = NULL;
		m_L->m_R = NULL;
		delete m_L;
		m_R = new bintree(A_des, A_masklen, A_nexthop);
		m_L = ptr_c;
		m_R->m_L = ptr_b;
		m_R->m_R = ptr_a;
	}

	void LR() {
		unsigned A_des = m_L->des, A_masklen = m_L->masklen, A_nexthop = m_L->nexthop;
		unsigned B_des = des, B_masklen = masklen, B_nexthop = nexthop;
		unsigned C_des = m_L->m_R->des, C_masklen = m_L->m_R->masklen, C_nexthop = m_L->m_R->nexthop;
		bintree *ptr_a = m_L->m_L, *ptr_b = m_L->m_R->m_L,
			*ptr_c = m_L->m_R->m_R, *ptr_d = m_R;
		m_L->m_R->m_L = NULL;
		m_L->m_R->m_R = NULL;
		delete m_L->m_R;
		m_L->m_L = NULL;
		m_L->m_R = NULL;
		delete m_L;
		des = C_des;
		masklen = C_masklen;
		nexthop = C_nexthop;
		m_L = new bintree(A_des, A_masklen, A_nexthop);
		m_R = new bintree(B_des, B_masklen, B_nexthop);
		m_L->m_L = ptr_a;
		m_L->m_R = ptr_b;
		m_R->m_L = ptr_c;
		m_R->m_R = ptr_d;
	}

	void RL() {
		unsigned A_des = m_R->des, A_masklen = m_R->masklen, A_nexthop = m_R->nexthop;
		unsigned B_des = des, B_masklen = masklen, B_nexthop = nexthop;
		unsigned C_des = m_R->m_L->des, C_masklen = m_R->m_L->masklen, C_nexthop = m_R->m_L->nexthop;
		bintree *ptr_a = m_R->m_R, *ptr_b = m_R->m_L->m_R,
			*ptr_c = m_R->m_L->m_L, *ptr_d = m_L;
		m_R->m_L->m_R = NULL;
		m_R->m_L->m_L = NULL;
		delete m_R->m_L;
		m_R->m_R = NULL;
		m_R->m_L = NULL;
		delete m_R;
		des = C_des;
		masklen = C_masklen;
		nexthop = C_nexthop;
		m_R = new bintree(A_des, A_masklen, A_nexthop);
		m_L = new bintree(B_des, B_masklen, B_nexthop);
		m_R->m_R = ptr_a;
		m_R->m_L = ptr_b;
		m_L->m_R = ptr_c;
		m_L->m_L = ptr_d;
	}

	void RR() {
		bintree *ptr_a = m_L, *ptr_b = m_R->m_L, *ptr_c = m_R->m_R;
		unsigned A_des = des, A_masklen = masklen, A_nexthop = nexthop;
		des = m_R->des;
		masklen = m_R->masklen;
		nexthop = m_R->nexthop;
		m_R->m_L = NULL;
		m_R->m_R = NULL;
		delete m_R;
		m_L = new bintree(A_des, A_masklen, A_nexthop);
		m_R = ptr_c;
		m_L->m_L = ptr_a;
		m_L->m_R = ptr_b;
	}

	void add(unsigned n_des, unsigned n_masklen, unsigned n_nexthop) {
		if (!_add(n_des, n_masklen, n_nexthop))
			return;
		bintree *tmp = this;
		bintree *min = NULL;
		while (tmp != NULL) {
			if (tmp->diff() < -1 || tmp->diff() > 1)
				min = tmp;
			if (n_des < tmp->des)
				tmp = tmp->m_L;
			else if (n_des > tmp->des)
				tmp = tmp->m_R;
			else
				break;
		}

		if (min == NULL)
			return;

		if (min->diff() == 2) {
			if (min->m_L->diff() > 0)
				min->LL();
			else
				min->LR();
		}
		else {
			if (min->m_R->diff() > 0)
				min->RL();
			else
				min->RR();
		}
	}

	bool getNextHop(unsigned n_des, unsigned &n_next_hop)
	{
		bintree *b = this, *target = NULL;
		while(b)
		{
			unsigned mask = 0xffffffff << (32-b->masklen);
			if((b->des) == (n_des & mask))
				target = b;
			if(b->des < n_des) b = b->m_R;
			else if(b->des > n_des) b = b->m_L;
			else break;
		}
		if(!target) return false;
		n_next_hop = target->nexthop;
		return true;
	}
};

bintree route_tree;

void stud_Route_Init()
{
	return;
}

void stud_route_add(stud_route_msg *proute)
{
	route_tree.add(ntohl(proute->dest) & (0xffffffff << (32-ntohl(proute->masklen))),
		ntohl(proute->masklen), ntohl(proute->nexthop));
}


int stud_fwd_deal(char *pBuffer, int length)
{
	unsigned char *PBuffer = new unsigned char[length];
	unsigned char IHL = PBuffer[0] & 0x0f;
	for(int i = 0; i < length; ++i)
		PBuffer[i] = (unsigned char)pBuffer[i];
	unsigned Des = ntohl(*(unsigned *)(PBuffer + 16));
	if(Des == getIpv4Address())
	{
		fwd_LocalRcv(pBuffer, length);
		return 0;
	}
	unsigned char &TTL = PBuffer[8];
	if(TTL-- == '\000' ) { fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_TTLERROR); return 1;}
	unsigned nextHop;
	if(!route_tree.getNextHop(Des, nextHop)) { fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_NOROUTE); return 1;}

	*(unsigned *)(PBuffer + 16) = ntohl(nextHop);

	PBuffer[10] = PBuffer[11] = 0;
	
	unsigned Sum = 0;
	unsigned short *p_short = (unsigned short *)PBuffer;
	for(int i = 0; i < IHL/2; ++i)
		Sum += ntohl(p_short[i]);
	if(IHL %2 == 1)
		Sum += ((unsigned short)PBuffer[length - 1] << 8);
	while (Sum >> 16)
		Sum = (Sum & 0xFFFF) + (Sum >> 16);

	Sum = ~Sum;

	*(unsigned short *)(PBuffer + 10) = htons(Sum);

	fwd_SendtoLower((char *)PBuffer, length, nextHop);
	return 0;
}

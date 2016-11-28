#include "sysinclude.h"

extern void SendFRAMEPacket(unsigned char* pData, unsigned int len);

#define WINDOW_SIZE_STOP_WAIT 1
#define WINDOW_SIZE_BACK_N_FRAME 4

typedef enum {data,ack,nak} frame_kind;

typedef struct frame_head
{
	frame_kind kind; //帧类型
	unsigned int seq; //序列号
	unsigned int ack; //确认号
	unsigned char data[100]; //数据
};

typedef struct frame
{
	frame_head head; //帧头
	unsigned int size; //数据的大小
};

struct node
{
	
	node() {}

	node(int num)
	{
		node *curr = this;
		while(num > 1)
		{
			--num;
			curr->next = new node();
			curr = curr->next;
		}
		curr->next = this;
	}

	node* next;
	frame data;
	int bufferSize;
};

struct wait_list
{
	wait_list()
	{
		head = new node();
		tail = head;
	}
	
	void push(char *pBuffer, int bufferSize)
	{
		frame *f = (frame *)pBuffer;
		head->data = *f;
		head->bufferSize = bufferSize;
		head->next = new node();
		head = head->next;
	}

	frame pop(int &bufferSize)
	{
		frame ret = tail->data;
		bufferSize = tail->bufferSize;
		node *tmp = tail->next;
		delete tail;
		tail = tmp;
		return ret;
	}

	bool empty()
	{
		return tail == head;
	}

	node *head;
	node *tail;
};

wait_list stop_and_wait_waitlist, back_n_frame_waitlist, choice_frame_resend_waitlist;
node *stop_and_wait_list_tail = new node(WINDOW_SIZE_STOP_WAIT + 1);
node *stop_and_wait_list_head = stop_and_wait_list_tail;

node *back_n_frame_list_head = new node(WINDOW_SIZE_BACK_N_FRAME + 1);
node *back_n_frame_list_tail = back_n_frame_list_head;

node *choice_frame_resend_list_head = new node(WINDOW_SIZE_BACK_N_FRAME + 1);
node *choice_frame_resend_list_tail = choice_frame_resend_list_head;

/*
* 停等协议测试函数
*/
int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType)
{
	frame *f = NULL;
	switch(messageType) 
	{
		case MSG_TYPE_TIMEOUT:
			f = &(stop_and_wait_list_head->data);
			SendFRAMEPacket((unsigned char*)f, stop_and_wait_list_head->bufferSize);
			break;

		case MSG_TYPE_SEND:
			if (stop_and_wait_list_tail->next != stop_and_wait_list_head)
			{
				SendFRAMEPacket((unsigned char*)pBuffer, bufferSize);
				f = (frame *)pBuffer;
				stop_and_wait_list_tail->data = *f;
				stop_and_wait_list_tail->bufferSize = bufferSize;
				stop_and_wait_list_tail = stop_and_wait_list_tail->next;
			}
			else
			{
				stop_and_wait_waitlist.push(pBuffer, bufferSize);
			}
			break;

		case MSG_TYPE_RECEIVE:
			stop_and_wait_list_head = stop_and_wait_list_head->next;
			if(!stop_and_wait_waitlist.empty())
			{
				int size;
				stop_and_wait_list_tail->data = stop_and_wait_waitlist.pop(size);	
				stop_and_wait_list_tail->bufferSize = size;
				f = &(stop_and_wait_list_tail->data);
				SendFRAMEPacket((unsigned char*)f, stop_and_wait_list_tail->bufferSize);
				stop_and_wait_list_tail = stop_and_wait_list_tail->next;
			}
			break;
	}
	return 0;
}

/*
* 回退n帧测试函数
*/
int stud_slide_window_back_n_frame(char *pBuffer, int bufferSize, UINT8 messageType)
{
	frame *f = NULL;
	node *tmp;
	switch(messageType) 
	{
		case MSG_TYPE_TIMEOUT:
			tmp = back_n_frame_list_head;
			while(tmp != back_n_frame_list_tail)
			{
				f = &(tmp->data);
				SendFRAMEPacket((unsigned char*)f, tmp->bufferSize);
				tmp = tmp->next;
			}
			break;

		case MSG_TYPE_SEND:
			if (back_n_frame_list_tail->next != back_n_frame_list_head)
			{
				SendFRAMEPacket((unsigned char*)pBuffer, bufferSize);
				f = (frame *)pBuffer;
				back_n_frame_list_tail->data = *f;
				back_n_frame_list_tail->bufferSize = bufferSize;
				back_n_frame_list_tail = back_n_frame_list_tail->next;
			}
			else
			{
				back_n_frame_waitlist.push(pBuffer, bufferSize);
			}
			break;

		case MSG_TYPE_RECEIVE:
			f = (frame *) pBuffer;
			while (ntohl((back_n_frame_list_head->data).head.seq) <= ntohl(f->head.ack) && back_n_frame_list_head != back_n_frame_list_tail)
				back_n_frame_list_head = back_n_frame_list_head->next;

			while(!back_n_frame_waitlist.empty() && back_n_frame_list_tail->next != back_n_frame_list_head )
			{
				int size;
				back_n_frame_list_tail->data = back_n_frame_waitlist.pop(size);	
				back_n_frame_list_tail->bufferSize = size;
				f = &(back_n_frame_list_tail->data);
				SendFRAMEPacket((unsigned char*)f, back_n_frame_list_tail->bufferSize);
				back_n_frame_list_tail = back_n_frame_list_tail->next;
			}
			break;
	}
	return 0;
}

/*
* 选择性重传测试函数
*/
int stud_slide_window_choice_frame_resend(char *pBuffer, int bufferSize, UINT8 messageType)
{
	frame *f = NULL;
	node *tmp;
	switch(messageType) 
	{
		case MSG_TYPE_TIMEOUT:
			tmp = choice_frame_resend_list_head;
			while(tmp != choice_frame_resend_list_tail)
			{
				f = &(tmp->data);
				SendFRAMEPacket((unsigned char*)f, tmp->bufferSize);
				tmp = tmp->next;
			}
			break;

		case MSG_TYPE_SEND:
			if (choice_frame_resend_list_tail->next != choice_frame_resend_list_head)
			{
				SendFRAMEPacket((unsigned char*)pBuffer, bufferSize);
				f = (frame *)pBuffer;
				choice_frame_resend_list_tail->data = *f;
				choice_frame_resend_list_tail->bufferSize = bufferSize;
				choice_frame_resend_list_tail = choice_frame_resend_list_tail->next;
			}
			else
			{
				choice_frame_resend_waitlist.push(pBuffer, bufferSize);
			}
			break;

		case MSG_TYPE_RECEIVE:
			f = (frame *) pBuffer;
			if (ntohl(f->head.kind) == ack)
			{
				while (ntohl(choice_frame_resend_list_head->data.head.seq) <= ntohl(f->head.ack) && choice_frame_resend_list_head->next != choice_frame_resend_list_tail)
					choice_frame_resend_list_head = choice_frame_resend_list_head->next;
				while (!choice_frame_resend_waitlist.empty() && choice_frame_resend_list_tail->next != choice_frame_resend_list_head )
				{
					int size;
					choice_frame_resend_list_tail->data = choice_frame_resend_waitlist.pop(size);	
					choice_frame_resend_list_tail->bufferSize = size;
					f = &(choice_frame_resend_list_tail->data);
					SendFRAMEPacket((unsigned char*)f, choice_frame_resend_list_tail->bufferSize);
					choice_frame_resend_list_tail = choice_frame_resend_list_tail->next;
				}
			}
			else
			{
				node *tmp = choice_frame_resend_list_head;
				while (ntohl(tmp->data.head.seq) != ntohl(f->head.ack) && choice_frame_resend_list_head != choice_frame_resend_list_tail)
					tmp = tmp->next;
				frame tmp_frame = tmp->data;
				f = &tmp_frame;
				SendFRAMEPacket((unsigned char*)f, tmp->bufferSize);
			}
			break;
	}
	return 0;
}

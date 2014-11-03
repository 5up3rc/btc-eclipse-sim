#ifndef CONNECTION_C
#define CONNECTION_C

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<unistd.h>
#include<signal.h>
#include<pthread.h>

#include"connection.h"

#define HDR_SIZE	16

void remove_links(struct links *will_remove){
	struct links *after, *before;
	after	= will_remove->next;
	before	= will_remove->prev;
	free(will_remove->link);
	free(will_remove);
	if(after!=NULL&&before!=NULL){
		after->prev	= before;
		before->next= after;
	}
	else if(after!=NULL){
		after->prev	= NULL;
	}
	else{
		before->next= NULL;
	}
}

struct links *add_links(unsigned int miner_id, struct link *dest, struct link *new_comer, struct links *links){
	struct links *tmp, *new;
	for(tmp=links; tmp->next!=NULL; tmp=tmp->next){}
	new			= malloc(sizeof(struct links));
	new->link	= malloc(sizeof(struct link));
	new->prev	= tmp;
	tmp->next	= new;
	new->miner_id		= miner_id;
	(new->link)->dest	= dest;
}

int send_msg(struct link *dest, char *message, int msg_size){
	unsigned int pos, over_size;
	pos = dest->write_pos;

	pthread_mutex_lock((pthread_mutex_t *)&dest->rcv_mutex);
	if(pos+msg_size < BUF_SIZE){
		memcpy(&dest->buf[pos], message, msg_size);
		dest->write_pos += msg_size;
	}
	else{
		over_size = (pos+msg_size) - BUF_SIZE;
		memcpy(&dest->buf[pos], message, (msg_size-over_size));
		memcpy(&dest->buf[0], &message[msg_size-over_size], over_size);
		dest->write_pos = over_size;
	}
	dest->num_msg += 1;
	pthread_mutex_unlock((pthread_mutex_t *)&dest->rcv_mutex);
	return 1;
}

int read_msg(struct link *link){
	const struct msg_hdr	*hdr;
	int						read_size, over_size;

	if(!(int)link->num_msg){
		return 0;
	}
	pthread_mutex_lock(&link->rcv_mutex);
	hdr 		= (struct msg_hdr*)&link->buf[(link)->read_pos];
	read_size	= HDR_SIZE + hdr->message_size;
//assumes that the BUF_SIZE is large enough for a message
	if((link->read_pos+read_size)<BUF_SIZE){
		memcpy(&link->process_buf, hdr, read_size);
		link->read_pos += read_size;
	}
	else{
		over_size = (read_size+link->read_pos) - BUF_SIZE;
		memcpy(link->process_buf, hdr, read_size-over_size);
		memcpy(&link->process_buf[read_size-over_size], link->buf, over_size);
		link->read_pos = over_size;
	}
	link->num_msg -= 1;
	pthread_mutex_unlock(&link->rcv_mutex);
	return 1;
}


#endif

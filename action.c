#ifndef ACTION_C
#define ACTION_C

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include"block.h"
#include"thread.c"
#include"connection.c"
#include"proto-node.h"


void dns_seed(int my_id, struct dns *dns, struct link *new_comer){
    struct links	*tmp, *new;
    struct link		*link;
    unsigned int	size, payload_size;
    size			= sizeof(struct link*);
	link			= new_comer; 
    link->dest		= &dns->new_comer;
	payload_size	= size + sizeof(unsigned int);
    memcpy(&link->sbuf[0], "dnsseed", 7);
    memcpy(&link->sbuf[12], &payload_size, sizeof(unsigned int));
    memcpy(&link->sbuf[16], &new_comer, size);
	memcpy(&link->sbuf[16+size], &my_id, sizeof(unsigned int));
    send_msg(&dns->new_comer, link->sbuf, 16+payload_size);
}
struct links *seed_receive(struct link *new_comer, struct links *seeds){
	unsigned int	miner_id, size;
	struct link		*link;
	struct links	*new;
	size = sizeof(struct link*);
	memcpy(&link, &new_comer->process_buf[16], sizeof(struct link*));
	memcpy(&miner_id, &new_comer->process_buf[16+size], sizeof(unsigned int));
	fprintf(stderr, "seed_receive(): will add_links()\n");
	return add_links(miner_id, link, link, seeds);
}
void dns_query(struct dns *dns, struct link *new_comer){
	unsigned int	size;
	struct link		*link;
	struct links	*tmp;
	size		= sizeof(struct link*);
	link		= new_comer;
	link->dest	= &(dns->new_comer);
	memcpy(&link->sbuf[0], "dnsquery", 8);
	memcpy(&link->sbuf[12], &size, 4);
	memcpy(&link->sbuf[16], &new_comer, size);
	send_msg(&dns->new_comer, link->sbuf, 16+size);  
}

void dns_roundrobin(struct link *new_comer, struct links *seeds){
	unsigned int	i, j, size, payload_size;
	struct link		*link;
	struct links	*tmp, *head;
	size			= sizeof(struct link*);
	payload_size	= size + sizeof(unsigned int);
	link 			= new_comer;
	for(tmp=seeds;tmp->prev!=NULL;tmp=tmp->prev){
		;
	}
	head=tmp;
	for(i=1; tmp->next!=NULL; i++){
		tmp=tmp->next;
	}
	j=rand()%i;
	tmp = head;
	for(i=0; i<j; tmp=tmp->next/*i++*/){
		i++;//	tmp = tmp->next;
	}
	memcpy(&link->dest, &link->process_buf[16], size);
	memcpy(&link->sbuf[0], "roundrobin", 10);
	memcpy(&link->sbuf[12], &payload_size, 4);
	memcpy(&link->sbuf[16], &tmp->new_comer, size);
	memcpy(&link->sbuf[16+size], &tmp->miner_id, sizeof(unsigned int));
	send_msg(link->dest, link->sbuf, 16+payload_size);
}

void getaddr(struct link *dest){
	unsigned int	size;
	struct link 	*link;
	size = 0;
	link = dest;
	memcpy(&link->sbuf[0], "getaddr", 7);
	memcpy(&link->sbuf[12], &size, 4);
	send_msg(link->dest, link->sbuf, 16);
}

void addr(struct link *dest, struct miner *me){
	unsigned int	size, i;
	struct link		*link, *tmp;
	struct links 	*links;
	link = dest;
	size = 0;
	memcpy(&link->sbuf[0], "addr", 4);
	for(links=me->links; links->prev!=NULL; links=links->prev){}
	for(i=0; links!=NULL; i++){
		link=links->link;
		memcpy(&link->sbuf[16+(i*sizeof(struct link*))], &links->miner_id, sizeof(unsigned int));	
		memcpy(&link->sbuf[16+(i*sizeof(struct link*))+sizeof(unsigned int)], &links->new_comer, sizeof(struct link*));
		links=links->next;
	}
	size=i*(sizeof(struct link*)+sizeof(unsigned int));
	memcpy(&link->sbuf[12], &size, 4);
	send_msg(link->dest, link->sbuf, 16);
}
//dest is desination's new_comer
struct links *version(unsigned int my_id, unsigned int dest_id, struct link *dest, struct link *new_comer, struct links *links){
	unsigned int miner_id;
	struct links *new;
	struct link *link;
	unsigned int size, total;
	fprintf(stderr, "will send version msg\n");
	size = sizeof(struct link*);
	total = size+sizeof(unsigned int)+sizeof(struct link*);
	new = add_links(dest_id, dest, dest,  links);
	link = new->link;
	memcpy(&link->sbuf[0], "version", 7);
	memcpy(&link->sbuf[12], &total, 4);
	memcpy(&link->sbuf[16], &link, size);
	memcpy(&link->sbuf[16+size], &my_id, sizeof(unsigned int));
	memcpy(&link->sbuf[16+size+sizeof(unsigned int)], new_comer, sizeof(struct link*));
	send_msg(dest, link->sbuf, 16+size+sizeof(unsigned int));
	fprintf(stderr, "sent version to: %d\n", new->miner_id); //debug
}

struct links *verack(struct link *new_comer, struct links *links){
	unsigned int miner_id;
	struct links *tmp, *new;
	struct link *link, *dest, *dest_new_comer;
	unsigned int size;
	fprintf(stderr, "will send verack msg\n"); //debug
	size = sizeof(struct link*);
	dest		= (struct link*)&new_comer->process_buf[16];
	memcpy(&miner_id, &new_comer->process_buf[16+size], sizeof(unsigned int));
	memcpy(&dest_new_comer, &new_comer->process_buf[16+size+sizeof(unsigned int)], sizeof(struct link*));
	tmp			= add_links(miner_id, dest, dest_new_comer, links);
	link		= tmp->link;
	memcpy(&link->sbuf[0], "verack", 6);
	memcpy(&link->sbuf[12], &size, 4);
	memcpy(&link->sbuf[16], &link, size);
	send_msg(link->dest, link->sbuf, 16+size);
	fprintf(stderr, "sent verack to: %d\n", tmp->miner_id); //debug
}

struct blocks *add_block(struct block *block, struct blocks *chain_head){
	struct blocks *tmp;
	tmp = malloc(sizeof(struct blocks));
	tmp->block			= block;
	chain_head->next 	= tmp;
	tmp->prev 			= chain_head;
	return tmp;
}

void send_block(struct block *block, struct link *dest){
	int size;
	struct link *link;
	size = sizeof(struct block);
	link = dest;
	memcpy(&link->sbuf[0], "block", 5);
	memcpy(&link->sbuf[12], &size, 4);
	memcpy(&link->sbuf[16], block, size);
	send_msg(link->dest, link->sbuf, 16+size);
}

void propagate_block(struct block *block, struct miner *me){
	struct link		*link;
	struct links	*links;
	for(links=me->links; links->prev!=NULL;links=links->prev){}
	for(; links!=NULL; links=links->next){
		link = links->link;
		send_block(block, link);
	}
}

struct blocks *mine_block(struct blocks *chain_head, unsigned int miner_id, struct miner *me){
	int				x, y, times, mined;
	struct block	*head, *current;
	struct blocks	*tmp;
	struct link		*link;
	struct links	*links;

	tmp = chain_head;
	if(tmp!=NULL){
		current = tmp->block;
	}else{
		current = NULL;
	}
	mined = 0;
	for(times = 0;times < 1000;times++){
		x = rand()/(RAND_MAX);
		y = rand()/(RAND_MAX);
		if((x*x+y*y)>0.5){
			fprintf(stderr,"mined block in %d times\n", times);
			mined			= 1;
			head			= malloc(sizeof(struct block));
//			head->prev      = current;
			if(current->height!=0){
				head->height	= current->height++;
				memcpy(head->prev, SHA256((char *)current, sizeof(struct block), 0), SHA256_DIGEST_LENGTH);
			}else{
				head->height	= 1;
				memset(head->prev, 0, SHA256_DIGEST_LENGTH);
			}
//			head->time              = ~~~
			head->miner_id	= miner_id;
			head->size		= sizeof(struct block);
			head->valid		= true;
			break;
		}
	}
	if(mined){
		tmp = add_block(head, tmp);
		propagate_block(head, me);
	}
	return tmp;
}

void request_block(unsigned int wanted_height, struct link *dest){
	int size;
	struct link *link;
	size = sizeof(unsigned int);
	link = dest;
	memcpy(&link->sbuf[0], "getblock", 8);
	memcpy(&link->sbuf[12], &size, 4);
	memcpy(&link->sbuf[16], &wanted_height, size);
	send_msg(link->dest, link->sbuf, 16+size);
}
int verify_block(struct block *new_block, struct blocks *chain_head){

	struct block *tmp;
	tmp = chain_head->block;
	if(tmp->height >= new_block->height){
		return -1;
	}
	else if(memcmp(new_block->prev, SHA256((char *)tmp, sizeof(struct block), 0), SHA256_DIGEST_LENGTH)){
		return 0;
	}
	return 1;

}
struct blocks *process_new_blocks(struct block *block, struct blocks *chain_head, struct link *from){
	int rerun;
	int validity;
	struct block *accept;
	struct blocks *tmp;
	tmp = chain_head;
	for(rerun = 1; rerun==1;){
		rerun = 0;
		for(;;){
			validity = verify_block(block, tmp);
			if(validity==1){
				accept = malloc(sizeof(block));
				memcpy(accept, block, sizeof(block));
				add_block(accept, chain_head);
			}
			else if(validity==0){
				request_block((block->height-1), from);
				tmp = tmp->prev;
			}
			else if(validity==-1){
				return chain_head;
			}
		}
	}
	return chain_head;
}
struct links *process_dns(struct link *new_comer, struct links *seeds){
	char 					*payload;
	struct link				*link;
	struct links			*tmp;
	const struct msg_hdr	*hdr;
	link	= new_comer;
	hdr		= (struct msg_hdr*)(link->process_buf);
	payload	= (char *)(hdr+sizeof(struct msg_hdr));
	if(strncmp(hdr->command, "dnsseed", 7)==0){
		fprintf(stderr, "dnsseed received\n"); //debug
		tmp = seed_receive(link, seeds);
		if(tmp!=NULL)
			return tmp;
		else
			return seeds;
	}
	else if(strncmp(hdr->command, "dnsquery", 8)==0){
		fprintf(stderr, "dnsquery received\n"); //debug
		dns_roundrobin(link, seeds);
		return seeds;
	}
	else{
		return seeds;
	}
}
int process_msg(struct link *new_comer,struct links *links, struct miner *me){
	char				*payload;
	unsigned int		height, i, miner_id;
	struct link			*link;
	const struct msg_hdr *hdr;
	struct block		*block;
	struct blocks		*blocks;

	link = links->link;
	hdr = (struct msg_hdr*)(link->process_buf);
	payload = (char *)(hdr +sizeof(struct msg_hdr));
	fprintf(stderr, "check command\n"); //debug
	if(strncmp(hdr->command, "block", 5)==0){
		me->blocks = process_new_blocks((struct block*)payload, me->blocks, link);
	}
/*	else if(strncmp(hdr->command, "newhead", 7)==0){

	}
*/
	else if(strncmp(hdr->command, "getblock", 8)==0){
		height = (unsigned int)*payload;
		for(blocks=me->blocks; blocks->next!=NULL; blocks=blocks->next){}
		for(;block->height!=height||blocks!=NULL;blocks=blocks->prev){
			block=blocks->block;
		}
		if(blocks==NULL)
			return;
		else{
			send_block(block, link);
		}
	}
	else if(strncmp(hdr->command, "roundrobin", 10)==0){
		version(me->miner_id, (unsigned int)*(payload+sizeof(struct link*)), (struct link*)payload, &me->new_comer, links);
	}
	else if(strncmp(hdr->command, "getaddr", 7)==0){
		addr(link, me);
	}
	else if(strncmp(hdr->command, "addr", 4)==0){
		for(i=0; i<(hdr->message_size/(sizeof(struct link*)+sizeof(unsigned int))); i++){
			memcpy(&miner_id, &payload[i*sizeof(struct link*)], sizeof(unsigned int));
			version(me->miner_id, miner_id, (struct link*)(&payload[i*sizeof(struct link*)+sizeof(unsigned int)]), &me->new_comer, links);
		}
	}
	return 0;
}

struct links *process_new(struct link *new_comer,struct links *links, struct miner *me){
    char                *payload;
    unsigned int        height, i, miner_id;
    struct link         *link;
    const struct msg_hdr *hdr;
    struct block        *block;
    struct blocks       *blocks;

    link = new_comer;
    hdr = (struct msg_hdr*)(link->process_buf);
    payload = (char *)(hdr +sizeof(struct msg_hdr));
    fprintf(stderr, "check command\n"); //debug
    if(strncmp(hdr->command, "roundrobin", 10)==0){
        return version(me->miner_id, (unsigned int)*(payload+sizeof(struct link*)), (struct link*)payload, &me->new_comer, links);
    }
    else if(strncmp(hdr->command, "version", 6)==0){
        return verack(new_comer, links);
    }
    else if(strncmp(hdr->command, "verack", 6)==0){
        link->dest = (struct link*)payload;
    }
    return NULL;
}


#endif

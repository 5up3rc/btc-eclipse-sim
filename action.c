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
#include"proto-node.c"

#ifndef ACTION_C
#define ACTION_C

void dns_query(){
    
}
void dns_roundrobin(){

}
void version(){
}
void verack(){
}
//chain_head = add_block(&new_block, &chain_head);
struct blocks *add_block(struct block *block, struct blocks *chain_head){
	struct blocks *tmp;
	tmp = malloc(sizeof(struct blocks));
	tmp->block			= block;
	chain_head->next 	= tmp;
	tmp->prev 			= chain_head;
	return tmp;
}
struct blocks *mine_block(struct blocks *chain_head, unsigned int miner_id){
	int x, y, times, mined;
	struct block *head, *current;
	struct blocks *tmp;
	tmp = chain_head;
	current = tmp->block;
	mined = 0;
	for(times = 0;times < 1000;times++){
		x = rand()/(RAND_MAX);
		y = rand()/(RAND_MAX);
		if((x*x+y*y)>0.5){
			fprintf(stderr,"mined block in %d times\n", times);
			mined = 1;
			head 			= malloc(sizeof(struct block));
			head->prev		= chain_head;
			if(current->height!=0){
				head->height	= current->height++;
			}else{
				head->height	= 1;
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
	}
	return tmp;
}
void get_block(struct link *link){

}
void request_block(struct block *wanted, struct link *dest){
	
}
int verify_block(struct block *new_block, struct blocks *chain_head){

	struct block *tmp;
	tmp = chain_head->block;
	if(tmp->height >= new_block->height){
		return -1;
	}
	else if(new_block->prev != (tmp)){
		return 0;
	}
	return 1;

}
struct blocks *process_new_blocks(struct block *block, struct blocks *chain_head, struct link *from){
	int rerun;
	int validity;
	struct blocks *tmp;
	tmp = chain_head;
	for(rerun = 1; rerun==1;){
		rerun = 0;
		for(;;){
			validity = verify_block(block, tmp);
			if(validity==1){
				add_block(block, chain_head);
			}
			else if(validity==0){
				request_block(block->prev, from);
				tmp = tmp->prev;
			}
			else if(validity==-1){
				return chain_head;
			}
		}
	}
	return chain_head;
}

int process_msg(char *msg_ptr){
	const struct msg_hdr *hdr;
	hdr = msg_ptr;
	if(strncmp(hdr->command, "addblock", 12)){
		
	}
	else if(strncmp(hdr->command, "block", 12)){
		
	}
	else if(strncmp(hdr->command, "newhead", 12)){

	}
	else if(strncmp(hdr->command, "getblock", 12)){

	}
	else if(strncmp(hdr->command, "roundrobin", 12)){

	}
	else if(strncmp(hdr->command, "getaddr", 12)){
		
	}
	else if(strncmp(hdr->command, "addr", 12)){

	}
	else if(strncmp(hdr->command, "version", 12)){
		
	}
	else if(strncmp(hdr->command, "verack", 12)){

	}
	return 0;
}

#endif

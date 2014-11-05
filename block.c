#ifndef BLOCK_C
#define BLOCK_C

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include"block.h"
#include"connection.h"
#include"proto-node.h"

void request_block(unsigned int wanted_height, struct link *dest);

struct blocks *add_block(struct block *block, struct blocks *chain_head){
	struct blocks *tmp;
	tmp = malloc(sizeof(struct blocks));
	memset(tmp, 0, sizeof(struct blocks));
	tmp->block          = block;
	if(chain_head!=NULL){
		chain_head->next    = tmp;
		tmp->prev           = chain_head;
		tmp->next           = NULL;
	}else{
		tmp->prev           = NULL;
		tmp->next           = NULL;
	}
	return tmp;
}

struct blocks *connect_block(struct block *block, struct blocks *new_chain){
	struct blocks *tmp;
	tmp = malloc(sizeof(struct blocks));
	memset(tmp, 0, sizeof(struct blocks));
	tmp->block          = block;    
	if(new_chain!=NULL){
		new_chain->prev     = tmp;
		tmp->next           = new_chain;
		tmp->next           = NULL;
	}else{
		tmp->prev           = NULL;
		tmp->next           = NULL;
	}
	return tmp;
}

struct blocks *search_prev(struct blocks *chain_head, struct blocks *new_chain){
	struct block *new, *search;
	struct blocks *tmp;
	new = new_chain->block;
	if(new->height==1){
		tmp = new_chain;
		return tmp;
	}
	for(tmp=chain_head; tmp->prev!=NULL; tmp=tmp->prev){
		search = tmp->block;
		if(!memcmp(new->prev, SHA256((char *)search, sizeof(struct block), 0), SHA256_DIGEST_LENGTH)){  
			tmp->next       = new_chain;
			new_chain->prev = tmp;
			for(tmp=new_chain; tmp->next!=NULL; tmp=tmp->next){}
			return tmp;
		}
	}
}

int verify_block(struct block *new_block, struct blocks *chain_head, struct blocks *new_chain){

	struct block *tmp;
	struct blocks *blocks;
	if(new_chain!=NULL){
		for(blocks = new_chain; blocks->prev!=NULL; blocks=blocks->prev){;}
		tmp = blocks->block;
		if(!memcmp(tmp->prev, SHA256((char *)tmp, sizeof(struct block), 0), SHA256_DIGEST_LENGTH)){
			return 2;
		}
	}
	else if(chain_head==NULL&&new_block->height ==1)
		return 1;

	tmp = chain_head->block;
	if(tmp->height >= new_block->height){
		fprintf(stderr, "new block's height equal or lower than mine\n");//debug
		return -1;
	}
	else if(memcmp(new_block->prev, SHA256((char *)tmp, sizeof(struct block), 0), SHA256_DIGEST_LENGTH)){
		fprintf(stderr, "don't have new block's previous block\n"); //debug
		return 0;
	}
	return 1;

}

struct blocks *process_new_blocks(struct block *block, struct blocks *chain_head, struct miner *me, struct link *from){
    int rerun;
    int validity;
    struct block *accept;
    struct blocks *tmp;
    fprintf(stderr, "process_new_blocks()\n"); //debug
    tmp = chain_head;
    for(rerun = 1; rerun==1;){
        rerun = 0;
        for(;;){
            validity = verify_block(block, tmp, me->new_chain);
            fprintf(stderr, "checked validity\n");
            if(validity==2){
                fprintf(stderr, "added to new_chain\n");
                me->new_chain   = connect_block(block, me->new_chain);
                me->blocks      = search_prev(me->blocks, me->new_chain);
                return me->blocks;
            }
            if(validity==1){
                accept = malloc(sizeof(block));
                memcpy(accept, block, sizeof(block));
                fprintf(stderr, "accepted block with height: %d\n", accept->height); // debug
                return add_block(accept, chain_head);
            }
            else if(validity==0){
                me->new_chain = connect_block(block, me->new_chain);
                request_block((block->height-1), from);
                if(tmp->prev==NULL)
                    return chain_head;
                tmp = tmp->prev;
            }
            else if(validity==-1){
                return chain_head;
            }
        }
    }
    return chain_head;
}

#endif

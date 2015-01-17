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

#include"global.c"
#include"block.h"
#include"connection.h"
#include"proto-node.h"
void join_record(struct block *new, struct blocks *blocks);
void get_blocks(struct link *dest, struct blocks *main_chain, struct blocks *new_chain);
void request_block(unsigned int wanted_height, struct link *dest);
void propagate_block(struct block *block, struct miner *me);
void send_block(struct block *block, struct link *dest);

struct blocks *add_block(struct block *block, struct blocks *chain_head){
	struct blocks *tmp, *tmp2;
	tmp = malloc(sizeof(struct blocks));
	if(tmp==NULL){
		perror("malloc");
		exit(-1);
	}
	memset(tmp, 0, sizeof(struct blocks));
	tmp->block          = block;
	tmp->next			= NULL;
	if(chain_head!=NULL){
		for(tmp2=chain_head; tmp2->next!=NULL; tmp2=tmp2->next){}
		tmp2->next			= tmp;
		tmp->prev           = tmp2;
	}else{
		tmp->prev           = NULL;
	}
	return tmp;
}

struct blocks *process_new_blocks(struct block *new_block, struct blocks *chain_head, struct miner *me, struct link *from){
	struct block	*accept=NULL, *head=NULL;
	struct blocks	*tmp=NULL, *tmp2=NULL, *tmp3=NULL, *check_new=NULL;
#ifdef DEBUG
	fprintf(stderr, "will process block: chain_head= %p new_chain=%p\n", chain_head, me->new_chain);
#endif
	accept = malloc(sizeof(struct block));
/*	if(accept == NULL){
		perror("malloc");
		exit(-1);
	}
*/
	memcpy(accept, new_block, sizeof(struct block));
	if(chain_head!=NULL){
		if(chain_head->block->height > accept->height && me->new_chain==NULL){
			free(accept);
			return chain_head;
		}
	}

	if(me->new_chain!=NULL){
//		fprintf(stderr, "will check in the new_chain\n");
		for(tmp=me->new_chain; tmp->next!=NULL; tmp=tmp->next){}
		head=tmp->block;
//		fprintf(stderr, "will check, if it's next one to new_chain\n");
		if(accept->height == head->height + 1 && !memcmp(accept->hash, SHA256((const unsigned char *)head, sizeof(struct block), 0), SHA256_DIGEST_LENGTH)){
#ifdef DEBUG
			fprintf(stderr, "new block added to new_chain's head\n"); //debug
#endif
			me->new_chain = add_block(accept, tmp);
			for(; tmp->prev!=NULL; tmp=tmp->prev){}
			head = tmp->block;
			//get_blocks(from, me->blocks, me->new_chain);	
			from->fgetblock=true;
			return chain_head;
		}
		for(tmp=me->new_chain; tmp->prev!=NULL; tmp=tmp->prev){}
		head = tmp->block;
		if(head->height == accept->height+1 && !memcmp(head->hash, SHA256((const unsigned char *)accept, sizeof(struct block), 0), SHA256_DIGEST_LENGTH)){
#ifdef DEBUG
			fprintf(stderr,"new block added to tail of new_chain\n"); //debug
#endif
			tmp2 = malloc(sizeof(struct blocks));
			tmp2->block	= accept;
			tmp2->prev	= NULL;
			tmp2->next	= tmp;
			tmp->prev	= tmp2;
			tmp3		= tmp;
			if(accept->height == 1){
#ifdef DEBUG
				fprintf(stderr, "new block was height 1\n");
#endif
				if(me->blocks!=NULL){
					for(tmp=me->blocks; tmp->next!=NULL; tmp=tmp->next){}
					for(; tmp!=NULL; tmp=tmp2){
							tmp2 = tmp->prev;
							free(tmp->block);
							tmp->block=NULL;
							free(tmp);
					}			
				}
				for(tmp=tmp3; tmp->next!=NULL; tmp=tmp->next){}
				me->new_chain = NULL;
				me->blocks=tmp;
				propagate_block(tmp->block, me);
				join_record(tmp->block, tmp);
				from->fgetblock=false;
				return tmp;
			}
			else{
				for(tmp=chain_head; tmp!=NULL; tmp=tmp->prev){
					head=tmp->block;
					if(accept->height==head->height+1){
						if(/*accept->height==head->height+1 &&*/ !memcmp(accept->hash, SHA256((const unsigned char *)head, sizeof(struct block), 0), SHA256_DIGEST_LENGTH)){
#ifdef DEBUG
							fprintf(stderr, "new block connected to main chain\n");
#endif
							tmp3		= tmp->next;
							tmp->next	= tmp2;
							tmp2->prev	= tmp;
							for(tmp=tmp3; tmp!=NULL; tmp=tmp3){
								tmp3	= tmp->next;
								free(tmp->block);
								tmp->block=NULL;
								free(tmp);		
							}
							for(tmp=tmp2; tmp->next!=NULL;  tmp=tmp->next){}
							me->new_chain=NULL;
							me->blocks=tmp;
							propagate_block(tmp->block, me);
							join_record(tmp->block, me->blocks);
							from->fgetblock=false;
							return tmp;
						}
					}
				}
				if(tmp==NULL){
					from->fgetblock=true;
					return me->blocks;
				}
			}
#ifdef DEBUG
			fprintf(stderr, "new_chain still disconnected to main chain\n");
#endif
			//get_blocks(from, me->blocks, me->new_chain);
			from->fgetblock=true;
			return me->blocks;
		}
	}
//	fprintf(stderr, "will check if it's lower or equal\n");
	if(chain_head==NULL&&accept->height==1){
#ifdef DEBUG
		fprintf(stderr, "genesis block received\n"); //debug
#endif
		me->blocks=add_block(accept, NULL);
		propagate_block(accept, me);
		join_record(accept, me->blocks);
		return me->blocks;
	}
	if(chain_head!=NULL){
//		fprintf(stderr, "will check if it's next block\n");
		for(tmp=chain_head; tmp->next!=NULL; tmp=tmp->next){}
		head = chain_head->block;
		if(accept->height == head->height+1 && !memcmp(accept->hash, SHA256((const unsigned char *)head, sizeof(struct block), 0), SHA256_DIGEST_LENGTH)){
#ifdef DEBUG
			fprintf(stderr, "next block received\n"); //debug
#endif
			tmp = add_block(accept, chain_head);
			propagate_block(accept, me);
			join_record(accept, me->blocks);
			return tmp;
		}
	}
	if(accept->height > 1 && chain_head==NULL){
		if(me->new_chain!=NULL){
			for(tmp=me->new_chain; tmp->next!=NULL; tmp=tmp->next){}
			if(accept->height <= tmp->block->height){
			}
			else{
#ifdef DEBUG
				fprintf(stderr, "free new_chain\n");
#endif
				if(me->new_chain!=NULL){
					for(tmp=me->new_chain; tmp->next!=NULL; tmp=tmp->next){}
					for(; tmp!=NULL; tmp=tmp2){
						tmp2 = tmp->prev;
						free(tmp->block);
						free(tmp);
					}
				}
				me->new_chain = malloc(sizeof(struct blocks));
				memset(me->new_chain, 0, sizeof(struct blocks));
				(me->new_chain)->block	= accept;
				(me->new_chain)->next	= NULL;
				(me->new_chain)->prev	= NULL;
				//get_blocks(from, me->blocks, me->new_chain);
				from->fgetblock=true;
			
				return NULL;
			}
		}
		else{
			me->new_chain = malloc(sizeof(struct blocks));
			memset(me->new_chain, 0, sizeof(struct blocks));
			(me->new_chain)->block	= accept;
			(me->new_chain)->next	= NULL;
			(me->new_chain)->prev	= NULL;
			//get_blocks(from, me->blocks, me->new_chain);
			from->fgetblock=true;
		
			return NULL;
		}
	}
//	fprintf(stderr, "check if higher than my chain height\n");
	if(chain_head!=NULL && me->new_chain!=NULL){
		head = chain_head->block;
		for(check_new=me->new_chain; check_new->next!=NULL; check_new=check_new->next){}
		if(accept->height > head->height+1 && accept->height > /*me->new_chain*/check_new->block->height ){
#ifdef DEBUG
			fprintf(stderr, "need previous block\n"); //debug
#endif
			if(me->new_chain!=NULL){
#ifdef DEBUG
				fprintf(stderr, "free new_chain\n");
#endif
				for(tmp=me->new_chain; tmp->next!=NULL; tmp=tmp->next){	}
				for(; tmp!=NULL; tmp=tmp2){
					tmp2 = tmp->prev;
					free(tmp->block);
//					tmp->block=NULL;
					free(tmp);
				}
			}
			me->new_chain = malloc(sizeof(struct blocks));
			memset(me->new_chain, 0, sizeof(struct blocks));
			(me->new_chain)->block = accept;
			(me->new_chain)->next = NULL;
			(me->new_chain)->prev = NULL;
//			fprintf(stderr, "will request for it's prev block\n");
			from->fgetblock=true;
			return chain_head;
		}
	}
	if(me->blocks!=NULL && me->new_chain==NULL){
		if(accept->height > me->blocks->block->height+1){
			me->new_chain = malloc(sizeof(struct blocks));
			memset(me->new_chain, 0, sizeof(struct blocks));
			(me->new_chain)->block = accept;
			(me->new_chain)->next = NULL;
			(me->new_chain)->prev = NULL;
	//		fprintf(stderr, "will request for it's prev block\n");
			from->fgetblock=true;
			return chain_head;
		}
	}
#ifdef DEBUG
	fprintf(stderr, "block received not added\n");
#endif	
	if(me->blocks!=NULL){
		for(tmp=me->blocks; tmp->next!=NULL; tmp=tmp->next){}
		if(accept->height<tmp->block->height)
			send_block(tmp->block, from);
	}
	free(accept);
	return chain_head;
}

/*
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
*/
#endif

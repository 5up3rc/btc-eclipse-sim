#ifndef GLOBAL_C
#define GLOBAL_C
#include"params.h"
#include"block.h"
#include"proto-node.h"
#include"connection.h"
struct dns;
struct links;
struct block_record;

unsigned int		sim_time = 0, global_id = 0;
bool				is_bad_dns[NUM_DNS];
struct links		*bad_links = NULL;
struct bad_threads		*bad_threads=NULL;
struct dns 			dns[NUM_DNS];
struct miner		seeds[SEED_NUM];
struct killed		*dead = NULL;

//for recording blocks
struct block_record *record = NULL;
#ifdef	MULTI
pthread_mutex_t block_mutex;
#endif

//for recording links
struct node_record	*n_link = NULL;

#endif


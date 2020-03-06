#include<unistd.h>
#include<stdio.h>
#include<sys/types.h>
#include<limits.h>
#include <pthread.h>
//version 1
void* ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);
//version 2
void* ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);
//project1
void* bf_malloc(size_t size);
void bf_free(void *ptr);

//the structure of the block
struct Meta_Control_Block{
  size_t size;
  struct Meta_Control_Block * fnext;
};
typedef struct Meta_Control_Block block; 

//helper functions
void merge_free_block(block* pmcb);
void split_block(block* pcurrent,size_t size);
void* extend_heap(size_t size);
void* bf_find(size_t size);
void remove_free_block(block* ptr);
void add_free_block(block* ptr);

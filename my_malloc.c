#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include <limits.h>
#include <pthread.h>
#include"my_malloc.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//the head of the no lock version
__thread block * fhead_nolock = NULL;
//the head of the lock version
block * fhead_lock = NULL;
//a global variable to determine which lock version is running
int lock_version=0;

//lock version
void* ts_malloc_lock(size_t size){
  lock_version=0;
  pthread_mutex_lock(&mutex);
  void* ret = bf_malloc(size);
  pthread_mutex_unlock(&mutex);
  return ret;
}

void ts_free_lock(void *ptr){
  lock_version=0;
  pthread_mutex_lock(&mutex);
  bf_free(ptr);
  pthread_mutex_unlock(&mutex);
}

//no lock version
void* ts_malloc_nolock(size_t size){
  lock_version=1;
  void* ret=bf_malloc(size);
  return ret;
}
void ts_free_nolock(void *ptr){
  lock_version=1;
  bf_free(ptr);
}


void split_block(block * ptr, size_t size) {
  remove_free_block(ptr);
  block * add = (block *)((char *)ptr + size + sizeof(block));
  add->size = ptr->size - sizeof(block) - size;
  ptr->size = size;
  add_free_block(add);
}

//first determine which version is running and use that head pointer,
//after remove function is done, update that version's head pointer
void remove_free_block(block * ptr) {
  block* fhead=NULL;
  if(lock_version==0){
    fhead=fhead_lock;
  }
  else{
    fhead=fhead_nolock;
  }
  if (ptr == fhead) {
    if (ptr->fnext == NULL) {
      fhead = NULL;
    }
    else {
      fhead = ptr->fnext;
    }
  }
  else {
    block * curr = fhead;
    while (curr->fnext != ptr) {
      curr = curr->fnext;
    }
    curr->fnext = ptr->fnext;
  }
  if(lock_version==0){
    fhead_lock=fhead;
  }
  else{
    fhead_nolock=fhead;
  }
}

//first determine which version is running and use that head pointer,                                                                                        
//after add free block function is done, update that version's head pointer 
void add_free_block(block * ptr) {
  block* fhead=NULL;
  if(lock_version==0){
    fhead=fhead_lock;
  }
  else{
    fhead=fhead_nolock;
  }
  if (fhead == NULL) {
    fhead = ptr;
    fhead->fnext = NULL;
  }
  else if (ptr < fhead) {
    ptr->fnext = fhead;
    fhead = ptr;
  }
  else{
    block * current = fhead;
    while (current->fnext != NULL) {
    if ((current < ptr) && (ptr < current->fnext)) {
      block * temp = current->fnext;
      current->fnext = ptr;
      ptr->fnext = temp;
      break;
    }
    current = current->fnext;
  }
  if(current->fnext==NULL){
    current->fnext = ptr;
    ptr->fnext = NULL;
  }
  }
  if(lock_version==0){
    fhead_lock=fhead;
  }
  else{
    fhead_nolock=fhead;
  }
}

//the no lock version need to add a lock here
void * extend_heap(size_t size){
  block* temp=NULL;
  if(lock_version==0){
    temp = sbrk(size + sizeof(block));
  }
  else{
    pthread_mutex_lock(&mutex);
    temp = sbrk(size + sizeof(block));
    pthread_mutex_unlock(&mutex);
  }
    temp->size = size;
    void * ret = (void *)((char *)temp + sizeof(block));
    return ret;
}

void * bf_malloc(size_t size){
  void * ret = bf_find(size);
  if (ret==NULL) {
    ret = extend_heap(size);
  }
  return ret;
}

//determine which lock version is running to choose the head pointer
void * bf_find(size_t size) {
  block* fhead=NULL;
  if(lock_version==0){
    fhead=fhead_lock;
  }
  else{
    fhead=fhead_nolock;
  }
  block * ret = NULL;
  block * current = fhead;
  long long int smallest_diff = 0x7fffffffffffffff; 
  while (current) {
    if ((current->size >= size)) { 
      long long int diff = (long long int)(current->size - size);
      if (diff == 0) { 
        ret = current;
        break;
      }
      if (diff < smallest_diff) {
        smallest_diff = diff;
        ret = current;
      }
    }
    current = current->fnext;
  }
  if(ret==NULL){
    return ret;
  }
  else{
      if ((long long int)(ret->size - size - sizeof(block)) < 0) {
        remove_free_block(ret);
	ret = (void *)((char *)ret + sizeof(block));
        return ret;
      }
      else {
        split_block(ret, size);
	ret = (void *)((char *)ret + sizeof(block));
        return ret;
      }
    remove_free_block(ret);
    ret = (void *)((char *)ret + sizeof(block));
    return ret;
  }
}

void bf_free(void * ptr) {
   block * temp = (void *)((char *)ptr - sizeof(block));
  add_free_block(temp);
  merge_free_block(temp);
}

//determine which lock version is running to choose the head pointer
//do not need to update the head pointer
void merge_free_block(block * ptr){
  block* fhead=NULL;
  if(lock_version==0){
    fhead=fhead_lock;
  }
  else{
    fhead=fhead_nolock;
  }
  block * current = fhead;
  while (current != ptr->fnext) {
    if ((block *)(current->fnext) == (block *)((char *)current + current->size + sizeof(block))) {
      current->size = current->size + sizeof(block) + current->fnext->size;
      current->fnext = current->fnext->fnext;
      if (!current->fnext) {
        break;
      }
      if ((block *)(current->fnext) == (block *)((char *)current + current->size + sizeof(block))) {
        current->size = current->size + sizeof(block) + current->fnext->size;
        current->fnext = current->fnext->fnext;
        break;
      }
      break;
    }
    current = current->fnext;
  }
}



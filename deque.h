#ifndef DEQUE_H
#define DEQUE_H
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
typedef  struct deque_node deque_node_t;
struct deque_node{
    void* data;
    deque_node_t* next;
    deque_node_t* back;
};

typedef  struct deque deque_t;
struct deque{
    size_t size;
    deque_node_t* head;
    deque_node_t* tail;
    pthread_mutex_t mutex; 
};

void deque_create(deque_t* deque);

void deque_purge(deque_t* deque);

void deque_pop_front(deque_t* deque);

void deque_pop_back(deque_t* deque);

void* deque_back(deque_t* deque);

void* deque_front(deque_t* deque);

bool deque_empty(deque_t* deque);

size_t deque_size(deque_t* deque);

int deque_insert_front(deque_t* deque, void* data);

int deque_insert_back(deque_t* deque, void* data);
#endif


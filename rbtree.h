#ifndef RBTREE_H
#define RBTREE_H

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>

typedef void (rbtree_destructor_fn)(void*);
typedef void* (rbtree_duplicator_fn) (const void*);
typedef int (rbtree_comparator_fn) (const void*, const void*);
typedef int (rbtree_serializer_fn) (FILE* , const void*);
typedef int (rbtree_deserializer_fn) (void**, FILE*);

typedef struct rbtree_node rbtree_node_t;

struct rbtree_node {
	void* key;
	void* data;
	bool color;
	rbtree_node_t* left;
	rbtree_node_t* right;
	rbtree_node_t* parent;
};

typedef struct rbtree rbtree_t;

struct rbtree {
	size_t size;
	rbtree_node_t* root;
	rbtree_comparator_fn* key_comparator;
	rbtree_duplicator_fn* key_duplicator;
	rbtree_serializer_fn* key_serializer;
	rbtree_deserializer_fn* key_deserializer;
	rbtree_destructor_fn* key_destructor;
	rbtree_duplicator_fn* data_duplicator;
	rbtree_serializer_fn* data_serializer;
	rbtree_deserializer_fn* data_deserializer;
	rbtree_destructor_fn* data_destructor;
};

struct rbtree_iterator{
	rbtree_t* tree;
	rbtree_node_t* node;
};
typedef struct rbtree_iterator rbtree_iterator_t;


void rbtree_create(rbtree_t* tree);

void* rbtree_search(rbtree_t* tree, const void* key);

int rbtree_insert(rbtree_t* tree, void* key, void* data);

int rbtree_update(rbtree_t* tree, void* key, void* data);

int rbtree_erase(rbtree_t* tree, const void* key);

size_t rbtree_size(rbtree_t* tree);

bool rbtree_empty(rbtree_t* tree);

void rbtree_purge(rbtree_t* tree);


const void* rbtree_iterator_get_key(const rbtree_iterator_t* iterator);

int rbtree_iterator_set_data(const rbtree_iterator_t* iterator);

void* rbtree_iterator_get_data(const rbtree_iterator_t* iterator);


void rbtree_set_key_duplicator (rbtree_t *tree, rbtree_duplicator_fn duplicator);

void rbtree_set_key_comparator (rbtree_t *tree, rbtree_comparator_fn comparator);

void rbtree_set_key_serializer (rbtree_t *tree, rbtree_serializer_fn serializer);

void rbtree_set_key_deserializer (rbtree_t *tree, rbtree_deserializer_fn deserializer);

void rbtree_set_key_destructor (rbtree_t *tree, rbtree_destructor_fn destructor);

void rbtree_set_data_duplicator (rbtree_t *tree, rbtree_duplicator_fn duplicator);

void rbtree_set_data_serializer (rbtree_t *tree, rbtree_serializer_fn serializer);

void rbtree_set_data_deserializer (rbtree_t *tree, rbtree_deserializer_fn deserializer);

void rbtree_set_data_destructor (rbtree_t *tree, rbtree_destructor_fn destructor);

void rbtree_iterator_set(rbtree_t* tree, rbtree_iterator_t* iterator);

bool rbtree_iterator_parking(rbtree_iterator_t* iterator);

void rbtree_iterator_minimum(rbtree_iterator_t* iterator);

void rbtree_iterator_maximum(rbtree_iterator_t* iterator);

void rbtree_iterator_erase_back(rbtree_iterator_t* iterator);

void rbtree_iterator_erase_next(rbtree_iterator_t* iterator);

void rbtree_iterator_next(rbtree_iterator_t* iterator);

void rbtree_iterator_back(rbtree_iterator_t* iterator);

void rbtree_iterator_search(rbtree_iterator_t* iterator, const void* key);

void rbtree_iterator_lower_bound(rbtree_iterator_t* iterator, const void* key);

void rbtree_iterator_upper_bound(rbtree_iterator_t* iterator, const void* key);


/* UNDER DEVELOPMENT
// int rbtree_clone(rbtree_t* destination, rbtree_t* source);
// int rbtree_pack(rbtree_t* tree, const char* filename);
// int rbtree_unpack(rbtree_t* tree, const char* filename);
*/


#endif
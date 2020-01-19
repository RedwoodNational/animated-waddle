#include "rbtree.h"


void rbtree_create(rbtree_t* tree) {
	tree->size = 0;
	tree->root = NULL;
	tree->key_comparator = NULL;
	tree->key_destructor = NULL;
	tree->key_duplicator= NULL;
	tree->data_destructor = NULL;
	tree->data_duplicator = NULL;
}

static inline bool rbtree_rotate_left(rbtree_node_t** node) {
	if ((*node) && (*node)->right) {
		(*node) = (*node)->right;
		(*node)->parent->right = (*node)->left;
		if ((*node)->left) {
			(*node)->left->parent = (*node)->parent;
		}
		(*node)->left = (*node)->parent;
		(*node)->parent = (*node)->left->parent;
		(*node)->left->parent = (*node);
		return true;
	}
	return false;
}

static inline bool rbtree_rotate_right(rbtree_node_t** node) {
	if ((*node) && (*node)->left) {
		(*node) = (*node)->left;
		(*node)->parent->left = (*node)->right;
		if ((*node)->right) {
			(*node)->right->parent = (*node)->parent;
		}
		(*node)->right = (*node)->parent;
		(*node)->parent = (*node)->right->parent;
		(*node)->right->parent = (*node);
		return true;
	}
	return false;
}

static inline void rbtree_erase_core(rbtree_t* tree, rbtree_node_t* garbage){
	rbtree_node_t* parent = garbage->parent;
	if (garbage->left && garbage->right) {
		rbtree_node_t* successor = garbage->right;
		while (successor->left) {
			successor = successor->left;
		}
		if(garbage->color){
			garbage->color=successor->color;
			successor->color=true;
		}
		else{
			garbage->color=successor->color;
			successor->color=false;
		}
		if(parent){
			if(garbage==parent->left){
				parent->left=successor;
			}
			else{
				parent->right=successor;
			}
		}
		else{
			tree->root=successor;
		}
		parent=successor->parent;
		if(successor==parent->left){
			parent->left=garbage;
		}
		else{
			parent->right=garbage;
		}
		if(successor->right){
			successor->parent=garbage->parent;
			successor->left=garbage->left;
			garbage->left=successor->right;
			successor->right=garbage->right;
			garbage->right=garbage->left;
			garbage->parent=parent;
			garbage->right->parent=garbage;
			successor->left->parent=successor;
			successor->right->parent=successor;
			parent=garbage->parent;
			garbage->left=NULL;
		}else{
			successor->parent=garbage->parent;
			successor->left=garbage->left;
			garbage->left=successor->right;
			successor->right=garbage->right;
			garbage->right=garbage->left;
			garbage->parent=parent;
			successor->left->parent=successor;
			successor->right->parent=successor;
			parent=garbage->parent;
			garbage->left=NULL;
		}
	}
	if (garbage->color) {
		if (garbage == parent->left) {
			parent->left = NULL;
		}
		else {
			parent->right = NULL;
		}
	}
	else if (garbage->left && garbage->left->color) {
		if (parent) {
			if (garbage == parent->left) {
				parent->left = garbage->left;
				parent->left->color = false;
				parent->left->parent = parent;
			}
			else {
				parent->right = garbage->left;
				parent->right->color = false;
				parent->right->parent = parent;
			}
		}
		else {
			tree->root = garbage->left;
			tree->root->parent = NULL;
			tree->root->color = false;
		}
	}
	else if (garbage->right && garbage->right->color) {
		if (parent) {
			if (garbage == parent->left) {
				parent->left = garbage->right;
				parent->left->color = false;
				parent->left->parent = parent;
			}
			else {
				parent->right = garbage->right;
				parent->right->color = false;
				parent->right->parent = parent;
			}
		}
		else {
			tree->root = garbage->right;
			tree->root->parent = NULL;
			tree->root->color = false;
		}
	}
	else if (garbage == tree->root) {
		tree->root = NULL;
	}
	else {
		bool unbalanced=true;
		rbtree_node_t* doubleblack = NULL;
		if (garbage == parent->left) {
			parent->left = doubleblack;
		}
		else {
			parent->right = doubleblack;
		}
		while (unbalanced && doubleblack != tree->root) {
			if (doubleblack == parent->left) {
				if (!parent->right->color) {
					if (parent->right->left && parent->right->left->color && !(parent->right->right && parent->right->right->color)) {
						rbtree_rotate_right(&parent->right);
						if (parent->parent) {
							if (parent == parent->parent->left) {
								rbtree_rotate_left(&parent->parent->left);
							}
							else {
								rbtree_rotate_left(&parent->parent->right);
							}
						}else{
							rbtree_rotate_left(&tree->root);
						}
						parent->parent->color = parent->color;
						parent->color = false;
						unbalanced = false;
					}
					else if (parent->right->right && parent->right->right->color) {
						if (parent->parent) {
							if (parent == parent->parent->left) {
								rbtree_rotate_left(&parent->parent->left);
							}
							else {
								rbtree_rotate_left(&parent->parent->right);
							}
						}else{
							rbtree_rotate_left(&tree->root);
						}
						parent->parent->color = parent->color;
						parent->parent->right->color = false;
						parent->color = false;
						unbalanced = false;
					}
					else {
						if (parent->color) {
							unbalanced = false;
							parent->color = false;
						}
						doubleblack = parent;
						parent = parent->parent;
						doubleblack->right->color = true;
					}
				}
				else {
					parent->color = true;
					parent->right->color = false;
					if (parent->parent) {
						if (parent == parent->parent->left) {
							rbtree_rotate_left(&parent->parent->left);
						}
						else {
							rbtree_rotate_left(&parent->parent->right);
						}
					}else{
						rbtree_rotate_left(&tree->root);
					}
				}
			}
			else {
				if (!parent->left->color) {
					if (parent->left->right && parent->left->right->color && !(parent->left->left && parent->left->left->color)) {
						rbtree_rotate_left(&parent->left);
						if (parent->parent) {
							if (parent == parent->parent->left) {
								rbtree_rotate_right(&parent->parent->left);
							}
							else {
								rbtree_rotate_right(&parent->parent->right);
							}
						}else{
							rbtree_rotate_right(&tree->root);
						}
						parent->parent->color = parent->color;
						parent->color = false;
						unbalanced = false;
					}
					else if (parent->left->left && parent->left->left->color) {
						if (parent->parent) {
							if (parent == parent->parent->left) {
								rbtree_rotate_right(&parent->parent->left);
							}
							else {
								rbtree_rotate_right(&parent->parent->right);
							}
						}else{
							rbtree_rotate_right(&tree->root);
						}
						parent->parent->color = parent->color;
						parent->parent->left->color = false;
						parent->color = false;
						unbalanced = false;
					}
					else {
						if (parent->color) {
							unbalanced = false;
							parent->color = false;
						}
						doubleblack = parent;
						parent = parent->parent;
						doubleblack->left->color = true;
					}
				}
				else {
					parent->color = true;
					parent->left->color = false;
					if (parent->parent) {
						if (parent == parent->parent->left) {
							rbtree_rotate_right(&parent->parent->left);
						}
						else {
							rbtree_rotate_right(&parent->parent->right);
						}
					}else{
						rbtree_rotate_right(&tree->root);
					}
				}
			}
		}
	}
	if(tree->key_destructor){
		tree->key_destructor(garbage->key);
	}
	if(tree->data_destructor){
		tree->data_destructor(garbage->data);
	}
	free(garbage);
	--tree->size;
}

static inline void rbtree_insert_core(rbtree_t* tree,  rbtree_node_t* node){
	rbtree_node_t* parent = node->parent;
	while (node != tree->root && parent->color) {
		if (parent == parent->parent->right) {
			if (!parent->parent->left || !parent->parent->left->color) {
				if (node == parent->left) {
					parent = parent->left;
					node = node->parent;
					rbtree_rotate_right(&node->parent->right);
				}
				parent->color = false;
				parent->parent->color = true;
				if (parent->parent->parent) {
					if (parent->parent == parent->parent->parent->right) {
						rbtree_rotate_left(&parent->parent->parent->right);
					}
					else {
						rbtree_rotate_left(&parent->parent->parent->left);
					}
				}
				else {
					rbtree_rotate_left(&tree->root);
				}
			}
			else {
				node = parent->parent;
				parent = node->parent;
				node->color = true;
				node->left->color = false;
				node->right->color = false;
			}
		}
		else {
			if (!parent->parent->right || !parent->parent->right->color) {
				if (node == parent->right) {
					parent = parent->right;
					node = node->parent;
					rbtree_rotate_left(&node->parent->left);
				}
				parent->color = false;
				parent->parent->color = true;
				if (parent->parent->parent) {
					if (parent->parent == parent->parent->parent->right) {
						rbtree_rotate_right(&parent->parent->parent->right);
					}
					else {
						rbtree_rotate_right(&parent->parent->parent->left);
					}
				}
				else {
					rbtree_rotate_right(&tree->root);
				}
			}
			else {
				node = parent->parent;
				parent = node->parent;
				node->color = true;
				node->left->color = false;
				node->right->color = false;
			}
		}
	}
	tree->root->color = false;
}

static inline rbtree_node_t* rbtree_search_core(rbtree_t* tree, const void* key){
	int compare;
	rbtree_node_t* node = tree->root;
	while (node && (compare = tree->key_comparator(node->key, key))) {
		if (compare > 0) {
			node = node->left;
		}
		else {
			node = node->right;
		}
	}
	return node;
}

static inline rbtree_node_t* rbtree_lower_bound_core(rbtree_t* tree, const void* key){
	int compare;
	rbtree_node_t *lower_bound=tree->root, *node=tree->root;
	while(node && (compare=tree->key_comparator(node->key, key))){
		if(compare>0){
			node = node->left;
			lower_bound = node;
		}
		else{
			lower_bound = node;
			node = node->right;
		}
	}
	return node?node:lower_bound;
}

static inline rbtree_node_t* rbtree_upper_bound_core(rbtree_t* tree, const void* key){
	int compare;
	rbtree_node_t *upper_bound=tree->root, *node=tree->root;
	while(node && (compare=tree->key_comparator(node->key, key))){
		if(compare<0){
			node = node->left;
			upper_bound = node;
		}
		else{
			upper_bound = node;
			node = node->right;
		}
	}
	return node?node:upper_bound;
}

static inline rbtree_node_t* rbtree_minimum_core(rbtree_t* tree){
	rbtree_node_t* node = tree->root;
	if(node){
		while(node->left){
			node = node->left;
		}
	}
	return node;
}

static inline rbtree_node_t* rbtree_maximum_core(rbtree_t* tree){
	rbtree_node_t* node = tree->root;
	if(node){
		while(node->right){
			node = node->right;
		}
	}
	return node;
}

static inline rbtree_node_t* rbtree_successor_core(rbtree_node_t* node){
	if(node){
		if(node->right){
			node = node->right;
			while(node->left){
				node = node->left;
			}
		}
		else{
			while(node->parent && node == node->parent->right){
				node = node->parent;
			}
			node = node->parent;
		}
	}
	return node;
}

static inline rbtree_node_t* rbtree_predecessor_core(rbtree_node_t* node){
	if(node){
		if(node->left){
			node = node->left;
			while(node->right){
				node = node->right;
			}
		}
		else{
			while(node->parent && node == node->parent->left){
				node = node->parent;
			}
			node = node->parent;
		}
	}
	return node;
	
}

void rbtree_set_key_duplicator (rbtree_t *tree, rbtree_duplicator_fn duplicator){
	tree->key_duplicator = duplicator;
}

void rbtree_set_key_comparator (rbtree_t *tree, rbtree_comparator_fn comparator){
	tree->key_comparator = comparator;
}

void rbtree_set_key_serializer (rbtree_t *tree, rbtree_serializer_fn serializer){
	tree->key_serializer = serializer;
}

void rbtree_set_key_deserializer (rbtree_t *tree, rbtree_deserializer_fn deserializer){
	tree->key_deserializer = deserializer;
}

void rbtree_set_key_destructor (rbtree_t *tree, rbtree_destructor_fn destructor){
	tree->key_destructor = destructor;
}

void rbtree_set_data_duplicator (rbtree_t *tree, rbtree_duplicator_fn duplicator){
	tree->data_duplicator = duplicator;
}

void rbtree_set_data_serializer (rbtree_t *tree, rbtree_serializer_fn serializer){
	tree->data_serializer = serializer;
}

void rbtree_set_data_deserializer (rbtree_t *tree, rbtree_deserializer_fn deserializer){
	tree->data_deserializer = deserializer;
}

void rbtree_set_data_destructor (rbtree_t *tree, rbtree_destructor_fn destructor){
	tree->data_destructor = destructor;
}



int rbtree_insert(rbtree_t* tree, void* key, void* data) {
	int status, compare;
	rbtree_node_t *parent = NULL, **source = &tree->root;
	while((*source) && (compare = tree->key_comparator((parent = *source)->key, key))){
		if(compare > 0){
			source = &parent->left;
		}
		else{
			source = &parent->right;
		}
	}
	if((*source)){
		status = EEXIST;
	}
	else{
		rbtree_node_t* node=malloc(sizeof(rbtree_node_t));
		if(node){
			int key_check;
			int data_check;
			if(tree->key_duplicator){
				node->key=tree->key_duplicator(key);
				key_check = 0;
			}
			else{
				key_check = 0;
				node->key = key;
			}
			if(tree->data_duplicator){
				node->data=tree->data_duplicator(data);
				data_check = 0;
			}
			else{
				data_check = 0;
				node->data = data;
			}
			if(!key_check && !data_check){
				*source = node;
				node->parent = parent;
				node->left = NULL;
				node->right = NULL;
				node->color = true;
				rbtree_insert_core(tree, node);
				++tree->size;
				status = 0;
			}else{
				if(!key_check && tree->key_destructor){
					tree->key_destructor(node->key);
				}
				if(!data_check && tree->data_destructor){
					tree->data_destructor(node->data);
				}
				free(node);
				status = EFAULT;
			}
		}
		else{
			status = ENOMEM;
		}
	}
	return status;
}

int rbtree_update(rbtree_t* tree, void* key, void* data) {

}


int rbtree_erase(rbtree_t* tree, const void* key) {
	rbtree_node_t* garbage = rbtree_search_core(tree, key);
	return garbage? rbtree_erase_core(tree, garbage),0: ESRCH;
}

void* rbtree_search(rbtree_t* tree, const void* key) {
	rbtree_node_t* node = rbtree_search_core(tree, key);
	return node? node->data: NULL;
}

void rbtree_iterator_set(rbtree_t* tree, rbtree_iterator_t* iterator){
	iterator->tree = tree;
	iterator->node = NULL;
}

void rbtree_iterator_minimum(rbtree_iterator_t* iterator) {
	iterator->node = rbtree_minimum_core(iterator->tree);
}

void rbtree_iterator_maximum(rbtree_iterator_t* iterator) {
	iterator->node = rbtree_maximum_core(iterator->tree);
}

void rbtree_iterator_next(rbtree_iterator_t* iterator) {
	iterator->node = rbtree_successor_core(iterator->node);
}

void rbtree_iterator_back(rbtree_iterator_t* iterator) {
	iterator->node = rbtree_predecessor_core(iterator->node);
}

bool rbtree_iterator_parking(rbtree_iterator_t* iterator) {
	return !iterator->node;
}

void rbtree_iterator_erase_back(rbtree_iterator_t* iterator) {
	rbtree_node_t* garbage = iterator->node;
	iterator->node = rbtree_predecessor_core(iterator->node);
	rbtree_erase_core(iterator->tree, garbage);
}

void rbtree_iterator_erase_next(rbtree_iterator_t* iterator) {
	rbtree_node_t* garbage = iterator->node;
	iterator->node =rbtree_successor_core(iterator->node);
	rbtree_erase_core(iterator->tree, garbage);
}

/*UNDER DEVELOPMENT
int rbtree_iterator_insert(rbtree_iterator_t* iterator, void* key, void* data){

}

int rbtree_iterator_update(rbtree_iterator_t* iterator, void* key, void* data){

}
*/

void rbtree_iterator_search(rbtree_iterator_t* iterator, const void* key){
	iterator->node = rbtree_search_core(iterator->tree, key);	
}

void rbtree_iterator_lower_bound(rbtree_iterator_t* iterator, const void* key){
	iterator->node = rbtree_lower_bound_core(iterator->tree, key);
}

void rbtree_iterator_upper_bound(rbtree_iterator_t* iterator, const void* key){
	iterator->node = rbtree_upper_bound_core(iterator->tree, key);
}

const void* rbtree_iterator_get_key(const rbtree_iterator_t* iterator){
	return iterator->node->key;
}

void* rbtree_iterator_get_data(const rbtree_iterator_t* iterator){
	return iterator->node->data;
}

size_t rbtree_size(rbtree_t* tree){
	return tree->size;
}

bool rbtree_empty(rbtree_t* tree){
	return !tree->root;
}

void rbtree_purge(rbtree_t* tree) {
	while (tree->size>1) {
		while (rbtree_rotate_right(&tree->root));
		tree->root = tree->root->right;
		if(tree->key_destructor){
			tree->key_destructor(tree->root->parent->key);
		}
		if(tree->data_destructor){
			tree->data_destructor(tree->root->parent->data);
		}
		free(tree->root->parent);
		--tree->size;
	}
	if(tree->size==1){
		if(tree->key_destructor){
			tree->key_destructor(tree->root->key);
		}
		if(tree->data_destructor){
			tree->data_destructor(tree->root->data);
		}
		tree->size=0;
		free(tree->root);
		tree->root = NULL;
	}
}
#include "deque.h"

void deque_create(deque_t* deque){
    deque->size=0;
    deque->head=NULL;
    deque->tail=NULL;
    pthread_mutex_init(&deque->mutex, NULL);
}

void deque_purge(deque_t* deque){
    while(deque->head!=deque->tail){
        deque->head=deque->head->next;
        free(deque->head->back);
    }
    free(deque->head);
    deque->head=NULL;
    deque->tail=NULL;
    deque->size=0;
    
}

void deque_pop_front(deque_t* deque){
    pthread_mutex_lock(&deque->mutex);
    if(deque->head){
        if(deque->head==deque->tail){
            free(deque->head);
            deque->head=NULL;
            deque->tail=NULL;
        }
        else{
            deque->head=deque->head->next;
            free(deque->head->back);
            deque->head->back=NULL;
        }
        --deque->size;
    }
    pthread_mutex_unlock(&deque->mutex);
}

void deque_pop_back(deque_t* deque){
    pthread_mutex_lock(&deque->mutex);
    if(deque->tail){
        if(deque->head==deque->tail){
            free(deque->tail);
            deque->head=NULL;
            deque->tail=NULL;
        }
        else{
            deque->tail=deque->tail->back;
            free(deque->tail->next);
            deque->tail->next=NULL;
        }
        --deque->size;
    }
    pthread_mutex_unlock(&deque->mutex);
}

void* deque_back(deque_t* deque){
    pthread_mutex_lock(&deque->mutex);
    void* ejected;
    if(deque->tail){
        ejected=deque->tail->data;
    }
    else{
        ejected=NULL;
    }
    pthread_mutex_unlock(&deque->mutex);
    return ejected;
}

void* deque_front(deque_t* deque){
    pthread_mutex_lock(&deque->mutex);
    void* ejected;
    if(deque->head){
        ejected=deque->head->data;
    }
    else{
        ejected=NULL;
    }
    pthread_mutex_unlock(&deque->mutex);
    return ejected;
}

bool deque_empty(deque_t* deque){
    bool check;
    pthread_mutex_lock(&deque->mutex);
    check=deque->head==NULL;
    pthread_mutex_unlock(&deque->mutex);
    return check;
}

size_t deque_size(deque_t* deque){
    size_t size;
    pthread_mutex_lock(&deque->mutex);
    size=deque->size;
    pthread_mutex_unlock(&deque->mutex);
    return size;
}

int deque_insert_front(deque_t* deque, void* data){
    int status=0;
    pthread_mutex_lock(&deque->mutex);
    if(data){
        deque_node_t* node=(deque_node_t*)malloc(sizeof(deque_node_t));
        if(node){
            node->data=data;
            if(deque->head){
                node->back=NULL;
                node->next=deque->head;
                deque->head->back=node;
                deque->head=node;
            }
            else{
                node->back=NULL;
                node->next=NULL;
                deque->head=node;
                deque->tail=node;
            }
            ++deque->size;
            status=+0;
        }
        else{
            status=-1;
        }
    }
    else{
        status=+1;
    }
    pthread_mutex_unlock(&deque->mutex);
    return status;
}

int deque_insert_back(deque_t* deque, void* data){
    int status=0;
    pthread_mutex_lock(&deque->mutex);
    if(data){
        deque_node_t* node=(deque_node_t*)malloc(sizeof(deque_node_t));
        if(node){
            node->data=data;
            if(deque->tail){
                node->next=NULL;
                node->back=deque->tail;
                deque->tail->next=node;
                deque->tail=node;
            }
            else{
                node->back=NULL;
                node->next=NULL;
                deque->head=node;
                deque->tail=node;
            }
            ++deque->size;
            status=+0;
        }
        else{
            status=-1;
        }
    }
    else{
        status=+1;
    }
    pthread_mutex_unlock(&deque->mutex);
    return status;
}



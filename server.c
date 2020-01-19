
#include "playroom.h"
#include "rbtree.h"
#include <assert.h>
#include "profile.h"
#include "message.h"
#include "deque.h"
#include "claim.h"
#include <string.h>
#include <zmq.h>
#include <unistd.h>
#include <stdlib.h>


int ullcmp(const void* x_key, const void* y_key){
    if(*(unsigned long long*)x_key==*(unsigned long long*)y_key){
        return 0;
    }
    if(*(unsigned long long*)x_key>*(unsigned long long*)y_key){
        return 1;
    }
    return -1;
}

int notify(void* socket, profile_t* profile){
    zmq_msg_t message;
    zmq_msg_init_size(&message, profile->entropy);
    memcpy(zmq_msg_data(&message), profile->socket, profile->entropy);
    zmq_msg_send(&message, socket, ZMQ_SNDMORE);
    zmq_msg_close(&message);

    zmq_msg_init_size(&message, sizeof(int));
    memcpy(zmq_msg_data(&message), &profile->state, sizeof(int));
    zmq_msg_send(&message, socket, 0);
    zmq_msg_close(&message);
}

int suggest(void* socket, profile_t* profile, field_event_t* event){
    zmq_msg_t message;
    zmq_msg_init_size(&message, profile->entropy);
    memcpy(zmq_msg_data(&message), profile->socket, zmq_msg_size(&message));
    zmq_msg_send(&message, socket, ZMQ_SNDMORE);
    zmq_msg_close(&message);

    zmq_msg_init_size(&message, sizeof(field_event_t));
    memcpy(zmq_msg_data(&message), event, zmq_msg_size(&message));
    zmq_msg_send(&message, socket, 0);
    zmq_msg_close(&message);
}

int reveal(void* socket, profile_t* profile, aiming_t* aiming){
    zmq_msg_t message;
    zmq_msg_init_size(&message, profile->entropy);
    memcpy(zmq_msg_data(&message), profile->socket, zmq_msg_size(&message));
    zmq_msg_send(&message, socket, ZMQ_SNDMORE);
    zmq_msg_close(&message);

    zmq_msg_init_size(&message, sizeof(aiming_t));
    memcpy(zmq_msg_data(&message), aiming, sizeof(aiming_t));
    zmq_msg_send(&message, socket, 0);
    zmq_msg_close(&message);
}



#define CLEANUP_FREQUENCY 10

#define INNACTIVE_TIMEOUT 24*60*60

#define GAME_TIMEOUT 300


#define GAME_SERVER_ADDRESS "tcp://192.168.1.38:4040"
#define AUTH_SERVER_ADDRESS "tcp://192.168.1.38:1234"


struct database{
    rbtree_t profiles[2];
    rbtree_t playrooms[2];
    deque_t playrooms_playing;
    deque_t profiles_playing;
    deque_t small;
    deque_t middle;
    deque_t large;
    pthread_mutex_t mutex;
};
typedef struct database database_t;

int accept_ship(void* socket, ship_t* ship){
    int feedback;
    zmq_msg_t message;
    feedback=zmq_msg_init(&message);
    if(feedback!=0){
        zmq_msg_close(&message);
        return feedback;
    }
    feedback=zmq_msg_recv(&message, socket, 0);
    if(feedback == -1){
        zmq_msg_close(&message);
        return feedback;
    }
    if(feedback != sizeof(ship_t)){
        zmq_msg_close(&message);
        return feedback;
    }
    memcpy(ship, zmq_msg_data(&message), sizeof(ship_t));
    feedback=zmq_msg_close(&message);
    if(feedback != 0){
        zmq_msg_close(&message);
        return feedback;
    }
    return feedback;
}


void* game_service(void* routine){
    int length;
    int state;
    int feedback;
    char identity[512];
    auth_t auth;
    size_t size;
    aiming_t hint;
    field_event_t event;
    field_location_t location;
    ship_t ship;
    claim_t* xclaim;
    claim_t* gclaim;
    profile_t* gprofile;
    profile_t* xprofile;
    playroom_t* playroom;
    action_t action;
    status_t status;
    zmq_msg_t message;
    database_t* database=(database_t*)routine;

    void* context=zmq_ctx_new();
    void* socket=zmq_socket(context, ZMQ_ROUTER);
    zmq_bind(socket, GAME_SERVER_ADDRESS);
   
    while(true){
        zmq_pollitem_t item = {socket,   0, ZMQ_POLLIN, 0};
        zmq_poll (&item, 1, 1);
        if (item.revents & ZMQ_POLLIN) {
            feedback=zmq_msg_init(&message);
            if(feedback!=0){
                zmq_msg_close(&message);
                continue;
            }
            length=zmq_msg_recv(&message, socket, 0);
            if(length==-1){
                zmq_msg_close(&message);
                continue;
            }
            memcpy(identity, zmq_msg_data(&message), zmq_msg_size(&message));
            feedback=zmq_msg_close(&message);
            if(feedback!=0){
                zmq_msg_close(&message);
                continue;
            }

            feedback=zmq_msg_init(&message);
            if(feedback!=0){
                zmq_msg_close(&message);
                continue;
            }
            feedback=zmq_msg_recv(&message, socket, 0);
            if(feedback==-1){
                zmq_msg_close(&message);
                continue;  
            }
            if(feedback!=sizeof(auth_t)){
                zmq_msg_close(&message);
                continue;
            }
            memcpy(&auth, zmq_msg_data(&message), zmq_msg_size(&message));
            feedback=zmq_msg_close(&message);
            if(feedback!=0){
                zmq_msg_close(&message);
                continue;   
            }

            feedback=zmq_msg_init(&message);
            if(feedback!=0){
                zmq_msg_close(&message);
                continue;
            }
            feedback=zmq_msg_recv(&message, socket, 0);
            if(feedback==-1){
                zmq_msg_close(&message);
                continue;  
            }
            if(feedback!=sizeof(action_t)){
                zmq_msg_close(&message);
                continue;
            }
            memcpy(&action, zmq_msg_data(&message), zmq_msg_size(&message));
            feedback=zmq_msg_close(&message);
            if(feedback!=0){
                zmq_msg_close(&message);
                continue;   
            }
            pthread_mutex_lock(&database->mutex);
            xprofile=rbtree_search(&database->profiles[0], &auth.uid);
            if(xprofile && xprofile->cookie==auth.cookie){
                xprofile->entropy=length;
                time(&xprofile->trace);
                memcpy(xprofile->socket, identity, length);
                switch(action){
                    case START:
                        if(xprofile->state==INQUEUE){
                            xprofile->claim->validation = INVALID;
                        }
                        feedback=zmq_msg_init(&message);
                        if(feedback!=0){
                            pthread_mutex_unlock(&database->mutex);
                            zmq_msg_close(&message);
                            continue;
                        }
                        feedback=zmq_msg_recv(&message, socket, 0);
                        if(feedback==-1){
                            pthread_mutex_unlock(&database->mutex);
                            zmq_msg_close(&message);
                            continue;  
                        }
                        if(feedback!=sizeof(size_t)){
                            pthread_mutex_unlock(&database->mutex);
                            zmq_msg_close(&message);
                            continue;
                        }
                        memcpy(&size, zmq_msg_data(&message), zmq_msg_size(&message));
                        feedback=zmq_msg_close(&message);
                        while(!deque_empty(&database->small) && ((claim_t*)deque_front(&database->small))->validation == INVALID){
                            deque_pop_front(&database->small);
                        }
                        while(!deque_empty(&database->middle) && ((claim_t*)deque_front(&database->middle))->validation == INVALID){
                            deque_pop_front(&database->middle);
                        }
                        while(!deque_empty(&database->large) && ((claim_t*)deque_front(&database->large))->validation == INVALID){
                            deque_pop_front(&database->large);
                        }
                        
                        gprofile = NULL;
                        switch (size){
                            case SMALL:
                                if(!deque_empty(&database->small)){
                                    gclaim = deque_front(&database->small);
                                    deque_pop_front(&database->small);
                                    gprofile=rbtree_search(&database->profiles[0], &gclaim->profile);
                                    free(gclaim);
                                }
                                if(gprofile){
                                    playroom=playroom_create(SMALL);
                                    playroom->gid=rand();
                                    playroom->i_profile=xprofile->uid;
                                    time(&playroom->o_trace);
                                    playroom->o_profile=gprofile->uid;
                                    time(&playroom->i_trace);
                                    rbtree_insert(&database->playrooms[0], &playroom->gid, playroom);
                                    deque_insert_back(&database->playrooms_playing, playroom);
                                    ++xprofile->games;
                                    xprofile->state=CHECKIN;
                                    xprofile->gid=playroom->gid;
                                    notify(socket, xprofile);
                                    ++gprofile->games;
                                    gprofile->state=CHECKIN;
                                    gprofile->gid=playroom->gid;
                                    notify(socket, gprofile);
                                }
                                else{
                                    xclaim = malloc(sizeof(claim_t));
                                    xprofile->claim = xclaim;
                                    xclaim->validation = VALID;
                                    xprofile->state = INQUEUE;
                                    xclaim->profile=xprofile->uid;
                                    deque_insert_back(&database->small, xclaim); 
                                    notify(socket, xprofile);                                                             
                                }
                                break;

                            case NORMAL:
                                if(!deque_empty(&database->middle)){
                                    gclaim = deque_front(&database->middle);
                                    deque_pop_front(&database->middle);
                                    gprofile=rbtree_search(&database->profiles[0], &gclaim->profile);
                                    free(gclaim);
                                }
                                if(gprofile){
                                    playroom=playroom_create(NORMAL);
                                    playroom->gid=rand();
                                    playroom->i_profile=xprofile->uid;
                                    time(&playroom->i_trace);
                                    playroom->o_profile=gprofile->uid;
                                    time(&playroom->o_trace);
                                    rbtree_insert(&database->playrooms[0], &playroom->gid, playroom);
                                    deque_insert_back(&database->playrooms_playing, playroom);
                                    ++xprofile->games;
                                    xprofile->state=CHECKIN;
                                    xprofile->gid=playroom->gid;
                                    notify(socket, xprofile);
                                    ++gprofile->games;
                                    gprofile->state=CHECKIN;
                                    gprofile->gid=playroom->gid;
                                    notify(socket, gprofile);
                                }
                                else{
                                    xclaim = malloc(sizeof(claim_t));
                                    xprofile->claim = xclaim;
                                    xclaim->validation = VALID;
                                    xprofile->state = INQUEUE;
                                    xclaim->profile=xprofile->uid;
                                    deque_insert_back(&database->middle, xclaim);                                                              
                                    notify(socket, xprofile);
                                }
                                break;

                            case LARGE:
                                if(!deque_empty(&database->large)){
                                    gclaim = deque_front(&database->large);
                                    deque_pop_front(&database->large);
                                    gprofile=rbtree_search(&database->profiles[0], &gclaim->profile);
                                    free(gclaim);
                                }
                                if(gprofile){
                                    playroom=playroom_create(LARGE);
                                    playroom->gid=rand();
                                    playroom->i_profile=xprofile->uid;
                                    time(&playroom->i_trace);
                                    playroom->o_profile=gprofile->uid;
                                    time(&playroom->o_trace);
                                    rbtree_insert(&database->playrooms[0], &playroom->gid, playroom);
                                    deque_insert_back(&database->playrooms_playing, playroom);
                                    ++xprofile->games;
                                    xprofile->state=CHECKIN;
                                    xprofile->gid=playroom->gid;
                                    notify(socket, xprofile);
                                    ++gprofile->games;
                                    gprofile->state=CHECKIN;
                                    gprofile->gid=playroom->gid;
                                    notify(socket, gprofile);
                                }
                                else{
                                    xclaim = malloc(sizeof(claim_t));
                                    xprofile->claim = xclaim;
                                    xclaim->validation = VALID;
                                    xprofile->state = INQUEUE;
                                    xclaim->profile=xprofile->uid;
                                    deque_insert_back(&database->large, xclaim);
                                    notify(socket, xprofile);                                                           
                                }
                                break;
                        }
                        break;

                    case NAVY:
                        if(xprofile->state == CHECKIN){
                            playroom=rbtree_search(&database->playrooms[0], &xprofile->gid);
                            if(xprofile->uid==playroom->i_profile){
                                gprofile=rbtree_search(&database->profiles[0], &playroom->o_profile);
                                time(&playroom->i_trace);
                                switch (playroom->scale){
                                    case SMALL:
                                        for(int i=0; i<SMALL_ONESQUARES; ++i){
                                            playroom->i_remaining+=1;
                                            accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->i_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->i_navyfield, &ship);
                                        }
                                        for(int i=0; i<SMALL_TWOSQUARES; ++i){
                                            playroom->i_remaining+=2;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->i_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->i_navyfield, &ship);
                                        }
                                        for(int i=0; i<SMALL_THREESQUARES; ++i){
                                            playroom->i_remaining+=3;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->i_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->i_navyfield, &ship);
                                        }
                                        for(int i=0; i<SMALL_FOURSQUARES; ++i){
                                            playroom->i_remaining+=4;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->i_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->i_navyfield, &ship);
                                        }
                                        break;

                                    case NORMAL:
                                        for(int i=0; i<NORMAL_ONESQUARES; ++i){
                                            playroom->i_remaining+=1;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->i_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->i_navyfield, &ship);
                                        }
                                        for(int i=0; i<NORMAL_TWOSQUARES; ++i){
                                            playroom->i_remaining+=2;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->i_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->i_navyfield, &ship);
                                        }
                                        for(int i=0; i<NORMAL_THREESQUARES; ++i){
                                            playroom->i_remaining+=3;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->i_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->i_navyfield, &ship);
                                        }
                                        for(int i=0; i<NORMAL_FOURSQUARES; ++i){
                                            playroom->i_remaining+=4;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->i_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->i_navyfield, &ship);                                       
                                        }
                                        break;

                                    case LARGE:
                                        for(int i=0; i<LARGE_ONESQUARES; ++i){
                                            playroom->i_remaining+=1;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->i_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->i_navyfield, &ship);

                                        }
                                        for(int i=0;  i<LARGE_TWOSQUARES; ++i){
                                            playroom->i_remaining+=2;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->i_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->i_navyfield, &ship);

                                        }
                                        for(int i=0;  i<LARGE_THREESQUARES; ++i){
                                            playroom->i_remaining+=3;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->i_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->i_navyfield, &ship);

                                        }
                                        for(int i=0;  i<LARGE_FOURSQUARES; ++i){
                                            playroom->i_remaining+=4;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->i_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->i_navyfield, &ship);
                                        }
                                        break;
                                }
                                if(!feedback){
                                    if(gprofile->state==CHECKIN){
                                        xprofile->state=READY;
                                    }else{
                                        if(rand()%2){
                                            if(rand()%20 == rand()%20){
                                                gprofile->state=REVEAL;
                                            }else{
                                                gprofile->state=PLAYING;
                                            }
                                            xprofile->state=WAITING;
                                        }else{
                                            xprofile->state=PLAYING;
                                            if(rand()%20 == rand()%20){
                                                xprofile->state=REVEAL;
                                            }else{
                                                xprofile->state=PLAYING;
                                            }
                                            gprofile->state=WAITING;
                                        }
                                        notify(socket, gprofile);
                                    }
                                }else{
                                    field_clear(&playroom->i_navyfield);
                                }
                                notify(socket, xprofile);
                            }
                            else{
                                gprofile=rbtree_search(&database->profiles[0], &playroom->i_profile);
                                time(&playroom->o_trace);
                                switch (playroom->scale){
                                    case SMALL:
                                        for(int i=0;  i<SMALL_ONESQUARES; ++i){
                                            playroom->o_remaining+=1;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->o_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->o_navyfield, &ship);
                                        }
                                        for(int i=0;  i<SMALL_TWOSQUARES; ++i){
                                            playroom->o_remaining+=2;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->o_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->o_navyfield, &ship);
                                        }
                                        for(int i=0;  i<SMALL_THREESQUARES; ++i){
                                            playroom->o_remaining+=3;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->o_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->o_navyfield, &ship);
                                        }
                                        for(int i=0;  i<SMALL_FOURSQUARES; ++i){
                                            playroom->o_remaining+=4;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->o_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->o_navyfield, &ship);
                                        }
                                        break;

                                    case NORMAL:
                                        for(int i=0;  i<NORMAL_ONESQUARES; ++i){
                                            playroom->o_remaining+=1;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->o_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->o_navyfield, &ship);
                                        }
                                        for(int i=0;  i<NORMAL_TWOSQUARES; ++i){
                                            playroom->o_remaining+=2;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->o_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->o_navyfield, &ship);

                                        }
                                        for(int i=0;  i<NORMAL_THREESQUARES; ++i){
                                            playroom->o_remaining+=3;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->o_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->o_navyfield, &ship);
                                        }
                                        for(int i=0;  i<NORMAL_FOURSQUARES; ++i){
                                            playroom->o_remaining+=4;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->o_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->o_navyfield, &ship);
                                        }
                                        break;


                                    case LARGE:
                                        for(int i=0;  i<LARGE_ONESQUARES; ++i){
                                            playroom->o_remaining+=1;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->o_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->o_navyfield, &ship);
                                        }
                                        for(int i=0;  i<LARGE_TWOSQUARES; ++i){
                                            playroom->o_remaining+=2;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->o_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->o_navyfield, &ship);

                                        }
                                        for(int i=0;  i<LARGE_THREESQUARES; ++i){
                                            playroom->o_remaining+=3;
                                            feedback=accept_ship(socket, &ship);
                                            navyfield_check_ship_raw(&playroom->o_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->o_navyfield, &ship);

                                        }
                                        for(int i=0;  i<LARGE_FOURSQUARES; ++i){
                                            playroom->o_remaining+=4;
                                            feedback=accept_ship(socket, &ship); 
                                            navyfield_check_ship_raw(&playroom->o_navyfield, &ship);
                                            navyfield_locate_ship_raw(&playroom->o_navyfield, &ship);
                                        }
                                        break;
                                }
                                if(!feedback){
                                    if(gprofile->state==CHECKIN){
                                        xprofile->state=READY;
                                    }else{
                                        if(rand()%2){
                                            if(rand()%20 == rand()%20){
                                                gprofile->state=REVEAL;
                                            }else{
                                                gprofile->state=PLAYING;
                                            }
                                            xprofile->state=WAITING;
                                        }else{
                                            xprofile->state=PLAYING;
                                            if(rand()%20 == rand()%20){
                                                xprofile->state=REVEAL;
                                            }else{
                                                xprofile->state=PLAYING;
                                            }
                                            gprofile->state=WAITING;
                                        }
                                        notify(socket, gprofile);
                                    }
                                }else{
                                    field_clear(&playroom->o_navyfield);
                                }
                                notify(socket, xprofile);
                            }
                        }
                        break;

                    case STRIKE:
                        if(xprofile->state==PLAYING){
                            zmq_msg_init(&message);
                            zmq_msg_recv(&message, socket, 0);
                            memcpy(&location, zmq_msg_data(&message), zmq_msg_size(&message));
                            zmq_msg_close(&message);
                            playroom = rbtree_search(&database->playrooms[0], &xprofile->gid);
                            if(xprofile->uid == playroom->i_profile){
                                time(&playroom->i_trace);
                                gprofile = rbtree_search(&database->profiles[0], &playroom->o_profile);
                            }else{
                                time(&playroom->o_trace);
                                gprofile = rbtree_search(&database->profiles[0], &playroom->i_profile);
                            }
                            playroom_strike(playroom, xprofile->uid, location);
                            xprofile->state = SYNCSTRIKE;
                            gprofile->state = SYNCNAVY;
                            notify(socket, xprofile);
                            notify(socket, gprofile);
                            playroom_update_strikefield(playroom, xprofile->uid, location, &event);
                            if(event.state==SHOT){
                                if(rand()%20==rand()%20){
                                    xprofile->state=REVEAL;
                                }else{
                                    xprofile->state=PLAYING;
                                }
                                gprofile->state=WAITING;
                                ++xprofile->shots;
                            }else{
                                if(rand()%20==rand()%20){
                                    gprofile->state=REVEAL;
                                }else{
                                    gprofile->state=PLAYING;
                                }
                                xprofile->state=WAITING;
                                ++xprofile->misses;
                            }
                            if(xprofile->misses!=0){
                                xprofile->ratio=(double)xprofile->shots/xprofile->misses;
                            }
                            if(gprofile->misses!=0){
                                gprofile->ratio=(double)gprofile->shots/gprofile->misses;
                            }
                            suggest(socket, xprofile, &event);
                            playroom_update_navyfield(playroom, gprofile->uid, location, &event);
                            suggest(socket, gprofile, &event);
                            if(playroom_played(playroom)){
                                ++xprofile->wins;
                                ++gprofile->loses;
                                xprofile->state = WINNER;
                                gprofile->state = LOSER;
                            }
                            notify(socket, xprofile);
                            notify(socket, gprofile);
                        }
                        break;

                    case CHOOSE:
                        if(xprofile->state==REVEAL){
                            playroom = rbtree_search(&database->playrooms[0], &xprofile->gid);
                            if(xprofile->uid==playroom->i_profile){
                                time(&playroom->i_trace);
                                gprofile=rbtree_search(&database->profiles[0], &playroom->o_profile);
                            }else{
                                time(&playroom->o_trace);
                                gprofile=rbtree_search(&database->profiles[0], &playroom->i_profile);
                            }
                            zmq_msg_init(&message);
                            zmq_msg_recv(&message, socket, 0);
                            memcpy(&state, zmq_msg_data(&message), zmq_msg_size(&message));
                            zmq_msg_close(&message);
                            switch (state){
                                case 1:
                                    xprofile->state=PLAYING;
                                    gprofile->state=WAITING;
                                    notify(socket, gprofile);
                                    notify(socket, xprofile);
                                    break;
                                case 0:
                                    zmq_msg_init(&message);
                                    zmq_msg_recv(&message, socket, 0);
                                    memcpy(&location, zmq_msg_data(&message), zmq_msg_size(&message));
                                    zmq_msg_close(&message);
                                    navyfield_aiming(playroom, xprofile->uid, &location, &hint);
                                    xprofile->state=SYNCHINT;
                                    notify(socket, xprofile);
                                    reveal(socket, xprofile, &hint);
                                    gprofile->state=REVEALED;
                                    notify(socket, gprofile);
                                    xprofile->state=WAITING;
                                    notify(socket, xprofile);
                                    gprofile->state=PLAYING;
                                    notify(socket, gprofile);
                                    break;
                            }
                        }
                }

            }else{
                status=UNAUTHORIZED;
                feedback=zmq_msg_init_size(&message, length);
                if(feedback!=0){
                    zmq_msg_close(&message);
                    pthread_mutex_unlock(&database->mutex);
                    continue;
                }
                memcpy(zmq_msg_data(&message), identity, length);
                feedback=zmq_msg_send(&message, socket, ZMQ_SNDMORE);
                if(feedback==-1){
                    zmq_msg_close(&message);
                    pthread_mutex_unlock(&database->mutex);
                    continue;
                }
                feedback=zmq_msg_close(&message);
                if(feedback!=0){
                    zmq_msg_close(&message);
                    pthread_mutex_unlock(&database->mutex);
                    continue;
                }
                feedback=zmq_msg_init_size(&message, 1);
                if(feedback!=0){
                    zmq_msg_close(&message);
                    pthread_mutex_unlock(&database->mutex);
                    continue;
                }
                memset(zmq_msg_data(&message), 0, 1);
                feedback=zmq_msg_send(&message, socket, ZMQ_SNDMORE);
                if(feedback==-1){
                    zmq_msg_close(&message);
                    pthread_mutex_unlock(&database->mutex);
                    continue;
                }
                feedback=zmq_msg_close(&message);
                if(feedback!=0){
                    zmq_msg_close(&message);
                    pthread_mutex_unlock(&database->mutex);
                    continue;
                }
                feedback=zmq_msg_init_size(&message, sizeof(status_t));
                if(feedback!=0){
                    zmq_msg_close(&message);
                    pthread_mutex_unlock(&database->mutex);
                    continue;
                }
                memcpy(zmq_msg_data(&message), &status, zmq_msg_size(&message));
                feedback=zmq_msg_send(&message, socket, 0);
                if(feedback==-1){
                    zmq_msg_close(&message);
                    pthread_mutex_unlock(&database->mutex);
                    continue;
                }
                feedback=zmq_msg_close(&message);
                if(feedback!=0){
                    zmq_msg_close(&message);
                    pthread_mutex_unlock(&database->mutex);
                    continue;
                }
            }
            pthread_mutex_unlock(&database->mutex);
        }
        while(!deque_empty(&database->small) && ((claim_t*)deque_front(&database->small))->validation == INVALID){
            deque_pop_front(&database->small);
        }
        while(!deque_empty(&database->middle) && ((claim_t*)deque_front(&database->middle))->validation == INVALID){
            deque_pop_front(&database->middle);
        }
        while(!deque_empty(&database->large) && ((claim_t*)deque_front(&database->large))->validation == INVALID){
            deque_pop_front(&database->large);
        }           
        int total=deque_size(&database->playrooms_playing);
        for(int i=0; i<total; ++i){
            playroom=deque_front(&database->playrooms_playing);
            deque_pop_front(&database->playrooms_playing);
            if(difftime(time(NULL), playroom->i_trace)>GAME_TIMEOUT || difftime(time(NULL), playroom->o_trace)>GAME_TIMEOUT){
                xprofile=rbtree_search(&database->profiles[0], &playroom->i_profile);
                if(xprofile->state==WAITING){
                    ++xprofile->wins;
                }else{
                    ++xprofile->loses;
                }
                xprofile->state=TIMEOUT;
                notify(socket, xprofile);
                gprofile=rbtree_search(&database->profiles[0], &playroom->o_profile);
                if(gprofile->state==WAITING){
                    ++gprofile->wins;
                }else{
                    ++gprofile->loses;
                }
                gprofile->state=TIMEOUT;
                notify(socket, gprofile);
                rbtree_erase(&database->playrooms[0], &playroom->gid);
                playroom_purge(playroom);
            }else{
                deque_insert_back(&database->playrooms_playing, playroom);
            }
        }
    }
}


void* signup_service(void* routine){
    auth_t auth;
    action_t action;
    status_t status;
    zmq_msg_t message;
    profile_t* profile;
    database_t* database=(database_t*) routine;

    rbtree_iterator_t iterator;
    rbtree_iterator_set(&database->profiles[1], &iterator);

    int feedback;
    void* context = zmq_ctx_new();
    assert(context!=NULL);
    void* socket = zmq_socket(context, ZMQ_REP);
    assert(socket!=NULL);
    feedback=zmq_bind(socket, AUTH_SERVER_ADDRESS);
    assert(feedback==0);
    while(true){
        feedback=zmq_msg_init(&message);
        assert(feedback==0);
        feedback=zmq_msg_recv(&message, socket, 0);
        assert(feedback==sizeof(action_t));
        memcpy(&action, zmq_msg_data(&message), sizeof(action_t));
        feedback=zmq_msg_close(&message);
        assert(feedback==0);
        pthread_mutex_lock(&database->mutex);
        switch(action){
            case SIGNIN:
                feedback=zmq_msg_init(&message);
                assert(feedback==0);
                feedback=zmq_msg_recv(&message, socket, 0);
                assert(feedback==sizeof(auth_t));
                memcpy(&auth, zmq_msg_data(&message), sizeof(auth_t));
                feedback=zmq_msg_close(&message);
                assert(feedback==0);
                profile = rbtree_search(&database->profiles[0], &auth.uid);
                if(profile && profile->cookie==auth.cookie){
                    status=AUTHORIZED;
                    feedback=zmq_msg_init_size(&message, sizeof(status_t));
                    assert(feedback==0);
                    memcpy(zmq_msg_data(&message),& status, sizeof(status_t));
                    feedback=zmq_msg_send(&message, socket, ZMQ_SNDMORE);
                    assert(feedback!=-1);
                    feedback=zmq_msg_close(&message);
                    assert(feedback==0);
                    feedback=zmq_msg_init_size(&message, sizeof(profile_t));
                    assert(feedback==0);
                    memcpy(zmq_msg_data(&message), profile, sizeof(profile_t));
                    feedback=zmq_msg_send(&message, socket, 0);
                    assert(feedback!=-1);
                    feedback=zmq_msg_close(&message);
                    assert(feedback==0);
                    time(&profile->trace);
                    printf("[SIGNUP SERVICE][PROFILE: %lu][COOKIE: %lu]:- Profile restored\n", auth.uid, auth.cookie);
                }
                else{
                    status=UNAUTHORIZED;
                    feedback=zmq_msg_init_size(&message, sizeof(status_t));
                    assert(feedback==0);
                    memcpy(zmq_msg_data(&message), &status, sizeof(status_t));
                    feedback=zmq_msg_send(&message, socket, 0);
                    assert(feedback==sizeof(status_t));
                    feedback=zmq_msg_close(&message);
                    assert(feedback==0);
                    printf("[SIGNUP SERVICE][PROFILE: %lu][COOKIE: %lu]:- Authorization failed\n", auth.uid, auth.cookie);
                }
                break;
            case SIGNUP:
                if(rbtree_empty(&database->profiles[1])){
                    auth.cookie=rand();
                    auth.uid=rbtree_size(&database->profiles[0]);
                    profile=profile_create(auth.uid, auth.cookie);
                }else{
                    rbtree_iterator_minimum(&iterator);
                    profile=rbtree_iterator_get_data(&iterator);
                    rbtree_iterator_erase_next(&iterator);
                    profile->cookie=rand();
                }
                rbtree_insert(&database->profiles[0], &profile->uid, profile);
                time(&profile->trace);
                feedback=zmq_msg_init_size(&message, sizeof(auth_t));
                memcpy(zmq_msg_data(&message), &auth, sizeof(auth_t));
                feedback=zmq_msg_send(&message, socket, ZMQ_SNDMORE);
                feedback=zmq_msg_close(&message);
                feedback=zmq_msg_init_size(&message, sizeof(profile_t));
                memcpy(zmq_msg_data(&message), profile, sizeof(profile_t));
                feedback=zmq_msg_send(&message, socket, 0);
                feedback=zmq_msg_close(&message);
                printf("[SIGNUP SERVICE][PROFILE: %lu][COOKIE: %lu]:- New profile created\n", auth.uid, auth.cookie);
                break;
        }
        pthread_mutex_unlock(&database->mutex);
    }
    zmq_unbind(socket, AUTH_SERVER_ADDRESS);
    zmq_close(socket);
    zmq_ctx_destroy (&context);
}

void* clean_service(void* routine){
    profile_t* profile;
    database_t* database=(database_t*) routine;
    rbtree_iterator_t blades[2];
    rbtree_iterator_set(&database->profiles[0], &blades[0]);
    rbtree_iterator_set(&database->profiles[1], &blades[1]);
    while(true){
        sleep(CLEANUP_FREQUENCY);
        pthread_mutex_lock(&database->mutex);
        printf("[CLEANUP SERVICE][%lu]:- CLEANUP STARTED\n", rbtree_size(&database->profiles[0]));

        rbtree_iterator_minimum(&blades[0]);
        while(!rbtree_iterator_parking(&blades[0])){
            profile=rbtree_iterator_get_data(&blades[0]);
            if(difftime(time(NULL), profile->trace)>INNACTIVE_TIMEOUT){
                rbtree_iterator_erase_next(&blades[0]);
                rbtree_insert(&database->profiles[1], &profile->uid, profile);
            }else{
                rbtree_iterator_next(&blades[0]);
            }
        }
        rbtree_iterator_maximum(&blades[0]);
        if(rbtree_iterator_parking(&blades[0])){
            rbtree_iterator_minimum(&blades[1]);
        }else{
            profile=rbtree_iterator_get_data(&blades[0]);
            rbtree_iterator_upper_bound(&blades[1], &profile->uid);
        }
        while(!rbtree_iterator_parking(&blades[1])){
            free(rbtree_iterator_get_data(&blades[1]));
            rbtree_iterator_erase_next(&blades[1]);
        }
        printf("[CLEANUP SERVICE][%lu]:- CLEANUP FINISHED\n",rbtree_size(&database->profiles[0]));
        pthread_mutex_unlock(&database->mutex);
    }
}








int main(){
    srand(time(NULL));
    database_t database;
    pthread_t signup_thread;
    pthread_t game_thread;
    pthread_mutex_init(&database.mutex, NULL);

    deque_create(&database.playrooms_playing);
    deque_create(&database.profiles_playing);

    deque_create(&database.small);
    deque_create(&database.middle);
    deque_create(&database.large);
    

    rbtree_create(&database.profiles[0]);
    rbtree_set_key_comparator(&database.profiles[0], ullcmp);
    rbtree_create(&database.profiles[1]);
    rbtree_set_key_comparator(&database.profiles[1], ullcmp);
    rbtree_create(&database.playrooms[0]);
    rbtree_set_key_comparator(&database.playrooms[0], ullcmp);


    pthread_create(&game_thread, NULL, game_service, &database);
    //pthread_create(&clean_thread, NULL, clean_service, &database);
    pthread_create(&signup_thread, NULL, signup_service, &database);

    pthread_join(game_thread, NULL);
    pthread_join(signup_thread, NULL);
    pthread_join(game_thread, NULL);
    
}
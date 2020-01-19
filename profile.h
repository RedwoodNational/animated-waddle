#ifndef PLAYER_H
#define PLAYER_H

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "claim.h"
#include "zmq.h"

#define INQUEUE 1
#define WINNER 7
#define LOSER 8
#define TIMEOUT 12
#define SYNCNAVY 10
#define SYNCHINT 13
#define REVEALED 14
#define SYNCSTRIKE 11
#define PLAYING 2
#define WAITING 3
#define FINISHED 4
#define CHECKIN 5
#define READY 6
#define REVEAL 9

struct profile{
    int state;
    size_t gid;
    size_t uid;
    time_t trace;
    size_t cookie;
    double ratio;
    unsigned wins;
    unsigned loses;
    unsigned games;
    unsigned shots;
    unsigned misses;
    claim_t* claim;
    char socket[512];
    size_t entropy;
};
typedef struct profile profile_t;

profile_t* profile_create(size_t uid, size_t cookie);

void profile_update(profile_t* profile);

void profile_reset(profile_t* profile);

void profile_purge(profile_t* profile);

#endif
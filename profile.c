#include "profile.h"



profile_t* profile_create(size_t uid, size_t cookie){
    profile_t* profile=malloc(sizeof(profile_t));
    profile->uid=uid;
    profile->cookie=cookie;
    profile->wins=0;
    profile->games=0;
    profile->loses=0;
    profile->ratio=1;
    profile->shots=0;
    profile->misses=0;
    return profile;
}

void profile_update(profile_t* profile){
    profile->trace=time(NULL);
}

void profile_reset(profile_t* profile){
    profile->wins=0;
    profile->games=0;
    profile->loses=0;
    profile->ratio=0;
    profile->shots=0;
}

void profile_connect(profile_t* profile, unsigned long long gid){
}

void profile_disconnect(profile_t* profile){

}

void profile_purge(profile_t* profile){
    free(profile);
}

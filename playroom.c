#include "playroom.h"
#include <stdlib.h>



void* playroom_create(size_t scale){
    playroom_t* playroom=(playroom_t*)malloc(sizeof(playroom_t));
    playroom->scale=scale;
    playroom->i_navyfield.matrix=calloc(scale*scale, sizeof(field_event_t));
    playroom->i_navyfield.scale=scale;
    playroom->i_strikefield.matrix=calloc(scale*scale, sizeof(field_event_t));
    playroom->i_strikefield.scale=scale;
    playroom->i_remaining=0;
    playroom->o_navyfield.matrix=calloc(scale*scale, sizeof(field_event_t));
    playroom->o_navyfield.scale=scale;
    playroom->o_strikefield.matrix=calloc(scale*scale, sizeof(field_event_t));
    playroom->o_strikefield.scale=scale;
    playroom->o_remaining=0;
    return playroom;
}

bool playroom_played(playroom_t* playroom){
    return !playroom->i_remaining || !playroom->o_remaining;
}

int playroom_strike(playroom_t* playroom, size_t profile, field_location_t location){
    if(profile==playroom->i_profile){
        switch(playroom->o_navyfield.matrix[playroom->scale*location.y+location.x]){
            case IMPACT:
                --playroom->o_remaining;
                playroom->o_navyfield.matrix[playroom->scale*location.y+location.x]=DAMAGED;
                playroom->i_strikefield.matrix[playroom->scale*location.y+location.x]=SHOT;
                break;
            case EMPTY:
                playroom->i_strikefield.matrix[playroom->scale*location.y+location.x]=MISFIRE;
                playroom->o_navyfield.matrix[playroom->scale*location.y+location.x]=MISFIRE;
                break;
        }        
    }
    if(profile==playroom->o_profile){
        switch(playroom->i_navyfield.matrix[playroom->scale*location.y+location.x]){
            case IMPACT:
                --playroom->i_remaining;
                playroom->o_strikefield.matrix[playroom->scale*location.y+location.x]=SHOT;
                playroom->i_navyfield.matrix[playroom->scale*location.y+location.x]=DAMAGED;
                break;
            case EMPTY:
                playroom->o_strikefield.matrix[playroom->scale*location.y+location.x]=MISFIRE;
                playroom->i_navyfield.matrix[playroom->scale*location.y+location.x]=MISFIRE;
                break;
        }
    }
    return 0;
}

int playroom_update_navyfield(playroom_t* playroom, size_t profile, field_location_t location, field_event_t* event){
    event->location=location;
    if(profile==playroom->i_profile){
        event->state=playroom->i_navyfield.matrix[playroom->scale*location.y+location.x];
        return 0;   
    }
    if(profile==playroom->o_profile){
        event->state=playroom->o_navyfield.matrix[playroom->scale*location.y+location.x];
        return 0;
    }
    return PROFILE;
}

int playroom_update_strikefield(playroom_t* playroom, size_t profile, field_location_t location, field_event_t* event){
    event->location=location;
    if(profile==playroom->i_profile){
        event->state=playroom->i_strikefield.matrix[playroom->scale*location.y+location.x];
        return 0;   
    }
    if(profile==playroom->o_profile){
        event->state=playroom->o_strikefield.matrix[playroom->scale*location.y+location.x];
        return 0;
    }
    assert(false);
    return PROFILE;
}

void playroom_set_gid_raw(playroom_t* playroom, size_t gid){
    playroom->gid=gid;
}

void field_set_event_raw(field_t* field, field_event_t* event){
        field->matrix[event->location.y*field->scale+event->location.x]=event->state;
}

void field_clear(field_t* field){
    for(int i=0; i<field->scale; ++i){
        for(int j=0; j<field->scale; ++j){
            field->matrix[i*field->scale+j]=EMPTY;
        }
    }
}

void navyfield_locate_ship_raw(field_t* navyfield, ship_t* ship){
    if(ship->polarization == VERTICAL){
        for(int i = 0; i < ship->scale; ++i){
            navyfield->matrix[(ship->location.y+i)*navyfield->scale+ship->location.x] = IMPACT;
        }
    }else{
        for(int i = 0; i < ship->scale; ++i){
            navyfield->matrix[ship->location.y*navyfield->scale+ship->location.x+i] = IMPACT;
        }
    }

}

bool navyfield_check_ship_raw(field_t* navyfield, ship_t* ship){
    short int up, down, left, right;
    switch(ship->polarization){
        case VERTICAL:
            if(ship->location.x+1>navyfield->scale || ship->location.y+ship->scale>navyfield->scale){
                return false;
            }            
            if(ship->location.x>0){
                left=ship->location.x-1;
            }else{
                left=0;
            }
            if(ship->location.x+1<navyfield->scale){
                right=ship->location.x+2;
            }else{
                right=ship->location.x+1;
            }
            up=ship->location.y;
            down=ship->location.y+ship->scale;
            for(int y=down-1, i=ship->scale-1; y>=up; --y, --i){
                for(int x=left; x<right; ++x){
                    if(navyfield->matrix[y*navyfield->scale+x]!=EMPTY){
                        return false;
                    }
                }
            }
            if(ship->location.y>0){
                up=ship->location.y-1;
                for(int x=left; x<right; ++x){
                    if(navyfield->matrix[up*navyfield->scale+x]!=EMPTY){
                        return false;
                    }
                }
            }
            if(ship->location.y+ship->scale<navyfield->scale){
                down=ship->location.y+ship->scale;
                for(int x=left; x<right; ++x){
                    if(navyfield->matrix[down*navyfield->scale+x]!=EMPTY){
                        return false;
                    }
                }
            }
            break;
        case HORIZONTAL:
            if(ship->location.y+1>navyfield->scale || ship->location.x+ship->scale>navyfield->scale){
                return false;
            }
            if(ship->location.y>0){
                up=ship->location.y-1;
            }else{
                up=0;
            }
            if(ship->location.y+1<navyfield->scale){
                down=ship->location.y+2;
            }else{
                down=ship->location.y+1;
            }
            left=ship->location.x;
            right=ship->location.x+ship->scale;
            for(int x=right-1, i=ship->scale-1; x>=left; --x, --i){
                for(int y=up; y<down; ++y){
                    if(navyfield->matrix[y*navyfield->scale+x]!=EMPTY){
                        return false;
                    }
                }
            }
            if(ship->location.x>0){
                left=ship->location.x-1;
                for(int y=up; y<down; ++y){
                    if(navyfield->matrix[y*navyfield->scale+left]!=EMPTY){
                        return false;
                    }
                }
            }
            if(ship->location.x+ship->scale<navyfield->scale){
                right=ship->location.x+ship->scale;
                for(int y=up; y<down; ++y){
                    if(navyfield->matrix[y*navyfield->scale+right]!=EMPTY){
                        return false;
                    }
                }
            }
            break;
    }
    return true;
}

void navyfield_aiming(playroom_t* playroom, size_t profile, field_location_t* location, aiming_t* aiming){
    field_t* navyfield;
    if(profile==playroom->i_profile){
        navyfield=&playroom->o_navyfield;
    }else{
        navyfield=&playroom->i_navyfield;
    }
    int left, right, up, down;
    if(location->x>0){
        left=location->x-1;
    }
    else{
        left=0;
    }
    if(location->x+1<navyfield->scale){
        right=location->x+1;
    }
    else{
        right=location->x;
    }
    if(location->y>0){
        up=location->y-1;
    }
    else{
        up=0;
    }
    if(location->y+1<navyfield->scale){
        down=location->y+1;
    }
    else{
        down=location->y;
    }
    aiming->size=0;
    for(int i=left; i<=right; ++i){
        for(int j=up; j<=down; ++j){
            aiming->events[aiming->size].location.x=i;
            aiming->events[aiming->size].location.y=j;
            if(navyfield->matrix[j*navyfield->scale+i]==IMPACT){
                aiming->events[aiming->size].state=IMPACT;
            }
            else{
                aiming->events[aiming->size].state=MISFIRE;                
            }
            ++aiming->size;
        }
    }
}

void playroom_purge(playroom_t* playroom){
    free(playroom->i_navyfield.matrix);
    free(playroom->o_navyfield.matrix);
    free(playroom->i_strikefield.matrix);
    free(playroom->o_strikefield.matrix);
    free(playroom);
}
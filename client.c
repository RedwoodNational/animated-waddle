#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <menu.h>
#include <pthread.h>
#include <ncurses.h>
#include <string.h>
#include "zmq.h"
#include "profile.h"
#include "playroom.h"
#include "message.h"

//ERROR CODES
#define MSG_RECIVED 0
#define MSG_SIZE_INVALID -1
#define MSG_SEND_TIMEOUT -2
#define MSG_RECEIVE_TIMEOUT -3

#define AUTH_FILE_PRFMD 0x0
#define AUTH_FILE_EREAD 0x1
#define AUTH_FILE_EWRTE 0x2
#define AUTH_FILE_EOPEN 0x4
#define AUTH_FILE_ECLSE 0x8

#define AUTH_SRVC_SAUTH 0x0
#define AUTH_SRVC_EAUTH 0x2
#define AUTH_SRVC_ESOCK 0x4
#define AUTH_SRVC_ESIZE 0x8


//SERVER CONFIGURARTION
#define GAME_SERVER_ADDRESS "tcp://192.168.1.38:4040"
#define AUTH_SERVER_ADDRESS "tcp://192.168.1.38:1234"

//TIMEOUT SETTINGS
#define ERROR_EXIT_TIMEOUT 30
#define GAME_SERVER_LINGER_TIMEOUT -1
#define GAME_SERVER_SEND_TIMEOUT -1
#define GAME_SERVER_RECEIVE_TIMEOUT -1
#define AUTH_SERVER_SEND_TIMEOUT -1
#define AUTH_SERVER_RECEIVE_TIMEOUT -1
#define AUTH_SERVER_LINGER_TIMEOUT -1

//DELAYS SETTINGS
#define MESSAGE_SHOW_DELAY 3
#define RUNNIG_LINE_DELAY 100000
#define DELAY_SERVER_PING 4


//COLOR SETTINGS FOR MAPS
#define COLOR_DEFAULT_SIGN               COLOR_BLUE
#define COLOR_DEFAULT_BACKGROUND         COLOR_WHITE
#define COLOR_FIELD_SIGN                 COLOR_BLACK
#define COLOR_FIELD_BACKGROUND           COLOR_BLUE
#define COLOR_FIELD_EMPTY_SIGN           COLOR_BLACK
#define COLOR_FIELD_EMPTY_BACKGROUND     COLOR_BLUE
#define COLOR_FIELD_SHOT_SIGN            COLOR_RED
#define COLOR_FIELD_SHOT_BACKGROUND      COLOR_BLUE
#define COLOR_FIELD_MISFIRE_SIGN         COLOR_BLACK
#define COLOR_FIELD_MISFIRE_BACKGROUND   COLOR_BLUE
#define COLOR_FIELD_IMPACT_SIGN          COLOR_WHITE
#define COLOR_FIELD_IMPACT_BACKGROUND    COLOR_BLUE
#define COLOR_FIELD_DAMAGED_SIGN         COLOR_RED
#define COLOR_FIELD_DAMAGED_BACKGROUND   COLOR_BLUE
#define COLOR_FIELD_WAITING_SIGN         COLOR_RED
#define COLOR_FIELD_WAITING_BACKGROUND   COLOR_BLUE
#define COLOR_FIELD_COLLISION_SIGN       COLOR_RED
#define COLOR_FIELD_COLLISION_BACKGROUND COLOR_BLUE
#define COLOR_SCORE_SCREEN_BACKGROUND    COLOR_BLUE
#define COLOR_SCORE_SCREEN_SIGN          COLOR_WHITE

#define AUTH_FILENAME "auth.data"

//SIGNS FOR GAME MAPS
#define SIGN_FIELD_SHOT       " X "
#define SIGN_FIELD_EMPTY      "   "
#define SIGN_FIELD_IMPACT     " * "
#define SIGN_FIELD_MISFIRE    " + "
#define SIGN_FIELD_DAMAGED    " x "
#define SIGN_FIELD_SELECTED   " ^ "
#define SIGN_FIELD_COLLISION  " ! "


int min(int x_value, int y_value){
    if(x_value<y_value){
        return x_value;
    }else{
        return y_value;
    }
}

int max(int x_value, int y_value){
    if(x_value>y_value){
        return x_value;
    }else{
        return y_value;
    }
}

int supply_ships(void* socket, auth_t* auth, fleet_t* fleet){
    if(fleet->size>0){
        int feedback;
        zmq_msg_t message;
        action_t action = NAVY;
        zmq_msg_init_size(&message, sizeof(auth_t));
        memcpy(zmq_msg_data(&message), auth, sizeof(auth_t));
        zmq_msg_send(&message, socket, ZMQ_SNDMORE);
        zmq_msg_close(&message);
        zmq_msg_init_size(&message, sizeof(action_t));
        memcpy(zmq_msg_data(&message), &action, sizeof(action_t));
        zmq_msg_send(&message,socket, ZMQ_SNDMORE);
        zmq_msg_close(&message);
        for(int i=fleet->size-1; i>0; --i){
            zmq_msg_init_size(&message, sizeof(ship_t));
            memcpy(zmq_msg_data(&message), &fleet->ship[i], sizeof(ship_t));
            zmq_msg_send(&message,socket, ZMQ_SNDMORE);
            zmq_msg_close(&message);
        }
        zmq_msg_init_size(&message, sizeof(ship_t));
        memcpy(zmq_msg_data(&message), &fleet->ship[0], sizeof(ship_t));
        zmq_msg_send(&message,socket, 0);
        zmq_msg_close(&message);
    }
    return 0;
    
}

int supply_strike(void* socket, auth_t* auth, field_location_t* strike){
    int feedback;
    zmq_msg_t message;
    action_t action = STRIKE;
    zmq_msg_init_size(&message, sizeof(auth_t));
    memcpy(zmq_msg_data(&message), auth,  zmq_msg_size(&message));
    zmq_msg_send(&message, socket, ZMQ_SNDMORE);
    zmq_msg_close(&message);
    zmq_msg_init_size(&message, sizeof(action_t));
    memcpy(zmq_msg_data(&message), &action,  zmq_msg_size(&message));
    zmq_msg_send(&message,socket, ZMQ_SNDMORE);
    zmq_msg_close(&message);
    zmq_msg_init_size(&message, sizeof(field_location_t));
    memcpy(zmq_msg_data(&message), strike, zmq_msg_size(&message));
    zmq_msg_send(&message, socket, 0);
    zmq_msg_close(&message);
}

int supply_acceptance(void* socket, auth_t* auth, field_location_t* location){
    zmq_msg_t message;
    int choice=0;
    action_t action=CHOOSE;
    zmq_msg_init_size(&message, sizeof(auth_t));
    memcpy(zmq_msg_data(&message), auth,  zmq_msg_size(&message));
    zmq_msg_send(&message, socket, ZMQ_SNDMORE);
    zmq_msg_close(&message);
    zmq_msg_init_size(&message, sizeof(action_t));
    memcpy(zmq_msg_data(&message), &action, sizeof(action_t));
    zmq_msg_send(&message, socket, ZMQ_SNDMORE);
    zmq_msg_close(&message);
    zmq_msg_init_size(&message, sizeof(int));
    memcpy(zmq_msg_data(&message), &choice, sizeof(int));
    zmq_msg_send(&message, socket, ZMQ_SNDMORE);
    zmq_msg_close(&message);
    zmq_msg_init_size(&message, sizeof(field_event_t));
    memcpy(zmq_msg_data(&message), location, sizeof(field_event_t));
    zmq_msg_send(&message, socket, 0);
    zmq_msg_close(&message);

}

int supply_rejection(void* socket, auth_t* auth){
    zmq_msg_t message;
    int choice=1;
    action_t action=CHOOSE;
    zmq_msg_init_size(&message, sizeof(auth_t));
    memcpy(zmq_msg_data(&message), auth,  zmq_msg_size(&message));
    zmq_msg_send(&message, socket, ZMQ_SNDMORE);
    zmq_msg_close(&message);
    zmq_msg_init_size(&message, sizeof(action_t));
    memcpy(zmq_msg_data(&message), &action, sizeof(action_t));
    zmq_msg_send(&message, socket, ZMQ_SNDMORE);
    zmq_msg_close(&message);
    zmq_msg_init_size(&message, sizeof(int));
    memcpy(zmq_msg_data(&message), &choice, sizeof(int));
    zmq_msg_send(&message, socket, 0);
    zmq_msg_close(&message);
}

int accept_event(void* socket, field_event_t* event){
    zmq_msg_t message;
    zmq_msg_init(&message);
    zmq_msg_recv(&message, socket, 0);
    memcpy(event, zmq_msg_data(&message), zmq_msg_size(&message));
    zmq_msg_close(&message);
}

int field_apply(field_t* field, field_event_t* event){
    field->matrix[event->location.y*field->scale+event->location.x]=event->state;
}

void draw_mesh(WINDOW* window, const unsigned scale){
    int height, width;
    wattron(window, COLOR_PAIR(1));
    getmaxyx(window, height, width);
    for(int j=1; j<4; ++j){
        mvwprintw(window, 1,j," ");
    }
    for(int j=1; j<height; j+=2){
        for(int i=0; i<width; i+=4){
            mvwprintw(window, j, i, "|");
        }
    }
    for(int j=0; j<height; j+=2){
        mvwprintw(window, j, 0, " ");
        for(int i=width-2; i>0; i-=1){
            mvwprintw(window, j, i, "-");
        }
        mvwprintw(window, j, width-1, " ");
    }
    for(int j=3, i=1; j<height; j+=2, ++i){
        mvwprintw(window, j, 1, "%-3d", i);
    }
    for(int j=5, i=1; i<width; j+=4, ++i){
        mvwprintw(window, 1, j, "%-3d", i);
    }
}

bool field_init(field_t* field, size_t scale){
    field->scale=scale;
    field->matrix=calloc(scale*scale,sizeof(field_state_t));
}

void field_destroy(field_t* field){
    field->scale=0;
    free(field->matrix);
    field->matrix=NULL;
}

void field_event(WINDOW* screen, const field_event_t event){
    switch(event.state){
        case SHOT:
            wattron(screen, COLOR_PAIR(2));
            mvwprintw(screen, 3+2*event.location.y, 5+4*event.location.x, SIGN_FIELD_SHOT);
            break;
        case EMPTY:
            wattron(screen, COLOR_PAIR(3));
            mvwprintw(screen, 3+2*event.location.y, 5+4*event.location.x, SIGN_FIELD_EMPTY);
            break;
        case IMPACT:
            wattron(screen, COLOR_PAIR(4));
            mvwprintw(screen, 3+2*event.location.y, 5+4*event.location.x, SIGN_FIELD_IMPACT);
            break;
        case MISFIRE:
            wattron(screen, COLOR_PAIR(5));
            mvwprintw(screen, 3+2*event.location.y, 5+4*event.location.x, SIGN_FIELD_MISFIRE);
            break;
        case DAMAGED:
            wattron(screen, COLOR_PAIR(6));
            mvwprintw(screen, 3+2*event.location.y, 5+4*event.location.x, SIGN_FIELD_DAMAGED);
            break;
        case SELECTED:
            wattron(screen, COLOR_PAIR(7));
            mvwprintw(screen, 3+2*event.location.y, 5+4*event.location.x, SIGN_FIELD_SELECTED);
            break;

        case COLLISION:
            wattron(screen, COLOR_PAIR(8));
            mvwprintw(screen, 3+2*event.location.y, 5+4*event.location.x, SIGN_FIELD_COLLISION);
            break;
    }
    wrefresh(screen);
}

bool field_restore(WINDOW* screen, field_t* field, field_location_t location){
    field_event_t event;
    event.location.x=location.x;
    event.location.y=location.y;
    event.state=field->matrix[event.location.y*field->scale+event.location.x];
    field_event(screen, event);
}

void field_redraw(WINDOW* screen, const field_t field){
    int height, width;
    getmaxyx(screen, height, width);
    wattron(screen, COLOR_PAIR(2));
    for(int x=5, i=0; x<width; x+=4, ++i){
        for(int y=3, j=0; y<height; y+=2, ++j){
            switch(field.matrix[j*field.scale+i]){
                case SHOT:
                    wattron(screen, COLOR_PAIR(2));
                    mvwprintw(screen, y, x, SIGN_FIELD_SHOT);
                    break;
                case EMPTY:
                    wattron(screen, COLOR_PAIR(3));
                    mvwprintw(screen, y, x, SIGN_FIELD_EMPTY);
                    break;
                case IMPACT:
                    wattron(screen, COLOR_PAIR(4));
                    mvwprintw(screen, y, x, SIGN_FIELD_IMPACT);
                    break;
                case MISFIRE:
                    wattron(screen, COLOR_PAIR(5));
                    mvwprintw(screen, y, x, SIGN_FIELD_MISFIRE);
                    break;
                case DAMAGED:
                    wattron(screen, COLOR_PAIR(6));
                    mvwprintw(screen, y, x, SIGN_FIELD_DAMAGED);
                    break;

                case SELECTED:
                    wattron(screen, COLOR_PAIR(7));
                    mvwprintw(screen, y, x, SIGN_FIELD_SELECTED);
                    break;

                case COLLISION:
                    wattron(screen, COLOR_PAIR(8));
                    mvwprintw(screen, y, x, SIGN_FIELD_COLLISION);
                    break;
            }  
        }
    }
}

bool navyfield_check(WINDOW* screen, field_t* navyfield, ship_t* ship){
    short int up;
    short int down;
    short int left;
    short int right;
    field_event_t event;
    bool last_check=true;
    bool global_check=true;
    bool ship_scan[ship->scale];
    for(int i=0; i<ship->scale; ++i){
        ship_scan[i]=true;
    }
    switch(ship->polarization){
        case VERTICAL:
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
                bool local_check=true;
                for(int x=left; x<right; ++x){
                    if(navyfield->matrix[y*navyfield->scale+x]!=EMPTY){
                        local_check=false;
                    }
                }
                ship_scan[i]&=local_check && last_check;
                global_check&=local_check;
                last_check=local_check;
            }
            last_check=true;
            for(int y=up, i=0; y<down; ++y, ++i){
                bool local_check=true;
                for(int x=left; x<right; ++x){
                    if(navyfield->matrix[y*navyfield->scale+x]!=EMPTY){
                        local_check=false;
                    }
                }
                ship_scan[i]&=local_check && last_check;
                global_check&=local_check;
                last_check=local_check;
            }
            if(ship->location.y>0){
                bool local_check=true;
                up=ship->location.y-1;
                for(int x=left; x<right; ++x){
                    if(navyfield->matrix[up*navyfield->scale+x]!=EMPTY){
                        local_check=false;
                    }
                }
                global_check&=local_check;
                ship_scan[0]&=local_check;
            }
            if(ship->location.y+ship->scale<navyfield->scale){
                bool local_check=true;
                down=ship->location.y+ship->scale;
                for(int x=left; x<right; ++x){
                    if(navyfield->matrix[down*navyfield->scale+x]!=EMPTY){
                        local_check=false;
                    }
                }
                ship_scan[ship->scale-1]&=local_check;
                global_check&=local_check;
            }
            event.location.x=ship->location.x;
            for(int i=0; i<ship->scale; ++i){
                event.location.y=ship->location.y+i;
                if(ship_scan[i]){
                    event.state=IMPACT;
                }else{
                    event.state=COLLISION;
                }
                field_event(screen, event);
            }
            break;
        case HORIZONTAL:
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
                bool local_check=true;
                for(int y=up; y<down; ++y){
                    if(navyfield->matrix[y*navyfield->scale+x]!=EMPTY){
                        local_check=false;
                    }
                }
                ship_scan[i]&=local_check && last_check;
                global_check&=local_check;
                last_check=local_check;
            }
            last_check=true;
            for(int x=left, i=0; x<right; ++x, ++i){
                bool local_check=true;
                for(int y=up; y<down; ++y){
                    if(navyfield->matrix[y*navyfield->scale+x]!=EMPTY){
                        local_check=false;
                    }
                }
                ship_scan[i]&=local_check && last_check;
                global_check&=local_check;
                last_check=local_check;
            }
            if(ship->location.x>0){
                bool local_check=true;
                left=ship->location.x-1;
                for(int y=up; y<down; ++y){
                    if(navyfield->matrix[y*navyfield->scale+left]!=EMPTY){
                        local_check=false;
                    }
                }
                global_check&=local_check;
                ship_scan[0]&=local_check;
            }
            if(ship->location.x+ship->scale<navyfield->scale){
                bool local_check=true;
                right=ship->location.x+ship->scale;
                for(int y=up; y<down; ++y){
                    if(navyfield->matrix[y*navyfield->scale+right]!=EMPTY){
                        local_check=false;
                    }
                }
                global_check&=local_check; 
                ship_scan[ship->scale-1]&=local_check;
            }
            event.location.y=ship->location.y;
            for(int i=0; i<ship->scale; ++i){
                event.location.x=ship->location.x+i;
                if(ship_scan[i]){
                    event.state=IMPACT;
                }else{
                    event.state=COLLISION;
                }
                field_event(screen, event);
            }
        break;
    }
    return global_check;
}

bool navyfield_setup(WINDOW* screen, field_t* navyfield, ship_t* ship){
    bool global_check=false;
    if(navyfield->scale<=0 && ship->scale<=0 && ship->scale<=navyfield->scale){
        return false;
    }
    field_event_t event;
    bool configured=false;
    global_check=navyfield_check(screen, navyfield, ship);
    wrefresh(screen);
    while(!configured){
        switch(wgetch(screen)){
            case KEY_UP:
                if(ship->location.y>0){
                    if(ship->polarization==VERTICAL){
                        --ship->location.y;
                        event.location.x=ship->location.x;                   
                        event.location.y=ship->location.y+ship->scale;
                        event.state=navyfield->matrix[(ship->location.y+ship->scale)*navyfield->scale+ship->location.x];
                        field_event(screen, event);
                    }else{
                        event.location.y=ship->location.y;
                        for(int i=ship->scale+ship->location.x-1; i>=0; --i){
                            event.location.x=i;
                            event.state=navyfield->matrix[ship->location.y*navyfield->scale+i];
                            field_event(screen, event);
                        }
                        --ship->location.y;
                    }
                }else{
                    if(ship->polarization==VERTICAL){
                        event.location.x=ship->location.x;
                        ship->location.y=navyfield->scale-ship->scale;
                        for(int i=min(ship->scale, ship->location.y-1); i>=0; --i){
                            event.location.y=i;
                            event.state=navyfield->matrix[i*navyfield->scale+ship->location.x];
                            field_event(screen, event);
                        }
                    }else{
                        event.location.y=ship->location.y;
                        for(int i=ship->scale+ship->location.x-1; i>=0; --i){
                            event.location.x=i;
                            event.state=navyfield->matrix[ship->location.y*navyfield->scale+i];
                            field_event(screen, event);
                        }
                        ship->location.y=navyfield->scale-1;
                    }
                }                   
                break;
            case KEY_DOWN:
                if(ship->polarization==VERTICAL){
                    if(ship->location.y+ship->scale<navyfield->scale){
                        event.location.y=ship->location.y;
                        event.location.x=ship->location.x;                   
                        event.state=navyfield->matrix[ship->location.y*navyfield->scale+ship->location.x];
                        field_event(screen, event);
                        ++ship->location.y;
                    }else{
                        event.location.x=ship->location.x;
                        for(int i=max(ship->scale, ship->location.y); i<navyfield->scale; ++i){
                            event.location.y=i;
                            event.state=navyfield->matrix[i*navyfield->scale+ship->location.x];
                            field_event(screen, event);
                        }
                        ship->location.y=0;
                    }
                }else{
                    if(ship->location.y+1<navyfield->scale){
                        event.location.y=ship->location.y;
                        for(int i=ship->scale+ship->location.x-1; i>=0; --i){
                            event.location.x=i;
                            event.state=navyfield->matrix[ship->location.y*navyfield->scale+i];
                            field_event(screen, event);
                        }
                        ++ship->location.y;      
                    }else{
                        event.location.y=ship->location.y;
                        for(int i=ship->scale+ship->location.x-1; i>=0; --i){
                            event.location.x=i;
                            event.state=navyfield->matrix[ship->location.y*navyfield->scale+i];
                            field_event(screen, event);
                        }
                        ship->location.y=0;  
                    }
                }
                break;
            case KEY_LEFT:
                if(ship->location.x>0){
                    if(ship->polarization==VERTICAL){
                        event.location.x=ship->location.x;
                        for(int i=ship->location.y+ship->scale-1; i>=0; --i){
                            event.location.y=i;
                            event.state=navyfield->matrix[i*navyfield->scale+ship->location.x];
                            field_event(screen, event);
                        }
                        --ship->location.x;
                    }else{
                        --ship->location.x;
                        event.location.y=ship->location.y;
                        event.location.x=ship->location.x+ship->scale;
                        event.state=navyfield->matrix[ship->location.y*navyfield->scale+ship->location.x+ship->scale];
                        field_event(screen, event);
                    }
                }else{
                    if(ship->polarization==VERTICAL){
                        event.location.x=ship->location.x;
                        for(int i=ship->location.y+ship->scale-1; i>=0; --i){
                            event.location.y=i;
                            event.state=navyfield->matrix[i*navyfield->scale+ship->location.x];
                            field_event(screen, event);
                        }
                        ship->location.x=navyfield->scale-1;
                    }else{
                        event.location.y=ship->location.y;
                        ship->location.x=navyfield->scale-ship->scale;
                        for(int i=min(ship->location.x-1, ship->scale); i>=0; --i){
                            event.location.x=i;
                            event.state=navyfield->matrix[ship->location.y*navyfield->scale+i];
                            field_event(screen, event);
                        }
                    }
                }
                break;
            case KEY_RIGHT:
                if(ship->polarization==VERTICAL){
                    if(ship->location.x+1<navyfield->scale){
                        event.location.x=ship->location.x;
                        for(int i=ship->location.y+ship->scale-1; i>=0; --i){
                            event.location.y=i;
                            event.state=navyfield->matrix[i*navyfield->scale+ship->location.x];
                            field_event(screen, event);
                        }
                        ++ship->location.x;
                    }else{
                        event.location.x=ship->location.x;
                        for(int i=ship->location.y+ship->scale-1; i>=0; --i){
                            event.location.y=i;
                            event.state=navyfield->matrix[i*navyfield->scale+ship->location.x];
                            field_event(screen, event);
                        }
                        ship->location.x=0;
                    }
                }else{
                    if(ship->location.x+ship->scale<navyfield->scale){
                        event.location.y=ship->location.y;
                        event.location.x=ship->location.x;
                        event.state=navyfield->matrix[ship->location.y*navyfield->scale+ship->location.x];
                        field_event(screen, event);
                        ++ship->location.x;
                    }else{
                        event.location.y=ship->location.y;
                        for(int i=max(ship->scale, ship->location.x); i<navyfield->scale; ++i){
                            event.location.x=i;
                            event.state=navyfield->matrix[ship->location.y*navyfield->scale+i];
                            field_event(screen, event);
                        }
                        ship->location.x=0;
                    }
                }
                break;
            case '\n':
                    configured=global_check;
                break;
            case '\t':
                if(ship->polarization==HORIZONTAL){
                    ship->polarization=VERTICAL;
                    event.location.y=ship->location.y;
                    for(int i=ship->location.x+ship->scale-1; i>=0; --i){
                        event.location.x=i;
                        event.state=navyfield->matrix[ship->location.y*navyfield->scale+i];
                        field_event(screen, event);
                    }
                    if(ship->location.y+ship->scale>navyfield->scale){
                        ship->location.y=navyfield->scale-ship->scale;
                    }
                }else{
                    ship->polarization=HORIZONTAL;
                    event.location.x=ship->location.x;
                    for(int i=ship->location.y+ship->scale-1; i>=ship->location.y; --i){
                        event.location.y=i;
                        event.state=navyfield->matrix[i*navyfield->scale+ship->location.x];
                        field_event(screen, event);
                    }
                    if(ship->location.x+ship->scale>navyfield->scale){
                        ship->location.x=navyfield->scale-ship->scale;
                    }
                }
                break;
        }
        global_check=navyfield_check(screen, navyfield, ship);
        wnoutrefresh(screen);
        doupdate();
    }
    event.state=IMPACT;
    if(ship->polarization==VERTICAL){
        event.location.x=ship->location.x;
        for(int i=0; i<ship->scale; ++i){
            navyfield->matrix[(ship->location.y+i)*navyfield->scale+ship->location.x]=IMPACT;
            event.location.y=ship->location.y+i;
            field_event(screen, event);
        }
    }else{
        event.location.y=ship->location.y;
        for(int i=0; i<ship->scale; ++i){
            navyfield->matrix[ship->location.y*navyfield->scale+ship->location.x+i]=IMPACT;
            event.location.x=ship->location.x+i;
            field_event(screen, event);
        }
    }
    return true;
}

void strikefield_select(WINDOW* screen, field_t* strikefield, field_location_t* location){
    field_event_t event={SELECTED, *location};
    field_event(screen, event);
    keypad(screen, true);
    bool active = true;
    while(active){
        switch (wgetch(screen)){
            case '\n':
                active&=strikefield->matrix[location->y*strikefield->scale+location->x]!=EMPTY;
                active&=strikefield->matrix[location->y*strikefield->scale+location->x]!=IMPACT;
                break;                                        
            case KEY_UP:
                field_restore(screen, strikefield, *location);
                if(location->y>0){
                    --location->y;
                }
                else{
                    location->y=strikefield->scale-1;
                }
                event.location.y=location->y;
                field_event(screen, event);
                break;
            case KEY_LEFT:
                field_restore(screen, strikefield, *location);
                if(location->x>0){
                    --location->x;
                }
                else{
                    location->x=strikefield->scale-1;
                }
                event.location.x=location->x;
                field_event(screen, event);
                break;
            case KEY_DOWN:
                field_restore(screen, strikefield, *location);
                if(location->y+1<strikefield->scale){
                    ++location->y;
                }
                else{
                    location->y=0;
                }
                event.location.y=location->y;
                field_event(screen, event);
                break;                
            case KEY_RIGHT:
                field_restore(screen, strikefield, *location);
                if(location->x+1<strikefield->scale){
                    ++location->x;
                }
                else{
                    location->x=0;
                }
                event.location.x=location->x;
                field_event(screen, event);
                break;
        }
    }
    keypad(screen, false);
}

int auth_signup(void* socket, auth_t* auth, profile_t* profile){
    int feedback;
    status_t status;
    zmq_msg_t message;
    action_t action=SIGNUP;
    feedback=zmq_msg_init_size(&message, sizeof(action_t));
    if(feedback!=0){
        zmq_msg_close(&message);
        return AUTH_SRVC_ESOCK;
    }
    memcpy(zmq_msg_data(&message), &action, sizeof(action_t));
    feedback=zmq_msg_send(&message, socket, 0);
    if(feedback==-1){
        zmq_msg_close(&message);
        return AUTH_SRVC_ESOCK;
    }
    feedback=zmq_msg_close(&message);
    if(feedback!=0){
        zmq_msg_close(&message);
        return AUTH_SRVC_ESOCK;
    }
    feedback=zmq_msg_init(&message);
    if(feedback!=0){
        zmq_msg_close(&message);
        return AUTH_SRVC_ESOCK;
    }
    feedback=zmq_msg_recv(&message, socket, 0);
    if(feedback==-1){
        zmq_msg_close(&message);
        return AUTH_SRVC_ESOCK;
    }
    if(feedback!=sizeof(auth_t)){
        zmq_msg_close(&message);
        return AUTH_SRVC_ESIZE;
    }
    memcpy(auth, zmq_msg_data(&message), sizeof(auth_t));
    feedback=zmq_msg_close(&message);
    if(feedback!=0){
        zmq_msg_close(&message);
        return AUTH_SRVC_ESOCK;
    }
    feedback=zmq_msg_init(&message);
    if(feedback!=0){
        zmq_msg_close(&message);
        return AUTH_SRVC_ESOCK;
    }
    feedback=zmq_msg_recv(&message, socket, 0);
    if(feedback==-1){
        zmq_msg_close(&message);
        return AUTH_SRVC_ESOCK;
    }
    if(feedback!=sizeof(profile_t)){
        zmq_msg_close(&message);
        return AUTH_SRVC_ESIZE;
    }
    memcpy(profile, zmq_msg_data(&message), sizeof(profile_t));
    feedback=zmq_msg_close(&message);
    if(feedback!=0){
        zmq_msg_close(&message);
        return AUTH_SRVC_ESOCK;
    }
    return AUTH_SRVC_SAUTH; 
}

int auth_signin(void* socket, auth_t* auth, profile_t* profile){
        int feedback;
        status_t status;
        zmq_msg_t message;
        action_t action=SIGNIN;
        feedback=zmq_msg_init_size(&message, sizeof(action_t));
        if(feedback!=0){
            zmq_msg_close(&message);
            return AUTH_SRVC_ESOCK;
        }
        memcpy(zmq_msg_data(&message), &action, sizeof(action_t));
        feedback=zmq_msg_send(&message, socket, ZMQ_SNDMORE);
        if(feedback==-1){
            zmq_msg_close(&message);
            return AUTH_SRVC_ESOCK;
        }
        feedback=zmq_msg_close(&message);
        if(feedback!=0){
            zmq_msg_close(&message);
            return AUTH_SRVC_ESOCK;
        }
        feedback=zmq_msg_init_size(&message, sizeof(auth_t));
        if(feedback!=0){
            zmq_msg_close(&message);
            return AUTH_SRVC_ESOCK;
        }
        memcpy(zmq_msg_data(&message), auth, sizeof(auth_t));
        feedback=zmq_msg_send(&message, socket, 0);
        if(feedback==-1){
            zmq_msg_close(&message);
            return AUTH_SRVC_ESOCK;
        }
        feedback=zmq_msg_close(&message);
        if(feedback!=0){
            zmq_msg_close(&message);
            return AUTH_SRVC_ESOCK;
        }
        feedback=zmq_msg_init(&message);
        if(feedback!=0){
            zmq_msg_close(&message);
            return AUTH_SRVC_ESOCK;
        }
        feedback=zmq_msg_recv(&message, socket, 0);
        if(feedback==-1){
            zmq_msg_close(&message);
            return AUTH_SRVC_ESOCK;
        }
        if(feedback!=sizeof(status_t)){
            zmq_msg_close(&message);
            return AUTH_SRVC_ESIZE;
        }
        memcpy(&status, zmq_msg_data(&message), sizeof(status_t));
        if(status!=AUTHORIZED){
            zmq_msg_close(&message);
            return AUTH_SRVC_EAUTH;
        }
        feedback=zmq_msg_close(&message);
        if(feedback!=0){
            zmq_msg_close(&message);
            return AUTH_SRVC_ESOCK;
        }
        feedback=zmq_msg_init(&message);
        if(feedback!=0){
            zmq_msg_close(&message);
            return AUTH_SRVC_ESOCK;
        }        
        feedback=zmq_msg_recv(&message, socket, 0);
        if(feedback==-1){
            zmq_msg_close(&message);
            return AUTH_SRVC_ESOCK;
        }
        if(feedback!=sizeof(profile_t)){
            zmq_msg_close(&message);
            return AUTH_SRVC_ESIZE;
        }
        memcpy(profile, zmq_msg_data(&message), sizeof(profile_t));
        feedback=zmq_msg_close(&message);
        if(feedback!=0){
            zmq_msg_close(&message);
            return AUTH_SRVC_ESOCK;
        }   
        return AUTH_SRVC_SAUTH;
}

int auth_save(const char* filename, auth_t* auth){
    FILE* backup;
    int operation=AUTH_FILE_PRFMD;
    if((backup=fopen(filename, "wb"))){
        if(fwrite(auth, sizeof(auth_t), 1, backup)!=1){
            operation|=AUTH_FILE_EWRTE;
        }
        if(fclose(backup)){
            operation|=AUTH_FILE_ECLSE;
        }
    }
    else{
        operation|=AUTH_FILE_EOPEN;
    }
    return operation;
}

int auth_load(const char* filename, auth_t* auth){
    FILE* backup;
    int operation=AUTH_FILE_PRFMD;
    if((backup=fopen(filename, "rb"))){
        if(fread(auth, sizeof(auth_t), 1, backup)!=1){
            operation|=AUTH_FILE_EREAD;
        }
        if(fclose(backup)){
            operation|=AUTH_FILE_ECLSE;
        }
    }
    else{
        operation=AUTH_FILE_EOPEN;
    }
    return operation;
}

int accept_hint(void* socket, field_t* strikefield, WINDOW* strikescreen){
    zmq_msg_t message;
    aiming_t aiming;
    zmq_msg_init(&message);
    zmq_msg_recv(&message, socket, 0);
    memcpy(&aiming, zmq_msg_data(&message), zmq_msg_size(&message));
    zmq_msg_close(&message);
    for(int i=0; i<aiming.size; ++i){
        field_apply(strikefield, &aiming.events[i]);
        field_event(strikescreen, aiming.events[i]);
    } 
}

struct rline{
    int delay;
    char* message;
    WINDOW* screen;
    pthread_mutex_t mutex;
};
typedef struct  rline rline_t;

void* rline_function(void* routine){
    rline_t* settings=(rline_t*)routine;
    int width=getmaxx(settings->screen);
    int height=getmaxy(settings->screen);
    int length=strlen(settings->message);

    int box_left;
    int message_left;
    int box_right;
    int message_right;

    if(length>width){
        while(true){
            box_left=width, box_right=width, message_left=0, message_right=0;
            while(box_left>-1){
                wclear(settings->screen);
                for(int j=message_left,  i=box_left; j<message_right; ++j, ++i){
                    mvwaddch(settings->screen, 0, i, settings->message[j]);
                }
                wrefresh(settings->screen);
                --box_left;
                ++message_right;
                usleep(settings->delay);
            }
            while(message_right<length){
                wclear(settings->screen);
                for(int j=message_left,  i=box_left; j<message_right; ++j, ++i){
                    mvwaddch(settings->screen, 0, i, settings->message[j]);
                }
                wrefresh(settings->screen);
                ++message_left;
                ++message_right;
                usleep(settings->delay);
            }
            while(box_right>-1){
                wclear(settings->screen);
                for(int j=message_left,  i=box_left; j<message_right; ++j, ++i){
                    mvwaddch(settings->screen, 0, i, settings->message[j]);
                }
                wrefresh(settings->screen);
                --box_right;
                ++message_left;
                usleep(settings->delay);
            }
        }
    }else{
        while(true){
            box_left=width;
            box_right=width;
            message_left=0;
            message_right=0;
            while(message_right<length){
                wclear(settings->screen);
                for(int j=message_left,  i=box_left; j<message_right; ++j, ++i){
                    mvwaddch(settings->screen, 0, i, settings->message[j]);
                }
                wrefresh(settings->screen);
                --box_left;
                ++message_right;
                usleep(settings->delay);
            }
            while(box_left>-1){
                wclear(settings->screen);
                for(int j=message_left,  i=box_left; j<message_right; ++j, ++i){
                    mvwaddch(settings->screen, 0, i, settings->message[j]);
                }
                wrefresh(settings->screen);
                --box_left;
                --box_right;
                usleep(settings->delay);
            }
            while(box_right>-1){
                wclear(settings->screen);
                for(int j=message_left,  i=box_left; j<message_right; ++j, ++i){
                    mvwaddch(settings->screen, 0, i, settings->message[j]);
                }
                wrefresh(settings->screen);
                --box_right;
                ++message_left;
                usleep(settings->delay);
            }
        }
    }
    pthread_exit(NULL);
}


void rline_start(rline_t* rline, pthread_t* thread){
    pthread_create(thread, NULL, rline_function, rline);
}

void rline_stop(pthread_t* thread){
    pthread_cancel(*thread);
    pthread_join(*thread, NULL);
}


int main(int argc, char *argv[]){
    int zmq_feedback;
    void* context=zmq_ctx_new();
    void* asocket=zmq_socket(context, ZMQ_REQ);
    int alngtimeo=AUTH_SERVER_LINGER_TIMEOUT;
    int atxtimeo=AUTH_SERVER_RECEIVE_TIMEOUT;
    int arxtimeo=AUTH_SERVER_RECEIVE_TIMEOUT;
    zmq_feedback=zmq_setsockopt(asocket, ZMQ_RCVTIMEO, &arxtimeo, sizeof(int));
    zmq_feedback=zmq_setsockopt(asocket, ZMQ_SNDTIMEO, &atxtimeo, sizeof(int));
    zmq_feedback=zmq_setsockopt(asocket, ZMQ_LINGER, &alngtimeo, sizeof(int));
    zmq_feedback=zmq_connect(asocket, AUTH_SERVER_ADDRESS);

    void* gsocket=zmq_socket(context, ZMQ_DEALER);
    int glngtimeo=GAME_SERVER_LINGER_TIMEOUT;
    int gtxtimeo=GAME_SERVER_RECEIVE_TIMEOUT;
    int grxtimeo=GAME_SERVER_RECEIVE_TIMEOUT;
    zmq_feedback=zmq_setsockopt(gsocket, ZMQ_SNDTIMEO, &gtxtimeo, sizeof(int));
    zmq_feedback=zmq_setsockopt(gsocket, ZMQ_RCVTIMEO, &grxtimeo, sizeof(int));
    zmq_feedback=zmq_setsockopt(gsocket, ZMQ_LINGER, &glngtimeo, sizeof(int));
    zmq_feedback=zmq_connect(gsocket, GAME_SERVER_ADDRESS);


    initscr(); 
    noecho();
    cbreak();
    clear();
    curs_set(false);
    srand(time(NULL));
    int width=getmaxx(stdscr);
    int height=getmaxy(stdscr);
    

    //----colors configuration-----------------------------------------------------------------
    if(!has_colors()){
        for(int i=ERROR_EXIT_TIMEOUT; i>0; --i){
            printw("Terminal doesn`t support color mode. Closing in %d\n", i);
            refresh();
            sleep(1);
            clear();
        }
        endwin();
        exit(EXIT_FAILURE);
    }

    start_color();
    init_pair(1, COLOR_FIELD_SIGN, COLOR_FIELD_BACKGROUND);
    init_pair(2, COLOR_FIELD_SHOT_SIGN, COLOR_FIELD_SHOT_BACKGROUND);
    init_pair(3, COLOR_FIELD_EMPTY_SIGN, COLOR_FIELD_EMPTY_BACKGROUND);
    init_pair(4, COLOR_FIELD_IMPACT_SIGN, COLOR_FIELD_IMPACT_BACKGROUND);
    init_pair(5, COLOR_FIELD_MISFIRE_SIGN, COLOR_FIELD_MISFIRE_BACKGROUND);
    init_pair(6, COLOR_FIELD_DAMAGED_SIGN, COLOR_FIELD_DAMAGED_BACKGROUND);
    init_pair(7, COLOR_FIELD_WAITING_SIGN, COLOR_FIELD_WAITING_BACKGROUND);
    init_pair(8, COLOR_FIELD_COLLISION_SIGN, COLOR_FIELD_COLLISION_BACKGROUND);
    init_pair(9, COLOR_SCORE_SCREEN_SIGN, COLOR_SCORE_SCREEN_BACKGROUND);
    init_pair(10, COLOR_DEFAULT_SIGN, COLOR_DEFAULT_BACKGROUND);
    //--------------------------------------------------------------------------------------------
    
    int state;
    auth_t auth;
    aiming_t aiming;
    action_t action;
    bool authorized;
    size_t scale;
    fleet_t fleet;
    field_t navyfield;
    field_t strikefield;
    field_event_t event;
    field_location_t location;

    profile_t profile;
    profile.wins=0;
    profile.loses=0;
    profile.games=0;
    profile.ratio=0;
    profile.shots=0;
    profile.misses=0;

    WINDOW* strikescreen;
    WINDOW* scorescreen;
    WINDOW* navyscreen;
    WINDOW* menuscreen;
    WINDOW* linescreen;
    MENU* main_menu;
    MENU* field_menu;
    MENU* reveal_menu;
    ITEM** main_menu_items;
    ITEM** field_menu_items;
    ITEM** reveal_menu_items;

    int field_width;
    int field_height;
    int score_width=30;
    int score_height=15;
    int vertical_margin=(height-score_height)/2;
    int horizontal_margin=(width-score_width)/2;

    strikescreen=newwin(0,0,0,0);
    scorescreen=newwin(0,0,0,0);
    navyscreen=newwin(0,0,0,0);
    linescreen=newwin(1,34 , (height-1)/2, (width-34)/2);
    menuscreen=newwin(score_height,score_width,vertical_margin, horizontal_margin);

    char* main_menu_choices[]={
        " N E W   G A M E ",
        "    S C O R E    ",
        "     E X I T     ",
    };

    char* field_menu_choices[]={
        "  S M A L L   (10 x 10) ",
        " N O R M A L  (12 x 12) ",
        "  L A R G E   (14 x 14) "
    };

    char* reveal_menu_choices[]={
        " A C C E P T ",
        " R E F U S E "
    };

    const int main_menu_choices_count=sizeof(main_menu_choices)/sizeof(*main_menu_choices);
    main_menu_items = calloc(main_menu_choices_count+ 1, sizeof(ITEM *));
	for(int i = 0; i < main_menu_choices_count; ++i){
        main_menu_items[i] = new_item(main_menu_choices[i], NULL);
    }
	main_menu_items[main_menu_choices_count] = (ITEM *)NULL;
    main_menu = new_menu((ITEM **)main_menu_items);
    set_menu_win(main_menu, menuscreen);
    WINDOW* main_menu_subwin=derwin(menuscreen, main_menu_choices_count,18, 6,6);
    set_menu_sub(main_menu, main_menu_subwin);
    set_menu_back(main_menu, COLOR_PAIR(1));
    set_menu_mark(main_menu, NULL);

    const int field_menu_choices_count=sizeof(field_menu_choices)/sizeof(*field_menu_choices);
    field_menu_items = (ITEM **)calloc(field_menu_choices_count + 1, sizeof(ITEM *));
    for(int i = 0; i < field_menu_choices_count; ++i){
        field_menu_items[i] = new_item(field_menu_choices[i], NULL);
    }
	field_menu_items[field_menu_choices_count] = (ITEM *)NULL;
    field_menu = new_menu((ITEM **)field_menu_items);
    set_menu_win(field_menu, menuscreen);
    WINDOW* field_menu_subwin=derwin(menuscreen, field_menu_choices_count,24, 6,3);
    set_menu_sub(field_menu, field_menu_subwin);
    set_menu_back(field_menu, COLOR_PAIR(1));
    set_menu_mark(field_menu, NULL);

    const int reveal_menu_choices_count=sizeof(reveal_menu_choices)/sizeof(*reveal_menu_choices);
    reveal_menu_items = calloc(reveal_menu_choices_count+ 1, sizeof(ITEM *));
	for(int i = 0; i < reveal_menu_choices_count; ++i){
        reveal_menu_items[i] = new_item(reveal_menu_choices[i], NULL);
    }
	reveal_menu_items[reveal_menu_choices_count] = (ITEM *)NULL;
    reveal_menu = new_menu((ITEM **)reveal_menu_items);
    set_menu_win(reveal_menu, scorescreen);
    set_menu_mark(reveal_menu, NULL);
    WINDOW* reveal_menu_border=derwin(scorescreen, 4, 23, 3, 1);
    WINDOW* reveal_menu_subwin=derwin(reveal_menu_border, 2, 13, 1, 5);
    set_menu_sub(reveal_menu, reveal_menu_subwin);
    set_menu_back(reveal_menu, COLOR_PAIR(9));
    wattron(reveal_menu_border, COLOR_PAIR(9));
    wattron(reveal_menu_subwin, COLOR_PAIR(9));


    wbkgd(stdscr, COLOR_PAIR(9));
    wbkgd(linescreen, COLOR_PAIR(9));
    wrefresh(stdscr);

    rline_t rline_settings;
    pthread_t rline_thread;
    rline_settings.screen=linescreen;
    rline_settings.delay=RUNNIG_LINE_DELAY;
    rline_settings.message="Connectig... Authentication data is being retreived.";
    rline_start(&rline_settings, &rline_thread);

    if(auth_load(AUTH_FILENAME, &auth)==AUTH_FILE_PRFMD){
        switch (auth_signin(asocket, &auth, &profile)){
            case AUTH_SRVC_SAUTH:
                authorized=true;
                rline_stop(&rline_thread);
                rline_settings.message="Authorization successfull. Profile restored.";
                rline_start(&rline_settings, &rline_thread);
                sleep(1);
                break;

            case AUTH_SRVC_ESOCK:
                rline_stop(&rline_thread);
                rline_settings.message="Connection couldn`t be established. Check server status.";
                rline_start(&rline_settings, &rline_thread);
                authorized=false;
                sleep(ERROR_EXIT_TIMEOUT);
                break;

            case AUTH_SRVC_ESIZE:
                rline_stop(&rline_thread);
                rline_settings.message="Server responce has incorrect size. Try restarting the game.";
                rline_start(&rline_settings, &rline_thread);
                authorized=false;
                sleep(ERROR_EXIT_TIMEOUT);
                break;

            case AUTH_SRVC_EAUTH:
                rline_stop(&rline_thread);
                rline_settings.message="Authentication data is incorrect. New profile will be ackquired.";
                rline_start(&rline_settings, &rline_thread);
                switch (auth_signup(asocket, &auth, &profile)){
                    case AUTH_SRVC_SAUTH:
                        authorized=true;
                        rline_stop(&rline_thread);
                        auth_save(AUTH_FILENAME, &auth);
                        rline_settings.message="Authorization successfull. New profile created.";
                        rline_start(&rline_settings, &rline_thread);
                        sleep(1);
                        break;

                    case AUTH_SRVC_ESOCK:
                        rline_stop(&rline_thread);
                        rline_settings.message="Connection couldn`t be established. Check server status.";
                        rline_start(&rline_settings, &rline_thread);
                        authorized=false;
                        sleep(ERROR_EXIT_TIMEOUT);
                        break;

                    case AUTH_SRVC_ESIZE:
                        rline_stop(&rline_thread);
                        rline_settings.message="Server reponce has incorrect size. Try restarting the game.";
                        rline_start(&rline_settings, &rline_thread);
                        authorized=false;
                        sleep(ERROR_EXIT_TIMEOUT);
                        break;
                }
                break;
        }
    }
    else{
        switch (auth_signup(asocket, &auth, &profile)){
            case AUTH_SRVC_SAUTH:
                authorized=true;
                rline_stop(&rline_thread);
                auth_save(AUTH_FILENAME, &auth);
                rline_settings.message="Authorization successfull. New profile created";
                rline_start(&rline_settings, &rline_thread);
                sleep(1);
                break;

            case AUTH_SRVC_ESOCK:
                rline_stop(&rline_thread);
                rline_settings.message="Connection couldn`t be established. Check server status";
                rline_start(&rline_settings, &rline_thread);
                authorized=false;
                sleep(ERROR_EXIT_TIMEOUT);
                break;
                
            case AUTH_SRVC_ESIZE:
                rline_stop(&rline_thread);
                rline_settings.message="Server reponce has incorrect size. Try restarting the game";
                rline_start(&rline_settings, &rline_thread);
                authorized=false;
                sleep(ERROR_EXIT_TIMEOUT);
                break;
        }
    }
    rline_stop(&rline_thread);
    int choice=0;
    bool finished=false;
    while(!finished && authorized){        
        wbkgd(stdscr, COLOR_PAIR(10));
        wbkgd(menuscreen, COLOR_PAIR(9));
        wborder(menuscreen, 0,0,0,0,0,0,0,0);
        keypad(menuscreen, true);
        post_menu(main_menu);
        wrefresh(stdscr);
        wrefresh(menuscreen);
        bool selected=false;
        while(!selected){
            switch(wgetch(menuscreen)){	
                case '\n':
                    selected=true;
                    break;
                case KEY_UP:
                    if(choice>0){
                        --choice;
                        menu_driver(main_menu, REQ_UP_ITEM);
                    }else{
                        choice=main_menu_choices_count-1;
                        menu_driver(main_menu, REQ_LAST_ITEM);
                    }
                    break;
                case KEY_DOWN:
                    if(choice+1<main_menu_choices_count){
                        ++choice;
                        menu_driver(main_menu, REQ_DOWN_ITEM);
                    }else{
                        choice=0;
                        menu_driver(main_menu, REQ_FIRST_ITEM);
                    }
                    break;
            }
            wrefresh(main_menu_subwin);
        }
        keypad(menuscreen, false);
        unpost_menu(main_menu);
        wrefresh(menuscreen);
        if(choice==0){
            selected=false;
            post_menu(field_menu);
            wrefresh(menuscreen);
            keypad(menuscreen, true);
            while(!selected){
                switch(wgetch(menuscreen)){	
                    case '\n':
                        selected=true;
                        break;
                    case KEY_UP:
                        if(choice>0){
                            --choice;
                            menu_driver(field_menu, REQ_UP_ITEM);
                        }else{
                            choice=field_menu_choices_count-1;
                            menu_driver(field_menu, REQ_LAST_ITEM);
                        }
                        break;
                    case KEY_DOWN:
                        if(choice+1<field_menu_choices_count){
                            ++choice;
                            menu_driver(field_menu, REQ_DOWN_ITEM);
                        }else{
                            choice=0;
                            menu_driver(field_menu, REQ_FIRST_ITEM);
                        }
                        break;
                }
                wrefresh(menuscreen);
            }
            unpost_menu(field_menu);
            keypad(menuscreen, false);
            wrefresh(menuscreen);
            switch(choice){
                case 0:
                    scale=SMALL;
                    break;

                case 1:
                    scale=NORMAL;
                    break;

                case 2:
                    scale=LARGE;
                    break;
            }
            field_init(&navyfield, scale);
            field_init(&strikefield, scale);

            field_height=2*navyfield.scale+3;
            field_width=3*(navyfield.scale+1)+navyfield.scale+2;
            score_width=25;
            score_height=field_height;
            vertical_margin=(height-field_height)/2;
            horizontal_margin=(width-2*field_width-score_width)/2;
            wresize(navyscreen, field_height, field_width);
            mvwin(navyscreen, (height-field_height)/2, (width-field_width)/2);
            draw_mesh(navyscreen, navyfield.scale);
            field_redraw(navyscreen, navyfield);
            wrefresh(stdscr);
            wrefresh(navyscreen);

            //---ship---location---setup-----------------------------------------------------------------------
            int foursquares;
            int threesquares;
            int twosquares;
            int onesquares;
            switch(navyfield.scale){
                case SMALL:
                    foursquares=SMALL_FOURSQUARES;
                    threesquares=SMALL_THREESQUARES;
                    twosquares=SMALL_TWOSQUARES;
                    onesquares=SMALL_ONESQUARES;
                    break;

                case NORMAL:
                    foursquares=NORMAL_FOURSQUARES;
                    threesquares=NORMAL_THREESQUARES;
                    twosquares=NORMAL_TWOSQUARES;
                    onesquares=NORMAL_ONESQUARES;
                    break;

                case LARGE:
                    foursquares=LARGE_FOURSQUARES;
                    threesquares=LARGE_THREESQUARES;
                    twosquares=LARGE_TWOSQUARES;
                    onesquares=LARGE_ONESQUARES;
                    break;

            }
            fleet.size=0;
            keypad(navyscreen, true);
            for(int i=0; i<foursquares; ++i){
                fleet.ship[fleet.size].scale=4;
                switch (rand()%2){
                    case HORIZONTAL:
                        fleet.ship[fleet.size].polarization=HORIZONTAL;
                        fleet.ship[fleet.size].location.y=rand()%navyfield.scale;
                        fleet.ship[fleet.size].location.x=rand()%(navyfield.scale-fleet.ship[fleet.size].scale);
                        break;
                    
                    case VERTICAL:
                        fleet.ship[fleet.size].polarization=VERTICAL;
                        fleet.ship[fleet.size].location.x=rand()%navyfield.scale;
                        fleet.ship[fleet.size].location.y=rand()%(navyfield.scale-fleet.ship[fleet.size].scale);
                        break;
                }
                navyfield_setup(navyscreen, &navyfield, &fleet.ship[fleet.size++]);
            }
            for(int i=0; i<threesquares; ++i){
                fleet.ship[fleet.size].scale=3;
                switch (rand()%2){
                    case HORIZONTAL:
                        fleet.ship[fleet.size].polarization=HORIZONTAL;
                        fleet.ship[fleet.size].location.y=rand()%navyfield.scale;
                        fleet.ship[fleet.size].location.x=rand()%(navyfield.scale-fleet.ship[fleet.size].scale);
                        break;
                    
                    case VERTICAL:
                        fleet.ship[fleet.size].polarization=VERTICAL;
                        fleet.ship[fleet.size].location.x=rand()%navyfield.scale;
                        fleet.ship[fleet.size].location.y=rand()%(navyfield.scale-fleet.ship[fleet.size].scale);
                        break;
                }
                navyfield_setup(navyscreen, &navyfield, &fleet.ship[fleet.size++]);
            }
            for(int i=0; i<twosquares; ++i){
                fleet.ship[fleet.size].scale=2;
                switch (rand()%2){
                    case HORIZONTAL:
                        fleet.ship[fleet.size].polarization=HORIZONTAL;
                        fleet.ship[fleet.size].location.y=rand()%navyfield.scale;
                        fleet.ship[fleet.size].location.x=rand()%(navyfield.scale-fleet.ship[fleet.size].scale);
                        break;
                    
                    case VERTICAL:
                        fleet.ship[fleet.size].polarization=VERTICAL;
                        fleet.ship[fleet.size].location.x=rand()%navyfield.scale;
                        fleet.ship[fleet.size].location.y=rand()%(navyfield.scale-fleet.ship[fleet.size].scale);
                        break;
                }
                navyfield_setup(navyscreen, &navyfield, &fleet.ship[fleet.size++]);
            }
            for(int i=0; i<onesquares; ++i){
                fleet.ship[fleet.size].scale=1;
                switch (rand()%2){
                    case HORIZONTAL:
                        fleet.ship[fleet.size].polarization=HORIZONTAL;
                        fleet.ship[fleet.size].location.y=rand()%navyfield.scale;
                        fleet.ship[fleet.size].location.x=rand()%(navyfield.scale-fleet.ship[fleet.size].scale);
                        break;
                    
                    case VERTICAL:
                        fleet.ship[fleet.size].polarization=VERTICAL;
                        fleet.ship[fleet.size].location.x=rand()%navyfield.scale;
                        fleet.ship[fleet.size].location.y=rand()%(navyfield.scale-fleet.ship[fleet.size].scale);
                        break;
                }
                navyfield_setup(navyscreen, &navyfield, &fleet.ship[fleet.size++]);
            }
            keypad(navyscreen, false);
            //-------------------------------------------------------------------------------------------------
            action=START;
            zmq_msg_t message;
            zmq_msg_init_size(&message, sizeof(auth_t));
            memcpy(zmq_msg_data(&message), &auth, sizeof(auth_t));
            zmq_msg_send(&message, gsocket, ZMQ_SNDMORE);
            zmq_msg_close(&message);
            zmq_msg_init_size(&message, sizeof(action_t));
            memcpy(zmq_msg_data(&message), &action, sizeof(action_t));
            zmq_msg_send(&message, gsocket, ZMQ_SNDMORE);
            zmq_msg_close(&message);
            zmq_msg_init_size(&message, sizeof(size_t));
            memcpy(zmq_msg_data(&message), &scale, sizeof(size_t));
            zmq_msg_send(&message, gsocket, 0);
            zmq_msg_close(&message);

            wresize(strikescreen, field_height, field_width);
            wresize(scorescreen, score_height, score_width);
            wresize(linescreen, 1, score_width-2);
            mvwin(navyscreen, vertical_margin, horizontal_margin+field_width+score_width);
            mvwin(scorescreen, vertical_margin, horizontal_margin+field_width);
            mvwin(strikescreen, vertical_margin, horizontal_margin);
            mvwin(linescreen, vertical_margin+1, horizontal_margin+field_width+1);

            wrefresh(linescreen);
            draw_mesh(strikescreen, strikefield.scale);
            field_redraw(strikescreen, strikefield);
            wborder(scorescreen, 0,0,0,0,0,0,0,0);
            field_redraw(navyscreen, navyfield);
            redrawwin(stdscr);
            wbkgd(scorescreen, COLOR_PAIR(9));
            wrefresh(stdscr);
            wrefresh(navyscreen);
            wrefresh(scorescreen);
            wrefresh(strikescreen);
            location.x=0;
            location.y=0;

        
            bool playing=true;
            while(playing){
                zmq_pollitem_t item = {gsocket,   0, ZMQ_POLLIN, 0};
                zmq_poll (&item, 1, 120);
                if (item.revents & ZMQ_POLLIN) {
                    zmq_msg_init(&message);
                    zmq_msg_recv(&message, gsocket, 0);
                    memcpy(&state, zmq_msg_data(&message), sizeof(int));
                    zmq_msg_close(&message);
                    switch(state){
                        case INQUEUE:
                            wclear(linescreen);
                            wprintw(linescreen, "       In  queue\n       ");
                            wrefresh(linescreen);
                        break;

                        case PLAYING:
                            wclear(linescreen);
                            wprintw(linescreen, "    Chose  position\n    ");
                            wrefresh(linescreen);
                            strikefield_select(strikescreen, &strikefield, &location);
                            supply_strike(gsocket, &auth, &location);
                        break;

                        case WAITING:
                            wclear(linescreen);
                            wprintw(linescreen, "     Wait for enemy\n    ");
                            wrefresh(linescreen);
                        break;

                        case WINNER:
                            wclear(linescreen);
                            wprintw(linescreen, "        Victory\n        ");
                            wrefresh(linescreen);
                            playing = false;
                            sleep(3);
                        break;

                        case LOSER:
                            wclear(linescreen);
                            wprintw(linescreen, "      Dont  worry\n      ");
                            wrefresh(linescreen);
                            playing = false;
                            sleep(3);
                        break;

                        case REVEAL:
                            wclear(linescreen);
                            wprintw(linescreen, "     Lucky  chance\n     ");
                            wrefresh(linescreen);
                            choice=0;
                            selected=false;
                            wborder(reveal_menu_border,0,0,0,0,0,0,0,0);
                            post_menu(reveal_menu);
                            wrefresh(scorescreen);
                            wrefresh(stdscr);
                            keypad(scorescreen, true);
                            while(!selected){
                                switch (wgetch(scorescreen)){
                                    case '\n':
                                        selected=true;
                                        break;
                                    case KEY_UP:
                                        if(choice>0){
                                            --choice;
                                            menu_driver(reveal_menu, REQ_UP_ITEM);
                                            break;
                                        }
                                        else{
                                            choice=reveal_menu_choices_count-1;
                                            menu_driver(reveal_menu, REQ_LAST_ITEM);
                                            break;
                                        }
                                        break;
                                    case KEY_DOWN:
                                        if(choice+1<reveal_menu_choices_count){
                                            ++choice;
                                            menu_driver(reveal_menu, REQ_DOWN_ITEM);
                                        }
                                        else{
                                            choice=0;
                                            menu_driver(reveal_menu, REQ_FIRST_ITEM);
                                        }
                                        break;
                                }
                                wrefresh(scorescreen);
                                wrefresh(stdscr);
                            }
                            keypad(scorescreen, false);
                            unpost_menu(reveal_menu);
                            wclear(reveal_menu_border);
                            wclear(reveal_menu_subwin);
                            wbkgd(reveal_menu_border, COLOR_PAIR(1));
                            wbkgd(reveal_menu_subwin, COLOR_PAIR(1));
                            wrefresh(scorescreen);
                            wrefresh(stdscr);
                            if(choice==0){
                                wclear(linescreen);
                                wprintw(linescreen, "     Choose area\n     ");
                                wrefresh(linescreen);
                                strikefield_select(strikescreen, &strikefield, &location);
                                supply_acceptance(gsocket, &auth, &location);
                            }else{
                                supply_rejection(gsocket, &auth);
                            }
                        break;

                        case REVEALED:
                            wclear(linescreen);
                            wprintw(linescreen, "     Declassified\n     ");
                            wrefresh(linescreen);
                            sleep(2);
                        break;

                        case CHECKIN:
                            wclear(linescreen);
                            wprintw(linescreen, "   Transfering  data\n   ");
                            wrefresh(linescreen);
                            supply_ships(gsocket, &auth, &fleet);
                        break;

                        case TIMEOUT:
                            wclear(linescreen);
                            wprintw(linescreen, "         Timeout\n       ");
                            wrefresh(linescreen);
                            playing=false;
                            sleep(5);
                        break;

                        case SYNCHINT:
                            wclear(linescreen);
                            wprintw(linescreen, "Retreiving itelligence\n");
                            wrefresh(linescreen);
                            supply_ships(gsocket, &auth, &fleet);
                            accept_hint(gsocket, &strikefield, strikescreen);
                        break;
                            
                        case SYNCNAVY:
                            wclear(linescreen);
                            wprintw(linescreen, "    Retreiving loss\n   ");
                            wrefresh(linescreen);
                            accept_event(gsocket, &event);
                            field_apply(&navyfield, &event);
                            field_redraw(navyscreen, navyfield);
                            wrefresh(navyscreen);
                        break;

                        case SYNCSTRIKE:
                            wclear(linescreen);
                            wprintw(linescreen, "Reporting coordinates\n");
                            wrefresh(linescreen);
                            accept_event(gsocket, &event);
                            field_apply(&strikefield, &event);
                            field_redraw(strikescreen, strikefield);
                            wrefresh(strikescreen);
                        break;

                    }
                }
            }
        }
        else if(choice == 1){
            score_width=30;
            score_height=15;
            vertical_margin=(height-score_height)/2;
            horizontal_margin=(width-score_width)/2;
            wresize(scorescreen, score_height, score_width);
            mvwin(scorescreen, vertical_margin, horizontal_margin);
            wclear(scorescreen);
            wattron(scorescreen, COLOR_PAIR(1));
            wbkgd(scorescreen, COLOR_PAIR(9));
            wborder(scorescreen, 0,0,0,0,0,0,0,0);
            WINDOW* conduit=derwin(scorescreen, 13,28, 1,1);
            wprintw(conduit, "Profile: %d\n", auth.uid);
            wprintw(conduit, "Cookie:  %d\n", auth.cookie);
            wprintw(conduit, "Ratio:   %lf \n", profile.ratio);
            wprintw(conduit, "Shots:   %u \n", profile.shots);
            wprintw(conduit, "Misses:  %u \n", profile.misses);
            wprintw(conduit, "Games:   %u \n", profile.games);
            wprintw(conduit, "Wins:    %u \n", profile.wins);
            wprintw(conduit, "Loses:   %u \n", profile.loses);
            wrefresh(stdscr);
            wrefresh(conduit);
            wrefresh(scorescreen);
            wgetch(scorescreen);
            delwin(conduit);
            wclear(scorescreen);
        }else if(choice == 2){
            finished=true;
        }
    }

    //resourse cleanup
    //---------------------------------------------------------------------------------------------------
    cleanup:
    wclear(stdscr);
    wrefresh(stdscr);
    zmq_unbind(asocket, AUTH_SERVER_ADDRESS);
    zmq_unbind(gsocket, GAME_SERVER_ADDRESS);
    zmq_close(asocket);
    zmq_close(gsocket);
    zmq_ctx_destroy(context);
    free_menu(main_menu);
    for(int i = 0; i < main_menu_choices_count; ++i){
        free_item(main_menu_items[i]);
    }
    free(main_menu_items);

    free_menu(field_menu);
    for(int i = 0; i < field_menu_choices_count; ++i){
        free_item(field_menu_items[i]);
    }
    free(field_menu_items);

    delwin(strikescreen);
    delwin(scorescreen);
    delwin(navyscreen);
    delwin(menuscreen);
    echo();
    endwin();
    return 0;
}
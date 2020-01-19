#ifndef PLAYROOM_H
#define PLAYROOM_H

#define FEATURE_REVEAL_AREA 0x02
#define FEATURE_FIRST_PLAYER 0x01

#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#define OFFSIDE -1
#define PROFILE -2
#define ESTRIKE -3
#define EROOMEX -4
#define EEVENTX -5


//DEFAULTS
#define SMALL 10
#define NORMAL 12
#define LARGE 14

#define SMALL_FOURSQUARES 1
#define SMALL_THREESQUARES 2
#define SMALL_TWOSQUARES 3
#define SMALL_ONESQUARES 4

#define NORMAL_FOURSQUARES 2
#define NORMAL_THREESQUARES 3
#define NORMAL_TWOSQUARES 4
#define NORMAL_ONESQUARES 5

#define LARGE_FOURSQUARES 3
#define LARGE_THREESQUARES 4
#define LARGE_TWOSQUARES 5
#define LARGE_ONESQUARES 6

struct ship;
struct field;
struct field_event;
struct field_location;

typedef struct ship ship_t;
typedef struct field field_t;
typedef struct field_event field_event_t;
typedef struct field_location field_location_t;

enum field_state{
    EMPTY,
    SHOT,
    MISFIRE,
    IMPACT,
    DAMAGED,
    SELECTED,
    COLLISION,
};

enum polarization{
    VERTICAL,
    HORIZONTAL
};

typedef enum field_state field_state_t;
typedef enum polarization polarization_t;

struct field_location{
    short unsigned x;
    short unsigned y;
};

struct field_event{
    field_state_t state;
    field_location_t location;
};

struct field{
    unsigned short scale;
    field_state_t* matrix;
};

struct ship{
    unsigned int scale;
    field_location_t location;
    polarization_t polarization;
};

struct playroom{
    size_t gid;
    short unsigned scale;
    time_t o_trace;
    size_t o_profile;
    size_t o_remaining;
    field_t o_navyfield;
    field_t o_strikefield;
    time_t i_trace;
    size_t i_profile;
    size_t i_remaining;
    field_t i_navyfield;
    field_t i_strikefield;
};

typedef struct playroom playroom_t;

struct fleet{
    size_t size;
    ship_t ship[32];
};
typedef struct fleet fleet_t;

struct aiming{
    field_event_t events[9];
    size_t size;
};
typedef struct aiming aiming_t;


void navyfield_aiming(playroom_t* playroom, size_t profile, field_location_t* location, aiming_t* aiming);

void* playroom_create(size_t size);

void playroom_set_gid(playroom_t* playroom, size_t gid);

void field_set_event(field_t* field, field_event_t* event);

void field_clear(field_t* field);

void navyfield_locate_ship_raw(field_t* navyfield, ship_t* ship);

bool navyfield_check_ship_raw(field_t* navyfield, ship_t* ship);

int playroom_strike(playroom_t* playroom, size_t profile, field_location_t location);

int playroom_update_strikefield(playroom_t* playroom, size_t profile, field_location_t location, field_event_t* event);

int playroom_update_navyfield(playroom_t* playroom, size_t profile, field_location_t location, field_event_t* event);

bool playroom_played(playroom_t* playroom);

void playroom_purge(playroom_t* playroom);
#endif
#ifndef MESSAGE_H
#define MESSAGE_H

struct auth{
    size_t uid;
    size_t cookie;
};
typedef struct auth auth_t;


enum action{
    SIGNUP,
    SIGNIN,
    STRIKE,
    CHOOSE,
    NAVY,
    START,
    ALIVE,
    FINISH,
};
typedef enum action action_t;

enum status{
    AUTHORIZED,
    REGISTERED,
    UNAVAILABLE,
    UNAUTHORIZED,
    DISCONNECTED,
    UNRECOGNIZED,
};
typedef enum status status_t;


#endif
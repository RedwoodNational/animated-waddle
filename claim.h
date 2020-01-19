#ifndef CLAIM_H
#define CLAIM_H
#include <stdlib.h>

#define VALID 1
#define INVALID 2
struct claim{
    size_t profile;
    int validation;
};
typedef struct claim claim_t;
#endif
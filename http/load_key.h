#ifndef _LOAD_KEY_H
#define _LOAD_KEY_H
#include "acsmx.h"
#include "common.h"


int init_fast_match();


typedef struct _key_node_t {
        char txt[MAX_TXT_LEN]; 
        unsigned int id;
        unsigned int flag; 
} key_node_t;

int rule_test();
#endif

#ifndef MSG_H
#define MSG_H

#define N 'n' 
#define F 'f'
#define L 'l'


typedef enum {TYPE, SEARCH, DATA, END} msg_type;

typedef struct
{
    msg_type type;
    char string[64];
} msg;

#endif

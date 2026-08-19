#ifndef __LOOP2APP_H__
    #define __LOOP2APP_H__

#include "rcserver.h"

typedef struct __attribute__((packed))
{
    uint8_t msjtype;
    uint16_t datalen;
    uint8_t data[1024];
}stTxRxRequest;


int init_loop2app(void);
int test_loop(void);


#endif

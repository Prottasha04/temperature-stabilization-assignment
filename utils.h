#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>   


typedef enum {
    MSG_TEMP = 1,  
    MSG_DONE = 2    
} MsgType;

struct msg {
    int   Type;     
    int   Index;    
    float T;       
};

struct msg prepare_message(int i_Index, float i_Temperature, int i_Type);

ssize_t send_all(int fd, const void *buf, size_t len);
ssize_t recv_all(int fd, void *buf, size_t len);

int send_msg(int fd, const struct msg *m);
int recv_msg(int fd, struct msg *m);

#endif 

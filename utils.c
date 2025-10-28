#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include "utils.h"


struct msg prepare_message(int i_Index, float i_Temperature, int i_Type) {
    struct msg message;
    message.Type  = i_Type;
    message.Index = i_Index;
    message.T     = i_Temperature;
    return message;
}

ssize_t send_all(int fd, const void *buf, size_t len) {
    const char *p = (const char *)buf;
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, p + sent, len - sent, 0);
        if (n < 0) return n;   // error
        if (n == 0) break;     // peer closed
        sent += (size_t)n;
    }
    return (ssize_t)sent;
}

ssize_t recv_all(int fd, void *buf, size_t len) {
    char *p = (char *)buf;
    size_t recvd = 0;
    while (recvd < len) {
        ssize_t n = recv(fd, p + recvd, len - recvd, 0);
        if (n < 0) return n;   // error
        if (n == 0) break;     // peer closed
        recvd += (size_t)n;
    }
    return (ssize_t)recvd;
}

int send_msg(int fd, const struct msg *m) {
    return send_all(fd, m, sizeof(*m)) == (ssize_t)sizeof(*m);
}

int recv_msg(int fd, struct msg *m) {
    return recv_all(fd, m, sizeof(*m)) == (ssize_t)sizeof(*m);
}


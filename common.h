//
// Created by Piero Ulloa on 2019-08-01.
//

#ifndef SOCKETEXAMPLE_COMMON_H
#define SOCKETEXAMPLE_COMMON_H

#include <semaphore.h>
#include "unistd.h"
#include "stdio.h"
#include "arpa/inet.h"
#include "libs/binn.h"

typedef enum {
    INVALID_MSG = 0,
    CHAT_MSG,
    WELCOME_MSG
}MessageKind;

typedef struct {
    MessageKind m;
    char from[20];
    char msg[200];
}Message_t;



int setup_socket_server(int port);
int setup_socket(const char* address, const char* port);
Message_t* parse_buffer(int sockfd);
binn* msg_to_binn(Message_t* msg);

#endif //SOCKETEXAMPLE_COMMON_H

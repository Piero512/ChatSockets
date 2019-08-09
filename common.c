//
// Created by Piero Ulloa on 2019-08-01.
//

#include "stdlib.h"
#include "common.h"
#include "string.h"
#include "arpa/inet.h"


#define BUFFER_SIZE 256

/**
 *
 * @param address Internet address to connect
 * @return -1 if setup socket is unsuccessful
 */

int setup_socket(const char* address, const char* port) {
    struct sockaddr_in sock;
    memset(&sock, 0, sizeof(struct sockaddr_in));
    int error_inet = inet_pton(AF_INET, address, &sock);
    if(error_inet <=0){
        return -1;
    }
    sock.sin_port = htons(atoi(port));
    int sock_fd = socket(AF_INET,SOCK_STREAM, 0);
    int error_connect = connect(sock_fd, (struct sockaddr*) &sock, sizeof(struct sockaddr_in));
    if (error_connect){
        perror(NULL);
        return -1;
    }
    return sock_fd;
}



int setup_socket_server(int port) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd == -1){
        return -1;
    }
    struct sockaddr_in sock;
    memset(&sock, 0, sizeof(struct sockaddr_in));
    sock.sin_family = AF_INET;
    sock.sin_addr.s_addr = INADDR_ANY;
    sock.sin_port = htons(port);
    int error_bind = bind(sock_fd, (struct sockaddr*) &sock, sizeof(sock));
    if(error_bind){
        return -1;
    }
    int error_listen = listen(sock_fd,128);
    if (error_listen){
        return -1;
    }
    return sock_fd;
}



binn* msg_to_binn(Message_t* msg) {
    binn *obj = binn_object();
    binn_object_set_str(obj,"from", msg->from);
    binn_object_set_str(obj, "msg", msg->msg);
    binn_object_set_int8(obj, "message_type", msg->m);
    return obj;
}


Message_t* parse_buffer(int sockfd) {
    char buffer[BUFFER_SIZE];
    memset(&buffer, 0, BUFFER_SIZE);
    size_t bytes_read = recv(sockfd, &buffer, BUFFER_SIZE, 0); // Shouldn't block, because it's supposed to have data here.
    if(bytes_read > 0){
        binn* obj = binn_open(buffer); 
        if(obj == NULL){
            fprintf(stderr, "Error al parsear buffer, binn ha retornado NULL\n");
        }
        Message_t* received_msg = malloc(sizeof(*received_msg));
        signed char msg_kind = 0;
        if (binn_object_get_int8(obj, "message_type", &msg_kind)) {
            received_msg->m = msg_kind;
            char* text = NULL;
            char* from = NULL;
            switch(received_msg->m){
                case CHAT_MSG:
                    if(binn_object_get_str(obj,"msg", &text)){
                        strncpy(received_msg->msg, text, sizeof(received_msg->msg));
                        received_msg->msg[199] = 0; // Null terminate
                    } else {
                        free(received_msg);
                        binn_free(obj);
                        return NULL;
                    }
                case WELCOME_MSG:
                    if(binn_object_get_str(obj, "from", &from)){
                        strncpy(received_msg->from, from, sizeof(received_msg->from) - 1);
                        received_msg->from[sizeof(received_msg->from) - 1] = 0; // Null terminate.
                    } else{
                        free(received_msg);
                        binn_free(obj);
                        return NULL;
                    }
                    break;
                case INVALID_MSG:
                    free(received_msg);
                    binn_free(obj);
                    return NULL;
            }
            return received_msg;
        } else {
            free(received_msg);
            binn_free(obj);
            return NULL;
        }
    }
    return NULL;
}

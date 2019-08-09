//
// Created by Piero Ulloa on 2019-08-01.
//

#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include "common.h"
#include "stdlib.h"
#include "string.h"
#include "libs/sts_queue.h"

struct ClientDetails {
    StsHeader* message_queue;
    int connected_fd;
    bool should_poll;
    char name[20];
};
typedef struct ClientDetails client_details_t;

static client_details_t conn_details;

void* send_loop(void* arg){
    client_details_t* details = arg;
    Message_t msg;
    memset(msg.msg, 0, sizeof(msg.msg));
    strncpy(msg.from,details->name, sizeof(msg.from) - 1);
    msg.from[sizeof(msg.from) - 1] = 0;
    char* buffer_getline = NULL;
    msg.m = CHAT_MSG;
    size_t linecap = 0;
    ssize_t linelen;
    while(conn_details.should_poll){
        linelen = getline(&buffer_getline, &linecap, stdin);
        if(linelen > 0){
            // Copiar el mensaje ingresado al msg.
            strncpy(msg.msg, buffer_getline ,sizeof(msg.msg) - 1);
            msg.msg[sizeof(msg.msg) - 1] = 0;
            binn* serialized_msg = msg_to_binn(&msg);
            if(serialized_msg != NULL){
                ssize_t bytes_sent = send(details->connected_fd, binn_ptr(serialized_msg), binn_size(serialized_msg), 0);
                if (bytes_sent <= 0){ // Error sending? Maybe disconnected, so end everything.
                    binn_free(serialized_msg);
                    break;
                }
                // printf("(%s): %s\n", msg.from, msg.msg); // Local echo.
                binn_free(serialized_msg);
            } 
        }
        
    }
    free(buffer_getline); // Free the buffer that getline alloc-ed
    return NULL;
}
void* receive_loop(void* arg){
    client_details_t* details = arg;
    StsHeader* queue = details->message_queue;
    int fd = details->connected_fd;
    assert(fd != -1);
    while(details->should_poll){
        Message_t * received_msg = parse_buffer(fd);
        if (received_msg != NULL){
            switch(received_msg->m){
                case INVALID_MSG:
                    // Ignore msg and free it.
                    break;
                case CHAT_MSG:
                    printf("(%s): %s\n", received_msg->from, received_msg->msg);
                    break;
                case WELCOME_MSG:
                    printf("(Server): El usuario %s se ha unido a la sala de chat!\n", received_msg->from);
                    break;
            }
            free(received_msg);
        } else {
            details->should_poll = false;
        }
    }
    return NULL;
}

void sighandler(int signal_no){
    conn_details.should_poll = false;
}
void print_usage(const char* program_name, bool go_stderr){
    fprintf(go_stderr ? stderr: stdout,"Uso: %s [IP] [Puerto] [Nombre de usuario]", program_name);
}

int main(int argc, char* argv[]){
    if(argc < 3){
        print_usage(argv[0], false);
        return 1;
    }
    struct sigaction sig = {0};
    sig.sa_handler = sighandler;
    int retval = sigaction(SIGINT,&sig, 0);
    if(retval){
        fprintf(stderr, "No se pudo fijar un handler para SIGINT :think:\n");
    }
    int sockfd = setup_socket(argv[1], argv[2]);
    if(sockfd == -1){
        fprintf(stderr,"Error al conectar a %s:%s\n", argv[1], argv[2]);
        return 1;
    }
    conn_details.should_poll = true;
    conn_details.connected_fd = sockfd;
    conn_details.message_queue = StsQueue.create();
    strncpy(conn_details.name, argv[3], sizeof(conn_details.name) - 1);
    conn_details.name[sizeof(conn_details.name) -1] = 0;
    pthread_t receive_thread, send_thread;
    pthread_create(&receive_thread, NULL, receive_loop, &conn_details);
    pthread_create(&send_thread, NULL, send_loop, &conn_details);
    pthread_join(receive_thread, NULL);
    pthread_join(send_thread, NULL);
    return 0;
}
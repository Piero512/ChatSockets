#include <stdbool.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>
#include <assert.h>
#include "common.h"
#include "stdlib.h"
#include "string.h"
#include "libs/sts_queue.h"
#include "libs/binn.h"

// Server global vars
bool active = true;

void resend_msg_to_everyone(Message_t* msg, struct pollfd array_fds[], nfds_t size){
    binn * obj = msg_to_binn(msg);
    for (unsigned int i = 0; i < size; ++i) {
        int fd = array_fds[i].fd;
        if(fd > 0){
            ssize_t bytes_sent = send(fd, binn_ptr(obj), binn_size(obj), 0); // Send message.
            if(bytes_sent < 0){
                fprintf(stderr, "Error al enviar mensaje?\n");
            }
        } 
        // Skip not valid FDs
    }
    binn_free(obj);
}

void * handle_connections(void* args){
    StsHeader* shared_queue = (StsHeader *) args;
    struct pollfd* array_fds = calloc(1024, sizeof(*array_fds));
    nfds_t last = 0;
    while(active){
        // Check in a queue if there are new sockets to listen to.
        while(StsQueue.size(shared_queue) > 0){
            int* received_fd = StsQueue.pop(shared_queue);
            if (received_fd != NULL) {
                assert(*received_fd > 0);
                (array_fds + last)->fd = *received_fd;
                (array_fds + last)->events = POLLIN;
                last++;
            } else { // It's empty for some reason?
                break;
            }
            // Dequeue file descriptors into the array. If missing spaces, realloc the array.
        }
        // poll for a while, and if the FDs have changed, read msgs and send msgs.
        int changed = poll(array_fds, last, 500); // Wait half a second for messages.
        if(changed){
            for (int i = 0; i < last; ++i) {
                struct pollfd* current_fd = (array_fds + i);
                short revents = current_fd->revents;
                if(revents & POLLIN){
                    // New data!
                    Message_t * msg = parse_buffer(current_fd->fd);
                    // Send this msg to every client!
                    if(msg){
                        resend_msg_to_everyone(msg, array_fds, last);
                    } else {
                        fprintf(stderr, "He recibido un mensaje invalido? \n");
                    }

                    free(msg);
                } else if (revents & POLLERR){
                    // Something happened?
                    current_fd->fd = -1; // Set the thing to skip stuff.
                }
            }
        }
    }
    return NULL;
}
// Receives connected socket.

void sighandler(int signal, siginfo_t* info, void* ctx){
    active = false;
}

int main(int argc, const char* argv[]) {
    printf("Inicializando servidor\n");
    if(argc < 2){
        fprintf(stderr,"Usage: %s [port]", argv[0]);
        return 1;
    }
    int sock_fd = setup_socket_server(atoi(argv[1]));
    if(sock_fd == -1){
        printf("Error al abrir puerto para escuchar!\n");
        return 1;
    }
    struct sockaddr_in datos_conn;
    socklen_t size = sizeof(datos_conn);
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = sighandler;
    int retval = sigaction(SIGINT, &sig, NULL);
    if(retval){
        printf("No se intercepta sigint. :think:\n");
    }
    StsHeader * handle = StsQueue.create();
    pthread_t socket_thread;
    if(pthread_create(&socket_thread, NULL, handle_connections, handle)){
        // Pthread failed?
        fprintf(stderr, "Error al crear el thread del servidor.");
        return 1;
    }
    printf("Se ha iniciado el hilo del servidor.");
    while(active){
        int connected_fd = accept(sock_fd, (struct sockaddr *) &datos_conn, &size);
        if(connected_fd > 0){
            printf("Se ha conectado un cliente! IP: %s\n", inet_ntoa(datos_conn.sin_addr));;
        } else {
            printf("Accept retornó -1?");
            active = false;
            break;
        }
        int* copy = malloc(sizeof(*copy));
        *copy = connected_fd;
        StsQueue.push(handle,copy);

    }
    pthread_join(socket_thread, NULL);
    return 0;
}
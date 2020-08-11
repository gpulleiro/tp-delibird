#ifndef socket_h
#define socket_h

#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "utils.h"
#include "serializacion.h"

#include <arpa/inet.h>//inet_addr
#include <pthread.h>


void* recibir_mensaje(int socket_cliente, int* size);
int iniciar_cliente(char *ip, char* puerto);
int iniciar_servidor(char* ip, char* puerto);
//void esperar_cliente(int);
void* recibir_mensaje(int socket_cliente, int* size);
int recibir_operacion(int);
//void process_request(int cod_queue, int cliente_fd);
//void serve_client(int *socket);
void liberar_conexion(int socket_cliente);



#endif /* socket_h */

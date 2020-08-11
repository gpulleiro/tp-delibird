#include "socket.h"

//Falta cambiar de void a int
int iniciar_servidor (char* ip, char* puerto){
	struct sockaddr_in direccion_servidor;

	direccion_servidor.sin_family = AF_INET;
	direccion_servidor.sin_addr.s_addr = INADDR_ANY;
	direccion_servidor.sin_port = htons(atoi(puerto));

	int servidor = socket(AF_INET, SOCK_STREAM,0);

	//para poder probar que ande sin tener que esperar 2min
	int activado = 1;
	setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

	if(bind(servidor, (void*) &direccion_servidor, sizeof(direccion_servidor)) !=0){
		perror("Fallo el bind");
	}


	listen(servidor,SOMAXCONN); //flag para que tome el maximo posible de espacio

//	while(1){
//		esperar_cliente(servidor);
//	}

	return servidor;

}

//void esperar_cliente(int servidor){
//	struct sockaddr_in direccion_cliente;
//
//	unsigned int tam_direccion = sizeof(struct sockaddr_in);
//
//	int cliente = accept (servidor, (void*) &direccion_cliente, &tam_direccion);
//
//	pthread_create(&thread,NULL,(void*)serve_client,&cliente);
//	pthread_detach(thread);
//}

//void serve_client(int* socket)
//{
//	int cod_op;
//	if(recv(*socket, &cod_op, sizeof(int), MSG_WAITALL) == -1)
//		cod_op = -1;
//	process_request(cod_op, *socket);
//}

int iniciar_cliente(char* ip, char* puerto){
	struct sockaddr_in direccion_servidor;

	direccion_servidor.sin_family = AF_INET;
	direccion_servidor.sin_addr.s_addr = inet_addr(ip);
	direccion_servidor.sin_port = htons(atoi(puerto));

	int cliente = socket(AF_INET, SOCK_STREAM, 0);

	if(connect(cliente, (void*) &direccion_servidor, sizeof(direccion_servidor)) !=0){
		perror("No se pudo conectar");
		return -1;
	}

	return cliente;
}
//sacar de commons y poner en cada proceso especifico?
//void process_request(int cod_op, int cliente_fd) {
//	int size = 0;
//	void* buffer = recibir_mensaje(cliente_fd, &size);
//	uint32_t id = recv(cliente_fd, &id,sizeof(uint32_t),0);
//
//	t_get_pokemon* get_pokemon = malloc(sizeof(t_get_pokemon));
//
//		switch (cod_op) {
//		case GET_POKEMON:
//			get_pokemon = deserializar_get_pokemon(buffer);
//			puts(get_pokemon->nombre.nombre);
//
//			break;
//		case 0:
//			pthread_exit(NULL);
//		case -1:
//			pthread_exit(NULL);
//		}
//}

void* recibir_mensaje(int socket_cliente, int* size)
{
	void * buffer;

	int aux_size = 0;

	recv(socket_cliente, &aux_size, sizeof(int), 0);
	buffer = malloc(aux_size);
	printf("el size del paquete recibido es: %d", aux_size);
	puts("");
	recv(socket_cliente, buffer, aux_size, 0);

	*size = aux_size;

	return buffer;
}

void liberar_conexion(int socket_cliente){

	close(socket_cliente);
}



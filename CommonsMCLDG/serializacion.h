#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include<commons/collections/list.h>
#include "utils.h"

typedef struct{
  op_code tipo_mensaje;
  char **parametros; //ejemplo: ["PARAM1","PARAM2","PARAM3"]
}t_mensaje;


typedef struct{
	uint32_t size;
	void* stream;
} t_buffer;

typedef struct{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef struct{
	uint32_t pos_x;
	uint32_t pos_y;
}coordenadas_pokemon;

typedef struct{
	uint32_t largo_nombre;
	char* nombre;
}nombre_pokemon;

typedef struct{
	coordenadas_pokemon coordenadas;
	nombre_pokemon nombre;
	uint32_t cantidad;
	uint32_t id;
}t_new_pokemon;

typedef struct{
	coordenadas_pokemon coordenadas;
	nombre_pokemon nombre;
	uint32_t id;
	uint32_t correlation_id;
}t_position_and_name;

typedef struct{
	nombre_pokemon nombre;
	uint32_t cantidad;
	t_list* listaCoordenadas;
	uint32_t id;
	uint32_t correlation_id;
}t_localized_pokemon;

typedef struct{
	nombre_pokemon nombre;
	uint32_t id;
}t_get_pokemon;

typedef struct{
	uint32_t caught;
	uint32_t id;
	uint32_t correlation_id;
}t_caught_pokemon;

typedef struct{
	op_code cola;
	int id_proceso;
}t_suscripcion;

typedef struct{
	uint32_t id_mensaje;
	uint32_t id_proceso;
}t_ack;

void enviar_mensaje(t_mensaje* mensaje, int socket);
void* serializar_paquete(t_paquete* paquete, int *bytes);
t_get_pokemon* deserializar_get_pokemon(void* buffer);
void enviar_ack(int socket, uint32_t id, int id_proceso);

t_buffer* cargar_buffer(t_mensaje* mensaje);
t_buffer* buffer_localized_pokemon(char** parametros);
t_buffer* buffer_caught_pokemon(char** parametros);
t_buffer* buffer_position_and_name(char** parametros);
t_buffer* buffer_get_pokemon(char** parametros);
t_buffer* buffer_new_pokemon(char** parametros);
t_buffer* buffer_suscripcion(char** parametros);
t_buffer* buffer_ack(char** parametros);

t_ack* deserializar_ack(void* buffer);
t_get_pokemon* deserializar_get_pokemon(void* buffer);
t_new_pokemon* deserializar_new_pokemon(void* buffer);
t_position_and_name* deserializar_position_and_name(void* buffer);
t_caught_pokemon* deserializar_caught_pokemon(void* buffer);
t_localized_pokemon* deserializar_localized_pokemon(void*buffer);
t_suscripcion* deserializar_suscripcion(void* buffer);





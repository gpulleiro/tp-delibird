#ifndef UTILSGAMEBOY_H_
#define UTILSGAMEBOY_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include<readline/history.h>
#include<../CommonsMCLDG/utils.h>
#include<../CommonsMCLDG/socket.h>

//Config
#define PATH "/home/utnso/workspace/tp-2020-1c-MCLDG/GameBoy/GAMEBOY.config"
#define PROCESOS "PROCESOS"
#define MENSAJES "MENSAJES"
#define FORMATO "FORMATO"

//Defino diferentes procesos
#define BROKER "BROKER"
#define TEAM "TEAM"
#define GAMECARD "GAMECARD"
#define GAMEBOY "GAMEBOY"


//cantidad de argumentos de mensajes
#define ARGUMENTOS_APPEARED_POKEMON 3
#define ARGUMENTOS_NEW_POKEMON 4
#define ARGUMENTOS_CATCH_POKEMON 3
#define ARGUMENTOS_CAUGHT_POKEMON 1
#define ARGUMENTOS_GET_POKEMON 1


//mensajes de errores
#define mensaje_invalido "Por favor ingrese un mensaje valido. Para obtener ayuda ingrese el comando help"
#define argumento_invalido "Argumento invalido"
#define terminar_consola "El proceso GAMEBOY se ha finalizado"

//Defino comandos
//Comandos
#define comando_help "help"
#define comando_exit "exit"

//Mensajes help
#define help_procesos "Los procesos validos son BROKER || TEAM || GAMECARD"
#define help_formato_argumentos "El unico formato valido para ingresar es: [PROCESO] [TIPO_MENSAJE] [ARGUMENTOS]*"
#define help_argumentos "Help admite los siguientes argumentos: \n 1- FORMATO\n 2- PROCESOS "
#define help_mensajes "Las combinaciones de mensajes validas son: ... "

typedef enum{
	NO_TIENE_ID = 0,
	ID_AL_FINAL = 1,
	ID_AL_PRINCIPIO = 2,
}tipo_id;


void iniciar_gameboy(char** argv, int argc);
void iniciar_consola(t_config* config, char** argv, int argc);
void help(char* mensaje);
void liberar_consola(char* proceso, char* mensaje, char** linea_split);

void ejecutar_proceso(char** linea_split, t_config* config, tipo_id flag_id);
void ejecutar_broker(char** linea_split, t_config* config, tipo_id flag_id);
void ejecutar_team(char** linea_split, t_config* config);
void ejecutar_gamecard(char** linea_split, t_config* config, tipo_id flag_id);
void ejecutar_modo_suscriptor(char** linea_split, t_config* config);

bool validar_mensaje(char* proceso, char* mensaje);
bool validar_argumentos(char* tipo_mensaje, char** linea_split, char* proceso, tipo_id *flag_id);
bool verificar_mensaje(char** linea_split, t_config* config, tipo_id* flag_id );


int cantidad_argumentos (char** linea_split);
char** argumentos(char** linea_split, tipo_id flag_id);
char* calcular_id(tipo_id flag_id, char** linea_split);
char* calcular_correlation_id(tipo_id flag_id, char** linea_split);
void recibir_mensajes_de_cola(int* socket);
void process_request(int cod_op, int cliente_fd);
void destruirElemento(void* elemento);


#endif /* UTILS_H_ */

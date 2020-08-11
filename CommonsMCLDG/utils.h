#ifndef UTILSCOMMONS_H_
#define UTILSCOMMONS_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>


//Config
#define IP_BROKER "IP_BROKER"
#define IP_TEAM "IP_TEAM"
#define IP_GAMECARD "IP_GAMECARD"
#define PUERTO_BROKER "PUERTO_BROKER"
#define PUERTO_TEAM "PUERTO_TEAM"
#define PUERTO_GAMECARD "PUERTO_GAMECARD"

//Defino tipo mensajes
#define MENSAJE_NEW_POKEMON "NEW_POKEMON"
#define MENSAJE_APPEARED_POKEMON "APPEARED_POKEMON"
#define MENSAJE_CATCH_POKEMON "CATCH_POKEMON"
#define MENSAJE_CAUGHT_POKEMON "CAUGHT_POKEMON"
#define MENSAJE_GET_POKEMON "GET_POKEMON"
#define MENSAJE_LOCALIZED_POKEMON "LOCALIZED_POKEMON"
#define MENSAJE_MODO_SUSCRIPTOR "SUSCRIPTOR"

//Defino diferentes procesos
#define BROKER "BROKER"
#define TEAM "TEAM"
#define GAMECARD "GAMECARD"
#define GAMEBOY "GAMEBOY"

typedef enum{
	SUSCRIPCION=1,
	NEW_POKEMON=2,
	APPEARED_POKEMON=3,
	CATCH_POKEMON=4,
	CAUGHT_POKEMON=5,
	GET_POKEMON=6,
	LOCALIZED_POKEMON=7,
	ACK=8,
}op_code;


t_log* iniciar_logger(t_config*);
t_config* leer_config(char* proceso);
void terminar_proceso(int, t_log*, t_config*);
void liberar_vector (char** vector);
op_code codigo_mensaje(char* tipo_mensaje);



#endif /* UTILS_H_ */


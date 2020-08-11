/*
 ============================================================================
 Name        : Broker.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */


#include <stdio.h>
#include <stdlib.h>
#include "Broker.h"
#define LOG_FILE "LOG_FILE"
#define LOG_NOMBRE_APLICACION "NOMBRE_APLICACION"
#define BROKER_CONFIG "Broker.config"

t_list* multihilos;

int main() {
	signal(SIGUSR1, dump_cache);

	iniciar_broker();
//	recibir_mensaje_broker(config);
//	terminar_broker(logger,config);
	return EXIT_SUCCESS;
}

void iniciar_broker(){
	config = leer_config(PATH);
	logger = iniciar_logger(config);

	sem_init(&new_pokemon_sem,0,0);
	sem_init(&appeared_pokemon_sem,0,0);
	sem_init(&catch_pokemon_sem,0,0);
	sem_init(&caught_pokemon_sem,0,0);
	sem_init(&localized_pokemon_sem,0,0);
	sem_init(&get_pokemon_sem,0,0);
	sem_init(&suscripcion_sem,0,0);
	sem_init(&ack_sem,0,0);

	crear_queues();

	multihilos = list_create();

	iniciar_memoria();

	pthread_t new_pokemon_thread;
	pthread_create(&new_pokemon_thread, NULL, (void*)ejecutar_new_pokemon, NULL);
	pthread_detach(new_pokemon_thread);

	pthread_t appeared_pokemon_thread;
	pthread_create(&appeared_pokemon_thread, NULL, (void*)ejecutar_appeared_pokemon, NULL);
	pthread_detach(appeared_pokemon_thread);

	pthread_t catch_pokemon_thread;
	pthread_create(&catch_pokemon_thread, NULL, (void*)ejecutar_catch_pokemon, NULL);
	pthread_detach(catch_pokemon_thread);

	pthread_t caught_pokemon_thread;
	pthread_create(&caught_pokemon_thread, NULL, (void*)ejecutar_caught_pokemon, NULL);
	pthread_detach(caught_pokemon_thread);

	pthread_t localized_pokemon_thread;
	pthread_create(&localized_pokemon_thread, NULL, (void*)ejecutar_localized_pokemon, NULL);
	pthread_detach(localized_pokemon_thread);

	pthread_t get_pokemon_thread;
	pthread_create(&get_pokemon_thread, NULL, (void*)ejecutar_get_pokemon, NULL);
	pthread_detach(get_pokemon_thread);

	pthread_t suscripcion_thread;
	pthread_create(&suscripcion_thread, NULL, (void*)ejecutar_suscripcion, NULL);
	pthread_detach(suscripcion_thread);

	pthread_t ack_thread;
	pthread_create(&ack_thread, NULL, (void*)ejecutar_ACK, NULL);
	pthread_detach(ack_thread);

	pthread_t llegada_mensajes;
	pthread_create(&llegada_mensajes, NULL, (void*)socket_mensajes, NULL);
	pthread_join(llegada_mensajes, NULL);

}

void socket_mensajes(){
	char* ip = config_get_string_value(config,IP_BROKER);
	char* puerto = config_get_string_value(config,PUERTO_BROKER);

	int servidor = iniciar_servidor(ip,puerto);

	printf("Se creo el socket servidor en el puerto ( %s )", puerto);
	while(1){
		esperar_cliente(servidor);
	}
}


void terminar_broker(t_log* logger, t_config* config)
{
	config_destroy(config);
	log_destroy(logger);
}




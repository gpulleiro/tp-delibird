/*
 ============================================================================
 Name        : GameCard.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "GameCard.h"


int main(void) {

	config = leer_config(PATH);
	logger_gamecard = iniciar_logger(config);


	crear_tall_grass(config);

	pthread_t suscripciones;
	sem_init(&conexiones, 0,0);
	pthread_create(&suscripciones, NULL, (void*)crear_conexiones, NULL);
	pthread_detach(suscripciones);

	pthread_create(&gameboy, NULL, (void*)socket_gameboy, NULL);
	pthread_join(gameboy, NULL);

	terminar_proceso(0, logger_gamecard, config);

}



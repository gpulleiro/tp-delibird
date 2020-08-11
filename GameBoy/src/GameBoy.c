/*
 ============================================================================
 Name        : GameBoy.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "GameBoy.h"
#define LOG_FILE "LOG_FILE"
#define LOG_NOMBRE_APLICACION "NOMBRE_APLICACION"
#define BROKER_CONFIG "GAMEBOY.config"


t_config* config;
t_log* logger;

int main(int argc, char* argv[]) {


	iniciar_gameboy(argv, argc);

	return EXIT_SUCCESS;
}

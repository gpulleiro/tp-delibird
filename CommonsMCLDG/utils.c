#include "utils.h"

t_log* iniciar_logger(t_config* config)
{
	char* nombre_archivo = config_get_string_value(config,"LOG_FILE");
	char* nombre_aplicacion = config_get_string_value(config,"NOMBRE_APLICACION");
	//char* path = config_get_string_value(config,"PATH");
	int log_consola = config_get_int_value(config,"LOG_CONSOLA");

	t_log* logger = log_create(nombre_archivo,nombre_aplicacion,log_consola,LOG_LEVEL_INFO);
	return logger;
}

t_config* leer_config(char* path)
{
	t_config* config = config_create(path);
	return config;
}

void terminar_proceso(int conexion, t_log* logger, t_config* config)
{
	config_destroy(config);
	//liberar_conexion(conexion);
	log_info(logger,"-----------LOG END--------");
	log_destroy(logger);
}

void liberar_vector (char** vector){

	int i = 0;
	while(vector[i]!=NULL){
		free(vector[i]);
		i++;
	}

	free(vector);
}

op_code codigo_mensaje(char* tipo_mensaje){

	if(string_equals_ignore_case(MENSAJE_NEW_POKEMON, tipo_mensaje)){
		return NEW_POKEMON;
	}else if(string_equals_ignore_case(MENSAJE_APPEARED_POKEMON, tipo_mensaje)){
		return APPEARED_POKEMON;
	}else if(string_equals_ignore_case(MENSAJE_CATCH_POKEMON, tipo_mensaje)){
		return CATCH_POKEMON;
	}else if(string_equals_ignore_case(MENSAJE_CAUGHT_POKEMON, tipo_mensaje)){
			return CAUGHT_POKEMON;
	}else if(string_equals_ignore_case(MENSAJE_GET_POKEMON, tipo_mensaje)){
		return GET_POKEMON;
	}else if(string_equals_ignore_case(MENSAJE_LOCALIZED_POKEMON, tipo_mensaje)){
		return LOCALIZED_POKEMON;
	}else{
		return 0;
	}
}


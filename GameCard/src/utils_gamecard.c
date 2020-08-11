#include "utils_gamecard.h"

//-------------------CREACION DE FS------------------//

void crear_tall_grass(t_config* config){
	pto_montaje = config_get_string_value(config,PUNTO_MONTAJE_TALLGRASS);

	mkdir(pto_montaje, 0777);

	char* path_metadata = string_new();

	string_append(&path_metadata, pto_montaje);
	string_append(&path_metadata,"/Metadata/Metadata.bin");

	t_config* config_metadata = config_create(path_metadata);

	metadata_fs = malloc(sizeof(t_metadata));

	metadata_fs->block_size = config_get_int_value(config_metadata, BLOCK_SIZE);
	metadata_fs->blocks = config_get_int_value(config_metadata, BLOCKS);
	metadata_fs->magic_number = config_get_string_value(config_metadata, MAGIC_NUMBER);

	config_destroy(config_metadata);
	free(path_metadata);

	crear_bitmap(pto_montaje);

	pokemones = list_create();
	pthread_mutex_init(&pokemones_mtx, NULL);

	pthread_mutex_init(&log_mtx, NULL);

	mensajes = list_create();
	pthread_mutex_init(&solicitudes_mtx, NULL);

	pthread_mutex_lock(&log_mtx);
	log_info(logger_gamecard,"Se inició el FS TALL_GRASS");
	pthread_mutex_unlock(&log_mtx);
	//free(metadata_fs);

}

//void crear_metadata(char* punto_montaje){
//
//	char* path_metadata = string_new();
//
//	string_append(&path_metadata, punto_montaje);
//	string_append(&path_metadata,"/Metadata");
//
//	mkdir(path_metadata, 0777);
//
//	string_append(&path_metadata, "/Metadata.bin");
//
//	FILE * metadata = fopen(path_metadata, "w+");
//
//	fprintf(metadata, "BLOCK_SIZE=64\n");
//	fprintf(metadata, "BLOCKS=4096\n");
//	fprintf(metadata, "MAGIC_NUMBER=TALL_GRASS\n");
//
//	fclose(metadata);
//
//	free(path_metadata);
//
//}

void crear_bitmap(char* punto_montaje){

	char* path_bitarray = string_new();

	string_append_with_format(&path_bitarray, "%s/Metadata/Bitmap.bin", punto_montaje);

	int blocks = metadata_fs->blocks/8;

	int bitarray_file = open(path_bitarray, O_RDWR | O_CREAT, 0700);  //uso open porque necesito el int para el mmap

	ftruncate(bitarray_file, blocks);

	char* mapeo_bitarray = mmap(0, blocks, PROT_WRITE | PROT_READ, MAP_SHARED, bitarray_file, 0);

	//ver errores en mapeo

	bitarray = bitarray_create_with_mode(mapeo_bitarray, blocks, LSB_FIRST);
	//
	//	for(int i = 0; i < blocks; i++){
	//		bitarray_clean_bit(bitarray, i);
	//	}

	msync(bitarray, sizeof(bitarray), MS_SYNC);

	pthread_mutex_init(&bitarray_mtx, NULL);

	close(bitarray_file);
	free(path_bitarray);
	pthread_mutex_lock(&log_mtx);
	log_info(logger_gamecard,"Se creó el bitmap");
	pthread_mutex_unlock(&log_mtx);
	//free(path_bitarray);
}

//-------------------ACCIONES DE MENSAJES------------------//

void new_pokemon(t_new_pokemon* pokemon){

	char* path_pokemon = string_new();
	string_append_with_format(&path_pokemon, "%s/Files/%s", pto_montaje, pokemon->nombre.nombre);

	t_mensaje* appeared_pokemon = malloc(sizeof(t_mensaje));
	appeared_pokemon->tipo_mensaje = APPEARED_POKEMON;
	char* parametros = string_new();

	if(existe_pokemon(path_pokemon)){
		actualizar_nuevo_pokemon(pokemon);
	}else{
		crear_pokemon(pokemon);
	}

	string_append_with_format(&parametros, "%s,%d,%d,0,%d", pokemon->nombre.nombre, pokemon->coordenadas.pos_x, pokemon->coordenadas.pos_y, pokemon->id);

	appeared_pokemon->parametros = string_split(parametros, ",");

	puts(parametros);
	int socket_broker = iniciar_cliente(config_get_string_value(config, IP_BROKER), config_get_string_value(config, PUERTO_BROKER));
	if(socket_broker == -1){
		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard,"No se pudo enviar el mensaje APPEARED_POKEMON al Broker, error de conexion");
		pthread_mutex_unlock(&log_mtx);
	}else{
		enviar_mensaje(appeared_pokemon, socket_broker);
		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard,"Se envió el mensaje APPEARED_POKEMON %s de la posicion %d - %d y Correlation-ID: %d",pokemon->nombre.nombre, pokemon->coordenadas.pos_x, pokemon->coordenadas.pos_y, pokemon->id );
		pthread_mutex_unlock(&log_mtx);
		uint32_t id;
		int _recv = recv(socket_broker, &id, sizeof(uint32_t), MSG_WAITALL);
		if(_recv == 0 || _recv == -1){
			pthread_mutex_lock(&log_mtx);
			log_info(logger_gamecard,"No se recibio un ACK del broker", id);
			pthread_mutex_unlock(&log_mtx);
		}

		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard,"El mensaje de ID: %d fue recibido por el Broker", id);
		pthread_mutex_unlock(&log_mtx);
	}
	free(pokemon->nombre.nombre);
	free(pokemon);
	liberar_vector(appeared_pokemon->parametros);
	free(appeared_pokemon);
	free(parametros);
	free(path_pokemon);
	liberar_conexion(socket_broker);
}

void catch_pokemon(t_position_and_name* pokemon){

	char* path_pokemon = string_new();
	string_append_with_format(&path_pokemon, "%s/Files/%s", pto_montaje, pokemon->nombre.nombre);

	t_mensaje* caught_pokemon = malloc(sizeof(t_mensaje));
	caught_pokemon->tipo_mensaje = CAUGHT_POKEMON;
	char* parametros = string_new();

	int resultado = 0; //si ocurre algun error el resultado será 0, si no se cambiará cuando actualice el pokemon

	if(existe_pokemon(path_pokemon)){
		actualizar_quitar_pokemon(pokemon, &resultado);
	}else{
		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard,"No se pudo capturar el pokemon %s porque no existe en el mapa", pokemon->nombre.nombre);
		pthread_mutex_unlock(&log_mtx);
	}

	string_append_with_format(&parametros, "%d,0,%d", resultado, pokemon->id);

	caught_pokemon->parametros = string_split(parametros, ",");

	puts(parametros);
	int socket_broker = iniciar_cliente(config_get_string_value(config, IP_BROKER), config_get_string_value(config, PUERTO_BROKER));
	if(socket_broker == -1){
		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard,"No se pudo enviar el mensaje CAUGHT_POKEMON al Broker, error de conexion");
		pthread_mutex_unlock(&log_mtx);
	}else{
		enviar_mensaje(caught_pokemon, socket_broker);
		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard,"Se envió el mensaje CAUGHT_POKEMON con resultado: %d y Correlation-ID: %d", resultado, pokemon->id);
		pthread_mutex_unlock(&log_mtx);
		uint32_t id;
		int _recv = recv(socket_broker, &id, sizeof(uint32_t), MSG_WAITALL);
		if(_recv == 0 || _recv == -1){
			pthread_mutex_lock(&log_mtx);
			log_info(logger_gamecard,"No se recibio un ACK del broker");
			pthread_mutex_unlock(&log_mtx);
		}

		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard,"El mensaje de ID: %d fue recibido por el Broker", id);
		pthread_mutex_unlock(&log_mtx);
	}
	free(pokemon->nombre.nombre);
	free(pokemon);
	free(parametros);
	free(path_pokemon);
	liberar_vector(caught_pokemon->parametros);
	free(caught_pokemon);
	liberar_conexion(socket_broker);

}

void get_pokemon(t_get_pokemon* pokemon){

	char* path_pokemon = string_new();
	string_append_with_format(&path_pokemon, "%s/Files/%s", pto_montaje, pokemon->nombre.nombre);

	t_mensaje* localized_pokemon = malloc(sizeof(t_mensaje));
	localized_pokemon->tipo_mensaje = LOCALIZED_POKEMON;
	char* parametros = string_new();

	if(existe_pokemon(path_pokemon)){
		free(parametros);
		parametros = obtener_posiciones(pokemon);
		if(parametros == NULL){
			parametros = string_new();
			string_append_with_format(&parametros, "%s,0", pokemon->nombre.nombre);
			pthread_mutex_lock(&log_mtx);
			log_info(logger_gamecard,"No se pudo obtener el pokemon %s porque no existe en el mapa", pokemon->nombre.nombre);
			pthread_mutex_unlock(&log_mtx);
		}
	}else{
		string_append_with_format(&parametros, "%s,0", pokemon->nombre.nombre);
		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard,"No se pudo obtener el pokemon %s porque no existe en el mapa", pokemon->nombre.nombre);
		pthread_mutex_unlock(&log_mtx);
	}

	string_append_with_format(&parametros, ",0,%d", pokemon->id);

	localized_pokemon->parametros = string_split(parametros, ",");

	puts(parametros);
	int socket_broker = iniciar_cliente(config_get_string_value(config, IP_BROKER), config_get_string_value(config, PUERTO_BROKER));
	if(socket_broker == -1){
		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard,"No se pudo enviar el mensaje LOCALIZED_POKEMON al Broker, error de conexion");
		pthread_mutex_unlock(&log_mtx);
	}else{
		enviar_mensaje(localized_pokemon, socket_broker);
		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard,"Se envió el mensaje LOCALIZED_POKEMON %s", parametros);
		pthread_mutex_unlock(&log_mtx);
		uint32_t id;
		int _recv = recv(socket_broker, &id, sizeof(uint32_t), MSG_WAITALL);
		if(_recv == 0 || _recv == -1){
			pthread_mutex_lock(&log_mtx);
			log_info(logger_gamecard,"No se recibio un ACK del broker");
			pthread_mutex_unlock(&log_mtx);
		}

		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard,"El mensaje de ID: %d fue recibido por el Broker", id);
		pthread_mutex_unlock(&log_mtx);
	}
	free(path_pokemon);
	free(pokemon->nombre.nombre);
	free(pokemon);
	liberar_conexion(socket_broker);
	free(parametros);
	liberar_vector(localized_pokemon->parametros);
	free(localized_pokemon);

}

//-------------------QUITAR/AGREGAR/OBTENER POSICIONES------------------//

char* obtener_posiciones(t_get_pokemon* pokemon){
	char* path_pokemon = string_new();
	char* parametros = string_new();

	string_append_with_format(&path_pokemon, "%s/Files/%s/Metadata.bin", pto_montaje, pokemon->nombre.nombre);

	t_pokemon* pokemon_sem = semaforo_pokemon(pokemon->nombre.nombre);

	pthread_mutex_lock(&(pokemon_sem->mtx));
	t_config* config_pokemon = config_create(path_pokemon);

	if(!archivo_abierto(config_pokemon)){

		config_set_value(config_pokemon, OPEN, YES);

		config_save_in_file(config_pokemon, path_pokemon);
		pthread_mutex_unlock(&(pokemon_sem->mtx));

		int tamanio_total = config_get_int_value(config_pokemon, SIZE);

		if(tamanio_total !=0){

			char** blocks = config_get_array_value(config_pokemon, BLOCKS);

			char** datos = leer_archivo(blocks, tamanio_total);

			t_list* lista_datos = transformar_a_lista(datos);

			int cantidad_posiciones = list_size(lista_datos); //la cantidad de posiciones en las que hay al menos 1 pokemon

			string_append_with_format(&parametros, "%s,%d", pokemon->nombre.nombre, cantidad_posiciones);

			char* coordenadas;
			char** aux_coord_cantidad;
			char** aux_coord;

			for(int i = 0; i<cantidad_posiciones; i++){ //+1 porque cantidad cuenta desde 1
				coordenadas = list_get(lista_datos, i);
				aux_coord_cantidad = string_split(coordenadas, "=");
				aux_coord = string_split(aux_coord_cantidad[0], "-");

				string_append_with_format(&parametros, ",%s,%s", aux_coord[0], aux_coord[1]);

				liberar_vector(aux_coord_cantidad);
				liberar_vector(aux_coord);

			}
			list_destroy_and_destroy_elements(lista_datos, (void*)liberar_elemento);
			config_set_value(config_pokemon, OPEN, NO);
			sleep(config_get_int_value(config,TIEMPO_RETARDO_OPERACION));
			guardar_metadata(config_pokemon, path_pokemon, pokemon->nombre.nombre);
			free(datos);
			liberar_vector(blocks);
			config_destroy(config_pokemon);
			free(path_pokemon);

			return parametros;

			//destruir lista y esas cosas (?
		}else{
			config_set_value(config_pokemon, OPEN, NO);
			sleep(config_get_int_value(config,TIEMPO_RETARDO_OPERACION));
			guardar_metadata(config_pokemon, path_pokemon, pokemon->nombre.nombre);
			free(parametros);
			free(path_pokemon);
			config_destroy(config_pokemon);
			return NULL;
		}
	}else{
		pthread_mutex_unlock(&(pokemon_sem->mtx));
		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard,"No se pudieron leer los datos de %s ya que el archivo está en uso", pokemon->nombre.nombre);
		pthread_mutex_unlock(&log_mtx);
		sleep(config_get_int_value(config,TIEMPO_DE_REINTENTO_OPERACION));
		obtener_posiciones(pokemon);
	}
}

void crear_pokemon(t_new_pokemon* pokemon){
	char* path_pokemon = string_new();
	string_append_with_format(&path_pokemon, "%s/Files/%s", pto_montaje, pokemon->nombre.nombre);

	mkdir(path_pokemon, 0777); //creo el directorio

	string_append(&path_pokemon, "/Metadata.bin");

	FILE* metadata = fopen(path_pokemon, "w+"); //creo su metadata

	t_pokemon* pokemon_sem = malloc(sizeof(t_pokemon));

	pokemon_sem->nombre = malloc(pokemon->nombre.largo_nombre+1);
	strcpy(pokemon_sem->nombre, pokemon->nombre.nombre);

	pthread_mutex_init(&(pokemon_sem->mtx), NULL);

	pthread_mutex_lock(&pokemones_mtx);
	list_add(pokemones, pokemon_sem);
	pthread_mutex_unlock(&pokemones_mtx);

	pthread_mutex_lock(&(pokemon_sem->mtx));

	fprintf(metadata, "DIRECTORY=N\n"); //no lo escribe esto???????
	fprintf(metadata, "OPEN=Y\n"); //lo marco como abierto
	fclose(metadata);

	pthread_mutex_unlock(&(pokemon_sem->mtx));

	char* datos = string_new();
	string_append_with_format(&datos, "%d-%d=%d\0", pokemon->coordenadas.pos_x, pokemon->coordenadas.pos_y, pokemon->cantidad);

	int tamanio = strlen(datos);

	int cantidad_bloques = ceil((float) tamanio / (float) metadata_fs->block_size);

	char** bloques_a_escribir = buscar_bloques_libres(cantidad_bloques);

	//no necesito semaforo porque ya lo marque como abierto, entonces no puede entrar otro hilo al metadata

	FILE* actualizar_metadata = fopen(path_pokemon, "r+");

	fprintf(actualizar_metadata, "BLOCKS=[");

	int j;

	for(j = 0; j < (cantidad_bloques - 1); j++){
		fprintf(actualizar_metadata, "%s", bloques_a_escribir[j]);
		fprintf(actualizar_metadata, ",");
	}

	fprintf(actualizar_metadata, "%s", bloques_a_escribir[j]); //imprimo el ultimo sin la coma
	fprintf(actualizar_metadata, "]\n");

	fclose(actualizar_metadata);

	t_config* config_aux = config_create(path_pokemon);

	pthread_mutex_lock(&log_mtx);
	log_info(logger_gamecard,"Se asignaron los bloques %s al pokemon %s con tamanio de %d", config_get_string_value(config_aux, BLOCKS), pokemon->nombre.nombre, tamanio);
	pthread_mutex_unlock(&log_mtx);

	char* aux_tamanio = string_itoa(tamanio);
	config_set_value(config_aux, SIZE, aux_tamanio);
	free(aux_tamanio);

	int offset = 0;

	int i;

	for(i = 0; i < cantidad_bloques; i++){
		escribir_bloque(&offset, datos, bloques_a_escribir[i], &tamanio);
	}

	config_set_value(config_aux, OPEN, NO);
	sleep(config_get_int_value(config,TIEMPO_RETARDO_OPERACION));
	guardar_metadata(config_aux, path_pokemon, pokemon->nombre.nombre);

	free(bloques_a_escribir);
	config_destroy(config_aux);
	free(path_pokemon);
	free(datos);

	pthread_mutex_lock(&log_mtx);
	log_info(logger_gamecard,"Se creo el pokemon %s", pokemon->nombre.nombre);
	pthread_mutex_unlock(&log_mtx);
}

void actualizar_nuevo_pokemon(t_new_pokemon* pokemon){

	char* path_pokemon = string_new();
	string_append_with_format(&path_pokemon, "%s/Files/%s/Metadata.bin", pto_montaje, pokemon->nombre.nombre);
	t_pokemon* pokemon_sem = semaforo_pokemon(pokemon->nombre.nombre);

	pthread_mutex_lock(&(pokemon_sem->mtx));
	t_config* config_pokemon = config_create(path_pokemon);

	if(!archivo_abierto(config_pokemon)){

		config_set_value(config_pokemon, OPEN, YES);

		config_save_in_file(config_pokemon, path_pokemon);
		pthread_mutex_unlock(&(pokemon_sem->mtx));

		char* posicion = string_new();

		string_append_with_format(&posicion, "%d-%d", pokemon->coordenadas.pos_x, pokemon->coordenadas.pos_y);

		int tamanio_total = config_get_int_value(config_pokemon, SIZE);

		if(tamanio_total != 0){

			char** blocks = config_get_array_value(config_pokemon, BLOCKS);

			char** datos = leer_archivo(blocks, tamanio_total);

			t_list* lista_datos = transformar_a_lista(datos);

			t_config* config_datos = transformar_a_config(datos);

			if(config_has_property(config_datos, posicion)){
				char* nueva_cantidad_posicion = string_itoa(config_get_int_value(config_datos, posicion) + pokemon->cantidad); //a la cantidad que ya hay, le sumo el nuevo pokemon

				int i = 0;

				while(!(comienza_con(posicion, list_get(lista_datos, i)))) {  //Se que existe la posicion, entonces recorro hasta encontrarla
					i++; 													 //al salir del while me queda el valor de i como la posicion de la lista que quiero cambiar
				}

				string_append_with_format(&posicion, "=%s", nueva_cantidad_posicion);

				list_replace(lista_datos, i, posicion); //posicion ahora es un string completo (posicion = cantidad)
				free(nueva_cantidad_posicion);

			}else{
				string_append_with_format(&posicion, "=%d",pokemon->cantidad); //si no tiene pokemones en esa posicion, cargo solamente 1 (el pokemon nuevo)
				list_add(lista_datos, posicion);
			}

			guardar_archivo(lista_datos, config_pokemon, path_pokemon, pokemon->nombre.nombre);
			config_destroy(config_datos);
			list_destroy_and_destroy_elements(lista_datos, (void*)liberar_elemento);
			liberar_vector(blocks);
			free(datos);
		}else{
			t_list* lista_datos = list_create();
			string_append_with_format(&posicion, "=%d",pokemon->cantidad); //si no tiene pokemones en esa posicion, cargo solamente 1 (el pokemon nuevo)
			list_add(lista_datos, posicion);
			guardar_archivo(lista_datos, config_pokemon, path_pokemon, pokemon->nombre.nombre);
			list_destroy_and_destroy_elements(lista_datos, (void*)liberar_elemento);
		}

		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard,"Se asignaron los bloques %s al pokemon %s con tamanio de %d", config_get_string_value(config_pokemon, BLOCKS), pokemon->nombre.nombre,config_get_int_value(config_pokemon, SIZE) );
		pthread_mutex_unlock(&log_mtx);

		config_destroy(config_pokemon);
		free(path_pokemon);

	}else{
		pthread_mutex_unlock(&(pokemon_sem->mtx));
		free(path_pokemon);
		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard,"No se pudieron leer los datos de %s ya que el archivo está en uso", pokemon->nombre.nombre);
		pthread_mutex_unlock(&log_mtx);
		sleep(config_get_int_value(config,TIEMPO_DE_REINTENTO_OPERACION));
		actualizar_nuevo_pokemon(pokemon);
	}
}

void actualizar_quitar_pokemon(t_position_and_name* pokemon, int* resultado){

	char* path_pokemon = string_new();
	string_append_with_format(&path_pokemon, "%s/Files/%s/Metadata.bin", pto_montaje, pokemon->nombre.nombre);

	t_pokemon* pokemon_sem = semaforo_pokemon(pokemon->nombre.nombre);

	pthread_mutex_lock(&(pokemon_sem->mtx));
	t_config* config_pokemon = config_create(path_pokemon);

	if(!archivo_abierto(config_pokemon)){

		config_set_value(config_pokemon, OPEN, YES);

		config_save_in_file(config_pokemon, path_pokemon);
		pthread_mutex_unlock(&(pokemon_sem->mtx));

		int tamanio_total = config_get_int_value(config_pokemon, SIZE);
		char** blocks = config_get_array_value(config_pokemon, BLOCKS);

		if(tamanio_total != 0 || blocks == NULL){

			char** datos = leer_archivo(blocks, tamanio_total);

			t_list* lista_datos = transformar_a_lista(datos);

			t_config* config_datos = transformar_a_config(datos);

			char* posicion = string_new();

			string_append_with_format(&posicion, "%d-%d", pokemon->coordenadas.pos_x, pokemon->coordenadas.pos_y);

			if(config_has_property(config_datos, posicion)){

				int i = 0;

				while(!(comienza_con(posicion, list_get(lista_datos, i)))) {  //Se que existe la posicion, entonces recorro hasta encontrarla
					i++; 													 //al salir del while me queda el valor de i como la posicion de la lista que quiero cambiar
				}

				char* nueva_cantidad_posicion = string_itoa(config_get_int_value(config_datos, posicion) - 1);

				if(string_equals_ignore_case(nueva_cantidad_posicion, "0")){
					char* removido = list_remove(lista_datos, i);
					free(removido);
				}else{
					string_append_with_format(&posicion, "=%s", nueva_cantidad_posicion);

					char* valor_viejo = list_replace(lista_datos, i, posicion); //posicion ahora es un string completo (posicion = cantidad)

					free(valor_viejo);
				}
				free(nueva_cantidad_posicion);
				*resultado = 1;
				guardar_archivo(lista_datos, config_pokemon, path_pokemon, pokemon->nombre.nombre);
				free(posicion);

			}else{
				free(posicion);
				pthread_mutex_lock(&log_mtx);
				log_info(logger_gamecard, "No se pudo capturar el pokemon %s ya que no existe en la posicion %d - %d", pokemon->nombre.nombre, pokemon->coordenadas.pos_x, pokemon->coordenadas.pos_y);
				pthread_mutex_unlock(&log_mtx);

				config_set_value(config_pokemon, OPEN, NO);

				sleep(config_get_int_value(config,TIEMPO_RETARDO_OPERACION));

				guardar_metadata(config_pokemon, path_pokemon, pokemon->nombre.nombre);
			}

			list_destroy_and_destroy_elements(lista_datos, (void*)liberar_elemento);
			free(datos);
			free(blocks);
			config_destroy(config_datos);
			config_destroy(config_pokemon);
		}else{
			pthread_mutex_lock(&log_mtx);
			log_info(logger_gamecard, "No se pudo capturar el pokemon %s ya que no existe en la posicion %d - %d", pokemon->nombre.nombre, pokemon->coordenadas.pos_x, pokemon->coordenadas.pos_y);
			pthread_mutex_unlock(&log_mtx);

			config_set_value(config_pokemon, OPEN, NO);

			sleep(config_get_int_value(config,TIEMPO_RETARDO_OPERACION));

			guardar_metadata(config_pokemon, path_pokemon, pokemon->nombre.nombre);
			config_destroy(config_pokemon);
		}

		free(path_pokemon);

	}else{
		pthread_mutex_unlock(&(pokemon_sem->mtx));
		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard,"No se pudieron leer los datos de %s ya que el archivo está en uso", pokemon->nombre.nombre);
		pthread_mutex_unlock(&log_mtx);
		sleep(config_get_int_value(config,TIEMPO_DE_REINTENTO_OPERACION));
		actualizar_quitar_pokemon(pokemon, resultado);
	}
}

void liberar_elemento(void* elemento){
	free(elemento);
}

//-------------------LEER/GUARDAR ARCHIVOS------------------//

void guardar_archivo(t_list* lista_datos, t_config* config_pokemon, char* path_pokemon, char* nombre_pokemon){

	//cantidad de bloques = tamanio real en bytes / tamanio de cada bloque redondeado hacia arriba

	int cantidad_bloques_antes = ceil((float)config_get_int_value(config_pokemon, SIZE) / (float)metadata_fs->block_size);
	int tamanio_nuevo;

	if(list_is_empty(lista_datos)){
		tamanio_nuevo = 0;

	}else{
		tamanio_nuevo = list_fold(lista_datos, 0, (void*) calcular_tamanio); //el -1 porque el ultimo elemento (ultima linea del archivo) no tiene \n
	}

	int cantidad_bloques_actuales = ceil((float)tamanio_nuevo / (float)metadata_fs->block_size);

	int offset = 0;

	char* datos = transformar_a_dato(lista_datos, tamanio_nuevo);

	char** bloques = config_get_array_value(config_pokemon, BLOCKS);

	char* bloques_guardar = string_new();
	string_append(&bloques_guardar, "[");

	if(cantidad_bloques_antes <= cantidad_bloques_actuales){ //si tienen el mismo tamanio, solo vuelvo a copiar los datos en los mismos bloques
		int i;

		for(i = 0; i < cantidad_bloques_antes -1; i++){
			escribir_bloque(&offset, datos, bloques[i], &tamanio_nuevo);
			string_append_with_format(&bloques_guardar, "%s,", bloques[i]);
		}

		if(bloques[0] != NULL){
			escribir_bloque(&offset, datos, bloques[i], &tamanio_nuevo);
			string_append_with_format(&bloques_guardar, "%s", bloques[i]);
		}

		if(cantidad_bloques_antes < cantidad_bloques_actuales){ //si es mayor tamanio tengo que pedir mas bloques
			if(bloques[0] != NULL) string_append(&bloques_guardar, ",");

			int bloques_a_pedir = cantidad_bloques_actuales - cantidad_bloques_antes;
			char** bloques_nuevos = buscar_bloques_libres(bloques_a_pedir); //falta verificar ver error si no hay disponibles

			int j;

			for(j = 0; j < bloques_a_pedir -1;j++){
				escribir_bloque(&offset, datos, bloques_nuevos[j], &tamanio_nuevo);
				string_append_with_format(&bloques_guardar, "%s,", bloques_nuevos[j]);
			}
			escribir_bloque(&offset, datos, bloques_nuevos[j], &tamanio_nuevo);
			string_append_with_format(&bloques_guardar, "%s", bloques_nuevos[j]);

			free(bloques_nuevos);
		}

	}else{ //tengo que borrar bloques

		int bloques_a_borrar = cantidad_bloques_antes - cantidad_bloques_actuales;

		int k = 0;
		for(k = 0; k < cantidad_bloques_actuales -1; k++){
			escribir_bloque(&offset, datos, bloques[k], &tamanio_nuevo);
			string_append_with_format(&bloques_guardar, "%s,", bloques[k]);
		}

		if(cantidad_bloques_actuales != 0){
			escribir_bloque(&offset, datos, bloques[k], &tamanio_nuevo);
			string_append_with_format(&bloques_guardar, "%s", bloques[k]);
			k++;
		}
		while(bloques_a_borrar!=0){
			char* aux_borrar = string_new();
			pthread_mutex_lock(&bitarray_mtx);
			bitarray_clean_bit(bitarray,atoi(bloques[k]));
			msync(bitarray, sizeof(bitarray), MS_SYNC);
			pthread_mutex_unlock(&bitarray_mtx);

			string_append_with_format(&aux_borrar,"%s/Blocks/%s.bin", pto_montaje, bloques[k]);
			remove(aux_borrar);
			k++;
			bloques_a_borrar--;
			free(aux_borrar);
		}

	}

	string_append(&bloques_guardar, "]");

	char* aux_offset = string_itoa(offset);

	config_set_value(config_pokemon, SIZE, aux_offset);

	free(aux_offset);

	config_set_value(config_pokemon, OPEN, NO);

	config_set_value(config_pokemon, BLOCKS, bloques_guardar);

	sleep(config_get_int_value(config,TIEMPO_RETARDO_OPERACION));

	guardar_metadata(config_pokemon, path_pokemon, nombre_pokemon );

	liberar_vector(bloques);
	free(datos);
	free(bloques_guardar);

}

void escribir_bloque(int* offset, char* datos, char* bloque, int* tamanio){

	char* path_blocks = string_new();
	string_append_with_format(&path_blocks, "%s/Blocks/%s.bin", pto_montaje, bloque);

	int tamanio_a_escribir = minimo_entre(metadata_fs->block_size, *tamanio); //si es mayor o igual al block_size escribo el bloque entero, si es menor escribo los bytes que quedan por escribir

	FILE* fd_bloque = fopen(path_blocks, "w+");

	fseek(fd_bloque, 0, SEEK_SET);

	fwrite(datos + *offset, tamanio_a_escribir, 1, fd_bloque);
	*offset += tamanio_a_escribir;
	*tamanio -= tamanio_a_escribir;

	fclose(fd_bloque);
	free(path_blocks);
}

char** leer_archivo(char** blocks, int tamanio_total){ //para leer el archivo, si su tamanio es mayor a block_size del metadata tall grass, leo esa cantidad, si no leo el tamanio que tiene

	char* path_blocks = string_new();

	string_append_with_format(&path_blocks, "%s/Blocks/", pto_montaje);

	int i;

	int cantidad = cantidad_bloques(blocks);

	char* datos = malloc(tamanio_total + 1);

	int offset = 0;

	for(i = 0; i < cantidad; i++){

		//armo el path de cada bloque que voy a leer
		char* bloque_especifico = string_new();
		string_append_with_format(&bloque_especifico, "%s%s.bin", path_blocks, blocks[i]);

		FILE* bloque = fopen(bloque_especifico, "r");

		if(tamanio_total > metadata_fs->block_size){
			fread(datos + offset, sizeof(char), metadata_fs->block_size, bloque);
			offset+= metadata_fs->block_size;
			tamanio_total -= metadata_fs->block_size;

		}else{
			fread(datos + offset, sizeof(char), tamanio_total, bloque); //si no esta lleno entonces es el ultimo bloque -> no necesito cambiar el tamanio ni el offset???
			offset+= tamanio_total;
		}

		fclose(bloque);
		free(bloque_especifico);

	}
	datos[offset] = '\0';

	free(path_blocks);

	char** array_datos = string_split(datos, "\n");

	free(datos);

	return array_datos;
}

//-------------------BLOQUES------------------//

char** buscar_bloques_libres(int cantidad){
	char** bloques = malloc(cantidad*sizeof(int));

	for(int i = 0; i < cantidad; i++){
		int aux = bloque_libre();
		if(aux == -1){
			perror("No se encontró bloque libre");
		}else{

			char* string_aux = string_itoa(aux);
			bloques[i] = string_new();
			string_append(&bloques[i], string_aux);
			free(string_aux);
		}
	}
	return bloques;
}

int bloque_libre(){
	for(int i = 0; i < metadata_fs->blocks; i++){
		if(!bitarray_test_bit(bitarray,i)){
			pthread_mutex_lock(&bitarray_mtx);
			bitarray_set_bit(bitarray,i);
			msync(bitarray, sizeof(bitarray), MS_SYNC);
			pthread_mutex_unlock(&bitarray_mtx);
			return i;
		}
	}
	pthread_mutex_lock(&log_mtx);
	log_info(logger_gamecard,"No existen bloques libres");
	pthread_mutex_unlock(&log_mtx);
	return -1;
}

int cantidad_bloques(char** blocks){
	int i = 0;
	while(blocks[i]!=NULL){
		i++;
	}
	return i;
}

//-------------------TRANSFORMAR DATOS------------------//

char* transformar_a_dato(t_list* lista_datos, int tamanio){
	char* datos = string_new();

	int cantidad_lineas = list_size(lista_datos); //la ultima linea no tiene \n

	int i;

	for (i=0; i < cantidad_lineas; i++){
		string_append_with_format(&datos, "%s\n", list_get(lista_datos, i));
	}

	//	if(!list_is_empty(lista_datos)){
	//	string_append(&datos, list_get(lista_datos, i)); //ultima linea
	//	}
	datos[tamanio] = '\0';

	return datos;
}

t_list* transformar_a_lista(char** lineas){
	int i = 0;
	t_list* lista_datos = list_create();

	while(lineas[i]!=NULL){
		list_add(lista_datos, lineas[i]);
		i++;
	}

	return lista_datos;
}

t_config* transformar_a_config(char** lineas){
	t_config* config_datos = config_create(pto_montaje);

	int i = 0;

	while(lineas[i]!=NULL){ //por cada "posicion", hay una linea del vector "lineas" (separado por \n)
		char** key_valor = string_split(lineas[i], "=");
		config_set_value(config_datos, key_valor[0], key_valor[1]); //separo la posicion de la cantidad (a traves del =)y seteo como key la posicion con su valor cantidad
		liberar_vector(key_valor);
		i++;
	}
	return config_datos;
}

//-------------------ABRIR/CERRAR ARCHIVOS------------------//

void guardar_metadata(t_config* config_archivo, char* path_pokemon, char* nombre_pokemon){

	t_pokemon* pokemon_sem = semaforo_pokemon(nombre_pokemon);

	pthread_mutex_lock(&(pokemon_sem->mtx));
	config_save_in_file(config_archivo, path_pokemon);
	pthread_mutex_unlock(&(pokemon_sem->mtx));

}

t_pokemon* semaforo_pokemon(char* nombre){

	bool _es_pokemon(t_pokemon* _pokemon){
		return string_equals_ignore_case(_pokemon->nombre, nombre);
	}

	pthread_mutex_lock(&pokemones_mtx);
	t_pokemon* pokemon_sem = list_find(pokemones, (void*) _es_pokemon);
	pthread_mutex_unlock(&pokemones_mtx);

	if(pokemon_sem == NULL){
		t_pokemon* new_pokemon = malloc(sizeof(t_pokemon));
		new_pokemon->nombre = malloc(strlen(nombre)+ 1);
		strcpy(new_pokemon->nombre, nombre);
		pthread_mutex_init(&(new_pokemon->mtx), NULL);
		pthread_mutex_lock(&pokemones_mtx);
		list_add(pokemones, new_pokemon);
		pthread_mutex_unlock(&pokemones_mtx);
		return new_pokemon;
	}

	return pokemon_sem;
}

bool archivo_abierto(t_config* config_pokemon){


	char* archivo_open = config_get_string_value(config_pokemon, OPEN);
	bool esta_abierto = string_equals_ignore_case(YES, archivo_open);

	return esta_abierto;
}

//-------------------CONEXIONES------------------//

void crear_conexiones(){
	int tiempoReconexion = config_get_int_value(config, "TIEMPO_DE_REINTENTO_CONEXION");
	pthread_t new_pokemon_thread;
	pthread_t catch_pokemon_thread;
	pthread_t get_pokemon_thread;
	while(1){
		pthread_create(&new_pokemon_thread,NULL,(void*)connect_new_pokemon,NULL);
		pthread_detach(new_pokemon_thread);
		pthread_create(&catch_pokemon_thread,NULL,(void*)connect_catch_pokemon,NULL);
		pthread_detach(catch_pokemon_thread);
		pthread_create(&get_pokemon_thread,NULL,(void*)connect_get_pokemon,NULL);
		pthread_detach(get_pokemon_thread);
		sem_wait(&conexiones);
		sem_wait(&conexiones);
		sem_wait(&conexiones);
		sleep(tiempoReconexion);
		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard, "Inicio Reintento de todas las conexiones");
		pthread_mutex_unlock(&log_mtx);

	}
	sem_destroy(&conexiones);
	return;
}

void connect_new_pokemon(){

	int socket_broker = iniciar_cliente_gamecard(config_get_string_value(config, "IP_BROKER"),config_get_string_value(config, "PUERTO_BROKER"));
	op_code codigo_operacion = SUSCRIPCION;
	t_mensaje* mensaje = malloc(sizeof(t_mensaje));

	int id_proceso = config_get_int_value(config, "ID_PROCESO");

	char* parametros[2] = {"NEW_POKEMON", string_itoa(id_proceso)};
	mensaje -> tipo_mensaje = codigo_operacion;
	mensaje -> parametros = parametros;

	enviar_mensaje(mensaje, socket_broker);

	free(mensaje);

	pthread_mutex_lock(&log_mtx);
	log_info(logger_gamecard, "Se suscribio a la cola NEW_POKEMON");
	pthread_mutex_unlock(&log_mtx);

	int size = 0;
	int cod_op;

	t_new_pokemon* _new_pokemon;

	while(1){

		if(recv(socket_broker, &cod_op, sizeof(int), MSG_WAITALL) == 0){
			pthread_mutex_lock(&log_mtx);
			log_info(logger_gamecard,"Se ha perdido la conexion con el proceso Broker");
			pthread_mutex_unlock(&log_mtx);
			liberar_conexion(socket_broker);
			sem_post(&conexiones);
			pthread_exit(NULL);
		}
		void* buffer = recibir_mensaje(socket_broker,&size);

		pthread_t solicitud_mensaje;

		if(cod_op == NEW_POKEMON){

			_new_pokemon = deserializar_new_pokemon(buffer);
			free(buffer);
			uint32_t id_ack = _new_pokemon->id;

			enviar_ack(socket_broker, id_ack, id_proceso);

			pthread_mutex_lock(&solicitudes_mtx);
			list_add(mensajes, &solicitud_mensaje);
			pthread_mutex_unlock(&solicitudes_mtx);

			pthread_create(&solicitud_mensaje, NULL, (void*)new_pokemon, _new_pokemon);
		}
	}

	liberar_conexion(socket_broker);

}

void connect_catch_pokemon(){
	int socket_broker = iniciar_cliente_gamecard(config_get_string_value(config, "IP_BROKER"),config_get_string_value(config, "PUERTO_BROKER"));
	op_code codigo_operacion = SUSCRIPCION;
	t_mensaje* mensaje = malloc(sizeof(t_mensaje));

	int id_proceso = config_get_int_value(config, "ID_PROCESO");

	char* parametros[2] = {"CATCH_POKEMON", string_itoa(id_proceso)};
	mensaje -> tipo_mensaje = codigo_operacion;
	mensaje -> parametros = parametros;

	enviar_mensaje(mensaje, socket_broker);

	free(mensaje);

	pthread_mutex_lock(&log_mtx);
	log_info(logger_gamecard, "Se suscribio a la cola CATCH_POKEMON");
	pthread_mutex_unlock(&log_mtx);

	int size = 0;
	int cod_op;
	t_position_and_name* _catch_pokemon;

	while(1){

		if(recv(socket_broker, &cod_op, sizeof(int), MSG_WAITALL) == 0){
			pthread_mutex_lock(&log_mtx);
			log_info(logger_gamecard,"Se ha perdido la conexion con el proceso Broker");
			pthread_mutex_unlock(&log_mtx);
			liberar_conexion(socket_broker);
			sem_post(&conexiones);
			pthread_exit(NULL);
		}
		void* buffer = recibir_mensaje(socket_broker,&size);
		pthread_t solicitud_mensaje;
		puts("recibi mensaje");
		if(cod_op == CATCH_POKEMON){
			_catch_pokemon = deserializar_position_and_name(buffer);

			free(buffer);
			uint32_t id_ack = _catch_pokemon->id;

			enviar_ack(socket_broker, id_ack, id_proceso);

			pthread_mutex_lock(&solicitudes_mtx);
			list_add(mensajes, &solicitud_mensaje);
			pthread_mutex_unlock(&solicitudes_mtx);

			pthread_create(&solicitud_mensaje, NULL, (void*)catch_pokemon, _catch_pokemon);
		}
	}

	liberar_conexion(socket_broker);
}

void connect_get_pokemon(){

	int socket_broker = iniciar_cliente_gamecard(config_get_string_value(config, "IP_BROKER"),config_get_string_value(config, "PUERTO_BROKER"));
	op_code codigo_operacion = SUSCRIPCION;
	t_mensaje* mensaje = malloc(sizeof(t_mensaje));

	int id_proceso = config_get_int_value(config, "ID_PROCESO");

	char* id_proceso_char = string_itoa(id_proceso);

	char* parametros[2] = {"GET_POKEMON",id_proceso_char };
	mensaje -> tipo_mensaje = codigo_operacion;
	mensaje -> parametros = parametros;

	enviar_mensaje(mensaje, socket_broker);

	free(id_proceso_char);
	free(mensaje);

	pthread_mutex_lock(&log_mtx);
	log_info(logger_gamecard, "Se suscribio a la cola GET_POKEMON");
	pthread_mutex_unlock(&log_mtx);

	int size = 0;
	t_get_pokemon* _get_pokemon;

	while(1){
		int cod_op;
		if(recv(socket_broker, &cod_op, sizeof(op_code), MSG_WAITALL) == 0){
			pthread_mutex_lock(&log_mtx);
			log_info(logger_gamecard,"Se ha perdido la conexion con el proceso Broker");
			pthread_mutex_unlock(&log_mtx);
			liberar_conexion(socket_broker);
			sem_post(&conexiones);
			pthread_exit(NULL);
		}


		void* buffer = recibir_mensaje(socket_broker,&size);

		pthread_t solicitud_mensaje;

		if(cod_op == GET_POKEMON){
			_get_pokemon = deserializar_get_pokemon(buffer);
			free(buffer);
			int id_proceso = config_get_int_value(config, "ID_PROCESO");

			uint32_t id_ack = _get_pokemon->id;

			enviar_ack(socket_broker, id_ack, id_proceso);

			pthread_mutex_lock(&solicitudes_mtx);
			list_add(mensajes, &solicitud_mensaje);
			pthread_mutex_unlock(&solicitudes_mtx);

			pthread_create(&solicitud_mensaje, NULL, (void*)get_pokemon, _get_pokemon);

			puts("recibio mensaje get pokemon");
		}
	}

	liberar_conexion(socket_broker);

	//	free(mensaje -> parametros);
	//	free(mensaje);
}

int iniciar_cliente_gamecard(char* ip, char* puerto){
	struct sockaddr_in direccion_servidor;

	direccion_servidor.sin_family = AF_INET;
	direccion_servidor.sin_addr.s_addr = inet_addr(ip);
	direccion_servidor.sin_port = htons(atoi(puerto));

	int cliente = socket(AF_INET, SOCK_STREAM, 0);

	if(connect(cliente, (void*) &direccion_servidor, sizeof(direccion_servidor)) !=0){
		pthread_mutex_lock(&log_mtx);
		log_info(logger_gamecard, "No se pudo realizar la conexion");
		pthread_mutex_unlock(&log_mtx);
		liberar_conexion(cliente);
		sem_post(&conexiones);
		pthread_exit(NULL);
	}

	pthread_mutex_lock(&log_mtx);
	log_info(logger_gamecard,"Se ha establecido una conexion con el proceso Broker");
	pthread_mutex_unlock(&log_mtx);
	return cliente;
}

//-------------------CONEXION GAMEBOY------------------//

void socket_gameboy(){

	socket_escucha(config_get_string_value(config, "IP_GAMECARD"), config_get_string_value(config, "PUERTO_GAMECARD"));

}

void socket_escucha(char*IP, char* Puerto){
	int servidor = iniciar_servidor(IP,Puerto);
	while(1){
		esperar_cliente(servidor);
	}

}

void esperar_cliente(int servidor){

	struct sockaddr_in direccion_cliente;

	unsigned int tam_direccion = sizeof(struct sockaddr_in);

	int cliente = accept (servidor, (void*) &direccion_cliente, &tam_direccion);

	pthread_create(&thread,NULL,(void*)serve_client,&cliente);
	pthread_detach(thread);
}

void serve_client(int* socket)
{
	int cod_op;
	if(recv(*socket, &cod_op, sizeof(int), MSG_WAITALL) == -1)
		cod_op = -1;
	process_request(cod_op, *socket);
}

void process_request(int cod_op, int cliente_fd) {
	int size = 0;
	void* buffer = recibir_mensaje(cliente_fd, &size);

	t_new_pokemon* _new_pokemon;
	t_position_and_name* _catch_pokemon;
	t_get_pokemon* _get_pokemon;

	int id_proceso = config_get_int_value(config, "ID_PROCESO");

	uint32_t id_ack;

	switch (cod_op) {
	case NEW_POKEMON:
		_new_pokemon = deserializar_new_pokemon(buffer);
		free(buffer);
		id_ack = _new_pokemon->id;
		enviar_ack(cliente_fd, id_ack, id_proceso);

		new_pokemon(_new_pokemon);
		break;
	case CATCH_POKEMON:
		_catch_pokemon = deserializar_position_and_name(buffer);
		free(buffer);
		id_ack = _catch_pokemon->id;
		enviar_ack(cliente_fd, id_ack, id_proceso);

		catch_pokemon(_catch_pokemon);
		break;
	case GET_POKEMON:
		_get_pokemon = deserializar_get_pokemon(buffer);
		free(buffer);
		id_ack = _get_pokemon->id;
		enviar_ack(cliente_fd, id_ack, id_proceso);

		get_pokemon(_get_pokemon);
		break;
	case 0:
		pthread_exit(NULL);
	case -1:
		pthread_exit(NULL);
	}
}

//-------------------AUXILIARES------------------//

int minimo_entre (int nro1, int nro2){
	if (nro1 >= nro2){
		return nro2;
	}
	return nro1;
}

int calcular_tamanio(int acc, char* linea){ //func para el fold en guardar archivo

	int tamanio_linea = strlen(linea) + 1;

	return acc + tamanio_linea;
}

bool comienza_con(char* posicion, char* linea){

	char** posicion_cantidad = string_split(linea, "=");

	bool es_la_posicion = string_equals_ignore_case(posicion_cantidad[0], posicion);

	liberar_vector(posicion_cantidad);

	return es_la_posicion;
}

bool existe_pokemon(char* path_pokemon){

	DIR* verificacion = opendir(path_pokemon);

	bool existe = !(verificacion == NULL); //si al abrirlo devuelve NULL, el directorio no existe

	free(verificacion);
	return existe;
}



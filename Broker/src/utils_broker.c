
#include "utils_broker.h"

extern t_list* multihilos;

int proceso_valido(char*procesos_validos,char* proceso){

	char* s = strstr(procesos_validos,proceso);

	if(s != NULL) return 1;
	return 0;
}

//void log_suscribir_mensaje_queue(char* proceso,char* queue){
//	pthread_mutex_lock(&logger_mutex);
//	log_info(logger,"Proceso: %s se suscribio a la cola: %s", proceso, queue);
//	pthread_mutex_unlock(&logger_mutex);
//}

void crear_queues(void){
	unique_message_id = 0;

	NEW_POKEMON_QUEUE = list_create();
	APPEARED_POKEMON_QUEUE = list_create();
	CATCH_POKEMON_QUEUE = list_create();
	CAUGHT_POKEMON_QUEUE = list_create();
	GET_POKEMON_QUEUE = list_create();
	LOCALIZED_POKEMON_QUEUE = list_create();
	NEW_POKEMON_QUEUE_SUSCRIPT = list_create();
	APPEARED_POKEMON_QUEUE_SUSCRIPT = list_create();
	CATCH_POKEMON_QUEUE_SUSCRIPT = list_create();
	CAUGHT_POKEMON_QUEUE_SUSCRIPT = list_create();
	GET_POKEMON_QUEUE_SUSCRIPT = list_create();
	LOCALIZED_POKEMON_QUEUE_SUSCRIPT = list_create();
	NEW_POKEMON_COLA = queue_create();
	APPEARED_POKEMON_COLA = queue_create();
	CATCH_POKEMON_COLA = queue_create();
	CAUGHT_POKEMON_COLA = queue_create();
	GET_POKEMON_COLA = queue_create();
	LOCALIZED_POKEMON_COLA = queue_create();
	SUSCRIPCION_COLA = queue_create();
	ACK_COLA = queue_create();
	IDS_RECIBIDOS = list_create();

	pthread_mutex_init(&new_pokemon_mutex,NULL);
	pthread_mutex_init(&appeared_pokemon_mutex,NULL);
	pthread_mutex_init(&catch_pokemon_mutex,NULL);
	pthread_mutex_init(&caught_pokemon_mutex,NULL);
	pthread_mutex_init(&localized_pokemon_mutex,NULL);
	pthread_mutex_init(&get_pokemon_mutex,NULL);
	pthread_mutex_init(&suscripcion_mutex,NULL);
	pthread_mutex_init(&new_pokemon_queue_mutex,NULL);
	pthread_mutex_init(&appeared_pokemon_queue_mutex,NULL);
	pthread_mutex_init(&catch_pokemon_queue_mutex,NULL);
	pthread_mutex_init(&caught_pokemon_queue_mutex,NULL);
	pthread_mutex_init(&localized_pokemon_queue_mutex,NULL);
	pthread_mutex_init(&get_pokemon_queue_mutex,NULL);
	pthread_mutex_init(&suscripcion_new_queue_mutex,NULL);
	pthread_mutex_init(&suscripcion_get_queue_mutex,NULL);
	pthread_mutex_init(&suscripcion_caught_queue_mutex,NULL);
	pthread_mutex_init(&suscripcion_localized_queue_mutex,NULL);
	pthread_mutex_init(&suscripcion_catch_queue_mutex,NULL);
	pthread_mutex_init(&suscripcion_appeared_queue_mutex,NULL);
	pthread_mutex_init(&multhilos_mutex,NULL);
	pthread_mutex_init(&logger_mutex,NULL);
	pthread_mutex_init(&memoria_buddy_mutex,NULL);
	pthread_mutex_init(&id_fifo_mutex,NULL);
	pthread_mutex_init(&ack_queue_mutex,NULL);
	pthread_mutex_init(&ids_recibidos_mtx,NULL);
}

void terminar_queues(void){
	list_destroy(NEW_POKEMON_QUEUE);
	list_destroy(APPEARED_POKEMON_QUEUE);
	list_destroy(CATCH_POKEMON_QUEUE);
	list_destroy(CAUGHT_POKEMON_QUEUE);
	list_destroy(GET_POKEMON_QUEUE);
	list_destroy(NEW_POKEMON_QUEUE_SUSCRIPT);
	list_destroy(APPEARED_POKEMON_QUEUE_SUSCRIPT);
	list_destroy(CATCH_POKEMON_QUEUE_SUSCRIPT);
	list_destroy(CAUGHT_POKEMON_QUEUE_SUSCRIPT);
	list_destroy(GET_POKEMON_QUEUE_SUSCRIPT);
}

void esperar_cliente(int servidor){

	struct sockaddr_in direccion_cliente;

	unsigned int tam_direccion = sizeof(struct sockaddr_in);

	int cliente = accept (servidor, (void*) &direccion_cliente, &tam_direccion);
	pthread_t hilo;

	pthread_mutex_lock(&multhilos_mutex);
	list_add(multihilos, &hilo);
	pthread_mutex_unlock(&multhilos_mutex);

	pthread_create(&hilo,NULL,(void*)serve_client,cliente);
	pthread_detach(hilo);

}

void serve_client(int socket){
	int rec;
	int cod_op;
	while(1){
		rec = recv(socket, &cod_op, sizeof(op_code), MSG_WAITALL);
		if(rec == -1 || rec == 0 ){
			cod_op = -1;
			//			pthread_mutex_lock(&logger_mutex);
			//			log_info(logger,"Se desconecto el proceso con id: %d",socket);
			//			pthread_mutex_unlock(&logger_mutex);
			pthread_exit(NULL);
		}
		puts("recibi un mensaje");
		printf("codigo: %d\n", cod_op);
		process_request(cod_op, socket);
	}
}

void process_request(int cod_op, int cliente_fd) {
	int size = 0;
	void* buffer = recibir_mensaje(cliente_fd, &size);
	pthread_mutex_lock(&logger_mutex);
	log_info(logger,"Se conecto el proceso con id: %d",cliente_fd);
	pthread_mutex_unlock(&logger_mutex);
	if(cod_op == SUSCRIPCION){
		t_mensaje_broker* mensaje_suscripcion = malloc(sizeof(t_mensaje_broker));
		mensaje_suscripcion->buffer = buffer;
		mensaje_suscripcion->suscriptor = cliente_fd;
		pthread_mutex_lock(&suscripcion_mutex);
		queue_push(SUSCRIPCION_COLA, mensaje_suscripcion);
		pthread_mutex_unlock(&suscripcion_mutex);
		sem_post(&suscripcion_sem);
	}else if(cod_op == ACK){
		t_ack* ack = deserializar_ack(buffer);
		free(buffer);
		printf("id %d, proceso %d\n", ack->id_mensaje, ack->id_proceso);
		pthread_mutex_lock(&logger_mutex);
		log_info(logger, "Recepcion de mensaje %d por el proceso %d ", ack->id_mensaje, cliente_fd);
		pthread_mutex_unlock(&logger_mutex);
		pthread_mutex_lock(&ack_queue_mutex);
		queue_push(ACK_COLA, ack);
		pthread_mutex_unlock(&ack_queue_mutex);
		sem_post(&ack_sem);
	}else{
		suscribir_mensaje(cod_op,buffer,cliente_fd,size);
	}


}


char* stringCola(op_code estado){
	switch (estado){
	case NEW_POKEMON:
		return "NEW_POKEMON";
		break;
	case APPEARED_POKEMON:
		return "APPEARED_POKEMON";
		break;
	case CATCH_POKEMON:
		return "CATCH_POKEMON";
		break;
	case CAUGHT_POKEMON:
		return "CAUGHT_POKEMON";
		break;
	case GET_POKEMON:
		return "GET_POKEMON";
		break;
	case LOCALIZED_POKEMON:
		return "LOCALIZED_POKEMON";
		break;
	}
}

int suscribir_mensaje(int cod_op,void* buffer,int cliente_fd,uint32_t size){

	t_buffer_broker* buffer_broker;
	pthread_mutex_lock(&unique_id_mutex);
	unique_message_id++;
	uint32_t mensaje_id = unique_message_id;
	pthread_mutex_unlock(&unique_id_mutex);

	if(cod_op != GET_POKEMON){
		send(cliente_fd,&mensaje_id,sizeof(uint32_t),0); //envio ack
	}else{
		enviar_ack(cliente_fd, mensaje_id, 0);
	}

	if(es_mensaje_respuesta(cod_op)){
		buffer_broker = deserializar_broker_vuelta(buffer,size);
	}else{
		buffer_broker = deserializar_broker_ida(buffer,size); //si no es de respuesta no tiene correlation id
	}

	free(buffer);

	pthread_mutex_lock(&logger_mutex);
	log_info(logger, "LLega mensaje %s id: %d correlation id: %d", stringCola(cod_op), buffer_broker->id, buffer_broker->correlation_id);
	pthread_mutex_unlock(&logger_mutex);
	puts("antes de almacenar dato");

	t_bloque_broker* bloque_broker = malloc(sizeof(t_bloque_broker));

	bloque_broker->particion = almacenar_dato(buffer_broker->buffer,buffer_broker->tamanio,cod_op, mensaje_id);
	bloque_broker->procesos_recibidos = list_create();
	bloque_broker->id = mensaje_id;
	bloque_broker->correlation_id = buffer_broker->correlation_id;
	bloque_broker->tamanio_real = buffer_broker->tamanio; //tamaño del mensaje (incluido correlation id y/o id) por si el mensaje se guarda en una particion mas grande que el (frag interna)

	free(buffer_broker->buffer);
	free(buffer_broker);

	pthread_mutex_init(&(bloque_broker->mtx), NULL);

	pthread_mutex_lock(&ids_recibidos_mtx);
	list_add(IDS_RECIBIDOS, bloque_broker);
	pthread_mutex_unlock(&ids_recibidos_mtx);

	printf("id: %d cid: %d\n", bloque_broker->id, bloque_broker->correlation_id);

	switch (cod_op) {
	case NEW_POKEMON:
		pthread_mutex_lock(&new_pokemon_mutex);
		queue_push(NEW_POKEMON_COLA,bloque_broker);
		pthread_mutex_unlock(&new_pokemon_mutex);
		sem_post(&new_pokemon_sem);
		break;
	case APPEARED_POKEMON:
		pthread_mutex_lock(&appeared_pokemon_mutex);
		queue_push(APPEARED_POKEMON_COLA,bloque_broker);
		pthread_mutex_unlock(&appeared_pokemon_mutex);
		sem_post(&appeared_pokemon_sem);
		break;
	case CATCH_POKEMON:
		pthread_mutex_lock(&catch_pokemon_mutex);
		queue_push(CATCH_POKEMON_COLA,bloque_broker);
		pthread_mutex_unlock(&catch_pokemon_mutex);
		sem_post(&catch_pokemon_sem);
		break;
	case CAUGHT_POKEMON:
		pthread_mutex_lock(&caught_pokemon_mutex);
		queue_push(CAUGHT_POKEMON_COLA,bloque_broker);
		pthread_mutex_unlock(&caught_pokemon_mutex);
		sem_post(&caught_pokemon_sem);
		break;
	case GET_POKEMON:
		pthread_mutex_lock(&get_pokemon_mutex);
		queue_push(GET_POKEMON_COLA,bloque_broker);
		pthread_mutex_unlock(&get_pokemon_mutex);
		sem_post(&get_pokemon_sem);
		break;
	case LOCALIZED_POKEMON:
		pthread_mutex_lock(&localized_pokemon_mutex);
		queue_push(LOCALIZED_POKEMON_COLA,bloque_broker);
		pthread_mutex_unlock(&localized_pokemon_mutex);
		sem_post(&localized_pokemon_sem);
		break;
	case 0:
		pthread_exit(NULL);
	case -1:
		pthread_exit(NULL);
	}

	return bloque_broker->id;
}

bool es_mensaje_respuesta(op_code cod_op){
	return cod_op == APPEARED_POKEMON || cod_op == LOCALIZED_POKEMON || cod_op == CAUGHT_POKEMON || cod_op == CATCH_POKEMON; //catch en realidad no va pero esta en position and name D:
}

void ejecutar_ACK(){
	while(1){
		sem_wait(&ack_sem);
		pthread_mutex_lock(&ack_queue_mutex);
		t_ack* ack = queue_pop(ACK_COLA);
		pthread_mutex_unlock(&ack_queue_mutex);
		printf("el id que llego %d", ack->id_mensaje);

		bool _buscar_por_id(t_bloque_broker* bloque){
			return bloque->id == ack->id_mensaje;
		}

		pthread_mutex_lock(&ids_recibidos_mtx);
		t_bloque_broker* bloque_broker = list_find(IDS_RECIBIDOS, (void*)_buscar_por_id);

		pthread_mutex_lock(&(bloque_broker->mtx));
		list_add(bloque_broker->procesos_recibidos, ack->id_proceso);
		pthread_mutex_unlock(&(bloque_broker->mtx));

		pthread_mutex_unlock(&ids_recibidos_mtx);

		free(ack);
	}
}

bool buscar_por_id(t_bloque_broker* bloque, int id_mensaje){
	return bloque->id == id_mensaje;

}

void enviar_mensaje_broker(int cliente_a_enviar,void* a_enviar,int bytes, op_code cola, int index){
	printf("cliente al que se le envia es %d", cliente_a_enviar);

	int buffer_aux;

	int se_cayo = recv(cliente_a_enviar, &buffer_aux, sizeof(uint32_t), MSG_PEEK | MSG_DONTWAIT);

	if(se_cayo != 0){
		send(cliente_a_enviar,a_enviar,bytes,0);
	}else{
		desuscribir_cliente(cliente_a_enviar, cola, index);
	}
}

void desuscribir_cliente(int cliente, op_code cola, int index){
	switch(cola){
	case NEW_POKEMON:
		list_remove(NEW_POKEMON_QUEUE_SUSCRIPT, index);
		break;
	case GET_POKEMON:
		list_remove(GET_POKEMON_QUEUE_SUSCRIPT, index);
		break;
	case CAUGHT_POKEMON:
		list_remove(CAUGHT_POKEMON_QUEUE_SUSCRIPT, index);
		break;
	case LOCALIZED_POKEMON:
		list_remove(LOCALIZED_POKEMON_QUEUE_SUSCRIPT, index);
		break;
	case APPEARED_POKEMON:
		list_remove(APPEARED_POKEMON_QUEUE_SUSCRIPT, index);
		break;
	case CATCH_POKEMON:
		list_remove(CATCH_POKEMON_QUEUE_SUSCRIPT, index);
		break;
	}
}


t_paquete* preparar_mensaje_a_enviar(t_bloque_broker* bloque_broker, op_code codigo_operacion){

	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete -> codigo_operacion = codigo_operacion;

	t_buffer* buffer_cargado = malloc(sizeof(t_buffer));

	int size = bloque_broker->tamanio_real + sizeof(uint32_t);

	bloque_broker->particion->ultimo_acceso = timestamp(&(bloque_broker->particion->fecha));

	if(es_mensaje_respuesta(codigo_operacion)){
		size+= sizeof(uint32_t);
	}

	buffer_cargado->size = size;
	void* stream = malloc(buffer_cargado->size);

	int offset = 0;

	pthread_mutex_lock(&memoria_cache_mtx);
	memcpy(stream + offset, &bloque_broker->id, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	if(es_mensaje_respuesta(codigo_operacion)){
		memcpy(stream + offset, &bloque_broker->correlation_id, sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}
	memcpy(stream + offset, (void*)bloque_broker->particion->base, bloque_broker->tamanio_real);
	bloque_broker->particion->ultimo_acceso = timestamp(&(bloque_broker->particion->fecha));
	pthread_mutex_unlock(&memoria_cache_mtx);

	buffer_cargado->stream = stream;

	paquete -> buffer = buffer_cargado;

	return paquete;
}

void ejecutar_new_pokemon(){

	while(1){

		sem_wait(&new_pokemon_sem);
		pthread_mutex_lock(&new_pokemon_mutex);
		t_bloque_broker* bloque_broker = queue_pop(NEW_POKEMON_COLA);
		pthread_mutex_unlock(&new_pokemon_mutex);

		op_code codigo_operacion = NEW_POKEMON;

		int bytes = 0;
		pthread_mutex_lock(&lista_particiones_mtx);

		t_paquete* paquete = preparar_mensaje_a_enviar(bloque_broker, codigo_operacion);

		pthread_mutex_unlock(&lista_particiones_mtx);

		void* a_enviar = serializar_paquete(paquete, &bytes);

		int index = 0;
		puts("esta por enviar un mensaje");
		void _enviar_mensaje_broker(int cliente_a_enviar){
			enviar_mensaje_broker(cliente_a_enviar, a_enviar, bytes, NEW_POKEMON, index);

			pthread_mutex_lock(&logger_mutex);
			log_info(logger, "Se envia mensaje %s id: %d a suscriptor %d", stringCola(codigo_operacion), bloque_broker->id, cliente_a_enviar);
			pthread_mutex_unlock(&logger_mutex);

			index++;
		}

		pthread_mutex_lock(&suscripcion_new_queue_mutex);
		list_iterate(NEW_POKEMON_QUEUE_SUSCRIPT, (void*)_enviar_mensaje_broker);
		pthread_mutex_unlock(&suscripcion_new_queue_mutex);

		free(a_enviar);
		free(paquete -> buffer->stream);
		free(paquete->buffer);
		free(paquete);

	}
}

void ejecutar_appeared_pokemon(){

	while(1){

		sem_wait(&appeared_pokemon_sem);
		pthread_mutex_lock(&appeared_pokemon_mutex);
		t_bloque_broker* bloque_broker = queue_pop(APPEARED_POKEMON_COLA);
		pthread_mutex_unlock(&appeared_pokemon_mutex);

		op_code codigo_operacion = APPEARED_POKEMON;

		int bytes = 0;
		pthread_mutex_lock(&lista_particiones_mtx);

		t_paquete* paquete = preparar_mensaje_a_enviar(bloque_broker, codigo_operacion);

		pthread_mutex_unlock(&lista_particiones_mtx);

		void* a_enviar = serializar_paquete(paquete, &bytes);

		int index = 0;
		puts("esta por enviar un mensaje");
		void _enviar_mensaje_broker(int cliente_a_enviar){
			enviar_mensaje_broker(cliente_a_enviar, a_enviar, bytes, APPEARED_POKEMON, index);

			pthread_mutex_lock(&logger_mutex);
			log_info(logger, "Se envia mensaje %s id: %d a suscriptor %d", stringCola(codigo_operacion), bloque_broker->id, cliente_a_enviar);
			pthread_mutex_unlock(&logger_mutex);

			index++;
		}

		pthread_mutex_lock(&suscripcion_appeared_queue_mutex);
		list_iterate(APPEARED_POKEMON_QUEUE_SUSCRIPT, (void*)_enviar_mensaje_broker);
		pthread_mutex_unlock(&suscripcion_appeared_queue_mutex);
		puts("envio appeared_pokemon");

		free(a_enviar);
		free(paquete -> buffer->stream);
		free(paquete->buffer);
		free(paquete);
	}
}

void ejecutar_catch_pokemon(){

	while(1){

		sem_wait(&catch_pokemon_sem);
		pthread_mutex_lock(&catch_pokemon_mutex);
		t_bloque_broker* bloque_broker = queue_pop(CATCH_POKEMON_COLA);
		pthread_mutex_unlock(&catch_pokemon_mutex);

		op_code codigo_operacion = CATCH_POKEMON;

		int bytes = 0;
		pthread_mutex_lock(&lista_particiones_mtx);

		t_paquete* paquete = preparar_mensaje_a_enviar(bloque_broker, codigo_operacion);

		pthread_mutex_unlock(&lista_particiones_mtx);

		void* a_enviar = serializar_paquete(paquete, &bytes);

		int index = 0;
		puts("esta por enviar un mensaje");
		void _enviar_mensaje_broker(int cliente_a_enviar){
			enviar_mensaje_broker(cliente_a_enviar, a_enviar, bytes, CATCH_POKEMON, index);

			pthread_mutex_lock(&logger_mutex);
			log_info(logger, "Se envia mensaje %s id: %d a suscriptor %d", stringCola(codigo_operacion), bloque_broker->id, cliente_a_enviar);
			pthread_mutex_unlock(&logger_mutex);

			index++;
		}

		pthread_mutex_lock(&suscripcion_catch_queue_mutex);
		list_iterate(CATCH_POKEMON_QUEUE_SUSCRIPT, (void*)_enviar_mensaje_broker);
		pthread_mutex_unlock(&suscripcion_catch_queue_mutex);

		free(a_enviar);
		free(paquete -> buffer->stream);
		free(paquete->buffer);
		free(paquete);
	}
}

void ejecutar_caught_pokemon(){

	while(1){

		sem_wait(&caught_pokemon_sem);
		pthread_mutex_lock(&caught_pokemon_mutex);
		t_bloque_broker* bloque_broker = queue_pop(CAUGHT_POKEMON_COLA);
		pthread_mutex_unlock(&caught_pokemon_mutex);

		op_code codigo_operacion = CAUGHT_POKEMON;

		int bytes = 0;
		pthread_mutex_lock(&lista_particiones_mtx);

		t_paquete* paquete = preparar_mensaje_a_enviar(bloque_broker, codigo_operacion);

		pthread_mutex_unlock(&lista_particiones_mtx);

		void* a_enviar = serializar_paquete(paquete, &bytes);

		int index = 0;
		puts("esta por enviar un mensaje");
		void _enviar_mensaje_broker(int cliente_a_enviar){
			enviar_mensaje_broker(cliente_a_enviar, a_enviar, bytes, CAUGHT_POKEMON, index);

			pthread_mutex_lock(&logger_mutex);
			log_info(logger, "Se envia mensaje %s id: %d a suscriptor %d", stringCola(codigo_operacion), bloque_broker->id, cliente_a_enviar);
			pthread_mutex_unlock(&logger_mutex);

			index++;
		}


		pthread_mutex_lock(&suscripcion_caught_queue_mutex);
		list_iterate(CAUGHT_POKEMON_QUEUE_SUSCRIPT, (void*)_enviar_mensaje_broker);
		pthread_mutex_unlock(&suscripcion_caught_queue_mutex);

		free(a_enviar);
		free(paquete -> buffer->stream);
		free(paquete->buffer);
		free(paquete);

	}
}

void ejecutar_get_pokemon(){

	while(1){

		sem_wait(&get_pokemon_sem);
		puts("entra a get pokemon");
		pthread_mutex_lock(&get_pokemon_mutex);
		t_bloque_broker* bloque_broker = queue_pop(GET_POKEMON_COLA);
		pthread_mutex_unlock(&get_pokemon_mutex);

		op_code codigo_operacion = GET_POKEMON;

		int bytes = 0;
		pthread_mutex_lock(&lista_particiones_mtx);

		t_paquete* paquete = preparar_mensaje_a_enviar(bloque_broker, codigo_operacion);

		pthread_mutex_unlock(&lista_particiones_mtx);

		void* a_enviar = serializar_paquete(paquete, &bytes);

		int index = 0;
		puts("esta por enviar un mensaje");
		void _enviar_mensaje_broker(int cliente_a_enviar){
			enviar_mensaje_broker(cliente_a_enviar, a_enviar, bytes, GET_POKEMON, index);

			pthread_mutex_lock(&logger_mutex);
			log_info(logger, "Se envia mensaje %s id: %d a suscriptor %d", stringCola(codigo_operacion), bloque_broker->id, cliente_a_enviar);
			pthread_mutex_unlock(&logger_mutex);

			index++;
		}

		pthread_mutex_lock(&suscripcion_get_queue_mutex);
		list_iterate(GET_POKEMON_QUEUE_SUSCRIPT, (void*)_enviar_mensaje_broker);
		pthread_mutex_unlock(&suscripcion_get_queue_mutex);

		free(a_enviar);
		free(paquete -> buffer->stream);
		free(paquete->buffer);
		free(paquete);

	}
}

void ejecutar_localized_pokemon(){

	while(1){

		sem_wait(&localized_pokemon_sem);
		pthread_mutex_lock(&localized_pokemon_mutex);
		t_bloque_broker* bloque_broker = queue_pop(LOCALIZED_POKEMON_COLA);
		pthread_mutex_unlock(&localized_pokemon_mutex);

		op_code codigo_operacion = LOCALIZED_POKEMON;

		int bytes = 0;
		pthread_mutex_lock(&lista_particiones_mtx);

		t_paquete* paquete = preparar_mensaje_a_enviar(bloque_broker, codigo_operacion);

		pthread_mutex_unlock(&lista_particiones_mtx);

		void* a_enviar = serializar_paquete(paquete, &bytes);

		int index = 0;
		puts("esta por enviar un mensaje");
		void _enviar_mensaje_broker(int cliente_a_enviar){
			enviar_mensaje_broker(cliente_a_enviar, a_enviar, bytes, LOCALIZED_POKEMON, index);

			pthread_mutex_lock(&logger_mutex);
			log_info(logger, "Se envia mensaje %s id: %d a suscriptor %d", stringCola(codigo_operacion), bloque_broker->id, cliente_a_enviar);
			pthread_mutex_unlock(&logger_mutex);

			index++;
		}

		pthread_mutex_lock(&suscripcion_localized_queue_mutex);
		list_iterate(LOCALIZED_POKEMON_QUEUE_SUSCRIPT, (void*)_enviar_mensaje_broker);
		pthread_mutex_unlock(&suscripcion_localized_queue_mutex);

		free(a_enviar);
		free(paquete -> buffer->stream);
		free(paquete->buffer);
		free(paquete);

	}
}

void ejecutar_suscripcion(){

	while(1){

		sem_wait(&suscripcion_sem);
		pthread_mutex_lock(&suscripcion_mutex);
		t_mensaje_broker* mensaje = queue_pop(SUSCRIPCION_COLA);
		pthread_mutex_unlock(&suscripcion_mutex);

		void* buffer = mensaje->buffer;

		t_suscripcion* mensaje_suscripcion = deserializar_suscripcion(buffer);

		int suscriptor = mensaje->suscriptor;
//		puts(string_itoa(suscriptor));

		switch (mensaje_suscripcion->cola) {
		case NEW_POKEMON:
			pthread_mutex_lock(&suscripcion_new_queue_mutex);
			list_add(NEW_POKEMON_QUEUE_SUSCRIPT,suscriptor);
			pthread_mutex_unlock(&suscripcion_new_queue_mutex);
			pthread_mutex_lock(&logger_mutex);
			log_info(logger,"Se suscribio el proceso, %d ,a la cola NEW_POKEMON",suscriptor);
			pthread_mutex_unlock(&logger_mutex);
			break;
		case APPEARED_POKEMON:
			pthread_mutex_lock(&suscripcion_appeared_queue_mutex);
			list_add(APPEARED_POKEMON_QUEUE_SUSCRIPT,suscriptor);
			pthread_mutex_unlock(&suscripcion_appeared_queue_mutex);
			pthread_mutex_lock(&logger_mutex);
			log_info(logger,"Se suscribio el proceso, %d ,a la cola APPEAREAD_POKEMON",suscriptor);
			pthread_mutex_unlock(&logger_mutex);
			break;
		case CATCH_POKEMON:
			pthread_mutex_lock(&suscripcion_catch_queue_mutex);
			list_add(CATCH_POKEMON_QUEUE_SUSCRIPT,suscriptor);
			pthread_mutex_unlock(&suscripcion_catch_queue_mutex);
			pthread_mutex_lock(&logger_mutex);
			log_info(logger,"Se suscribio el proceso, %d ,a la cola CATCH_POKEMON",suscriptor);
			pthread_mutex_unlock(&logger_mutex);
			break;
		case CAUGHT_POKEMON:
			pthread_mutex_lock(&suscripcion_caught_queue_mutex);
			list_add(CAUGHT_POKEMON_QUEUE_SUSCRIPT,suscriptor);
			pthread_mutex_unlock(&suscripcion_caught_queue_mutex);
			pthread_mutex_lock(&logger_mutex);
			log_info(logger,"Se suscribio el proceso, %d ,a la cola CAUGHT_POKEMON",suscriptor);
			pthread_mutex_unlock(&logger_mutex);
			break;
		case GET_POKEMON:
			pthread_mutex_lock(&suscripcion_get_queue_mutex);
			list_add(GET_POKEMON_QUEUE_SUSCRIPT,suscriptor);
			pthread_mutex_unlock(&suscripcion_get_queue_mutex);
			pthread_mutex_lock(&logger_mutex);
			log_info(logger,"Se suscribio el proceso, %d ,a la cola GET_POKEMON",suscriptor);
			pthread_mutex_unlock(&logger_mutex);
			break;
		case LOCALIZED_POKEMON:
			pthread_mutex_lock(&suscripcion_localized_queue_mutex);
			list_add(LOCALIZED_POKEMON_QUEUE_SUSCRIPT,suscriptor);//Ver si va el & o no
			pthread_mutex_unlock(&suscripcion_localized_queue_mutex);
			pthread_mutex_lock(&logger_mutex);
			log_info(logger,"Se suscribio el proceso, %d ,a la cola LOCALIZED_POKEMON",suscriptor);
			pthread_mutex_unlock(&logger_mutex);
			break;
		}


//		free(mensaje->buffer);
		enviar_faltantes(suscriptor, mensaje_suscripcion);
		free(buffer);
		free(mensaje);
		free(mensaje_suscripcion);
	}
}

void enviar_faltantes(int suscriptor, t_suscripcion* mensaje_suscripcion){

	bool _buscar_por_proceso(int proceso){
		return proceso == mensaje_suscripcion->id_proceso;
	}

	bool _falta_enviar(t_bloque_broker* bloque){
		pthread_mutex_lock(&(bloque->mtx));
		bool entregado = list_any_satisfy(bloque->procesos_recibidos, (void*) _buscar_por_proceso); //esto da true si el mensaje ya fue enviado al proceso
		bool es_de_cola = bloque->particion->cola == mensaje_suscripcion->cola;
		pthread_mutex_unlock(&(bloque->mtx));
		return !entregado && es_de_cola;
	}

	pthread_mutex_lock(&lista_particiones_mtx);

	pthread_mutex_lock(&ids_recibidos_mtx);

	t_list* mensajes_de_cola = list_filter(IDS_RECIBIDOS, (void*)_falta_enviar); //tengo los mensajes que no le mande a ese proceso

	if(!list_is_empty(mensajes_de_cola)){
		puts("-----------------FALTA ENVIAR---------------");
		int index;
		void _enviar_mensaje_faltante(t_bloque_broker* bloque){

			t_paquete* paquete = preparar_mensaje_a_enviar(bloque, mensaje_suscripcion->cola);
			int bytes = 0;
			void* a_enviar = serializar_paquete(paquete, &bytes);

			pthread_mutex_t* cola_mtx = semaforo_de_cola(mensaje_suscripcion->cola);
			pthread_mutex_lock(cola_mtx);
			enviar_mensaje_broker(suscriptor, a_enviar, bytes, bloque->particion->cola, index);

			pthread_mutex_lock(&logger_mutex);
			log_info(logger, "Se envia mensaje %s id: %d a suscriptor %d", stringCola(bloque->particion->cola), bloque->id, suscriptor);
			pthread_mutex_unlock(&logger_mutex);
			pthread_mutex_unlock(cola_mtx);
			index++;
			free(a_enviar);
			free(paquete -> buffer->stream);
			free(paquete->buffer);
			free(paquete);
		}

		list_iterate(mensajes_de_cola, (void*)_enviar_mensaje_faltante); //TODO esto se me hace re falopa, hay que ver que este bien
	}

	list_destroy(mensajes_de_cola);

	pthread_mutex_unlock(&ids_recibidos_mtx);
	pthread_mutex_unlock(&lista_particiones_mtx);
}

pthread_mutex_t* semaforo_de_cola(op_code cola){
	switch(cola){
	case NEW_POKEMON:
		return &suscripcion_new_queue_mutex;
	case GET_POKEMON:
		return &suscripcion_get_queue_mutex;
	case CAUGHT_POKEMON:
		return &suscripcion_caught_queue_mutex;
	case LOCALIZED_POKEMON:
		return &suscripcion_localized_queue_mutex;
	case APPEARED_POKEMON:
		return &suscripcion_appeared_queue_mutex;
	case CATCH_POKEMON:
		return &suscripcion_catch_queue_mutex;
	}
	return NULL;
}

//------------MEMORIA------------//
void iniciar_memoria(){

	configuracion_cache = malloc(sizeof(t_config_cache));

	configuracion_cache->tamanio_memoria = config_get_int_value(config, TAMANO_MEMORIA);
	configuracion_cache->tamanio_minimo_p = config_get_int_value(config, TAMANO_MINIMO_PARTICION);

	if(string_equals_ignore_case(config_get_string_value(config, ALGORITMO_MEMORIA), "BS")) configuracion_cache->algoritmo_memoria = BS;
	else if(string_equals_ignore_case(config_get_string_value(config, ALGORITMO_MEMORIA), "PARTICIONES")) configuracion_cache->algoritmo_memoria = PARTICIONES;

	if(string_equals_ignore_case(config_get_string_value(config, ALGORITMO_REEMPLAZO), "FIFO")) configuracion_cache->algoritmo_reemplazo = FIFO;
	else if(string_equals_ignore_case(config_get_string_value(config, ALGORITMO_REEMPLAZO), "LRU")) configuracion_cache->algoritmo_reemplazo = LRU;

	if(string_equals_ignore_case(config_get_string_value(config, ALGORITMO_PARTICION_LIBRE), "FF")) configuracion_cache->algoritmo_part_libre = FIRST_FIT;
	else if(string_equals_ignore_case(config_get_string_value(config, ALGORITMO_PARTICION_LIBRE), "BF")) configuracion_cache->algoritmo_part_libre = BEST_FIT;

	configuracion_cache->frecuencia_compact = config_get_int_value(config, FRECUENCIA_COMPACTACION);

	memoria_cache = malloc(configuracion_cache->tamanio_memoria);

	t_particion* particion_inicial = malloc(sizeof(t_particion));

	particion_inicial->base = (int)memoria_cache;
	particion_inicial->tamanio = configuracion_cache->tamanio_memoria;
	particion_inicial->id_mensaje = 0;
	particion_inicial->cola = 0;
	particion_inicial->ocupado = false;
	particion_inicial->ultimo_acceso = timestamp(&(particion_inicial->fecha));

	pthread_mutex_lock(&id_fifo_mutex);
	id_fifo = 0;
	particion_inicial->id = id_fifo;
	pthread_mutex_unlock(&id_fifo_mutex);

	pthread_mutex_lock(&lista_particiones_mtx);
	particiones = list_create();
	list_add(particiones, particion_inicial);
	pthread_mutex_unlock(&lista_particiones_mtx);

	pthread_mutex_lock(&memoria_buddy_mutex);
	memoria_buddy = list_create();
	list_add(memoria_buddy,particion_inicial);
	pthread_mutex_unlock(&memoria_buddy_mutex);
}

void* almacenar_dato(void* datos, int tamanio, op_code codigo_op, uint32_t id){

	void* lugar_donde_esta;

	switch(configuracion_cache->algoritmo_memoria){
	case BS:
		lugar_donde_esta = almacenar_datos_buddy(datos, tamanio,codigo_op,id);
		break;
	case PARTICIONES:
		lugar_donde_esta = almacenar_dato_particiones(datos, tamanio, codigo_op, id);
		break;
	}

	return lugar_donde_esta;
}

t_particion* almacenar_dato_particiones(void* datos, int tamanio, op_code codigo_op, uint32_t id){

	t_particion* particion_libre;

	pthread_mutex_lock(&lista_particiones_mtx);
	switch(configuracion_cache->algoritmo_part_libre){
	case FIRST_FIT:
		particion_libre = particion_libre_ff(tamanio);
		break;
	case BEST_FIT:
		particion_libre = particion_libre_bf(tamanio);
	}

	asignar_particion(datos, particion_libre, tamanio, codigo_op, id);
	pthread_mutex_unlock(&lista_particiones_mtx);
	pthread_mutex_lock(&logger_mutex);
	log_info(logger,"Se ha guardado el mensaje %d en la posicion %p", id, (void*)particion_libre->base);
	pthread_mutex_unlock(&logger_mutex);

	return particion_libre;
}

void ordenar_particiones(t_list* lista){ //no se si anda esto

	bool _orden(t_particion* particion1, t_particion* particion2){
		return particion1->base < particion2->base;
	}

	list_sort(lista, (void*)_orden);
}

void compactar(){
	int offset = 0;
	t_particion* aux;

	bool _esta_ocupada(t_particion* part){
		return part->ocupado;
	}



//	pthread_mutex_lock(&lista_particiones_mtx);

	ordenar_particiones(particiones); //ordeno entonces puedo ir moviendo una por una al principio de la memoria

	t_list* particiones_ocupadas = list_filter(particiones, (void*)_esta_ocupada);

	int cantidad_p_ocupadas = list_size(particiones_ocupadas);

	pthread_mutex_lock(&memoria_cache_mtx);
	for(int i = 0; i < cantidad_p_ocupadas; i++){
		aux = list_get(particiones_ocupadas, i);
		memcpy(memoria_cache + offset, (void*)aux->base, aux->tamanio);
		aux->base = (int)memoria_cache + offset;
		offset+= aux->tamanio;
	}

	t_list* particiones_aux = list_filter(particiones, (void*)esta_ocupada);

	list_clean(particiones);

	particiones = particiones_aux;

	if(configuracion_cache->tamanio_memoria - offset != 0){

		t_particion* particion_unica = malloc(sizeof(t_particion));
		particion_unica->base = (int) memoria_cache + offset;
		particion_unica->tamanio = configuracion_cache->tamanio_memoria - offset; //esto esta bien?
		particion_unica->id_mensaje = 0;
		particion_unica->ultimo_acceso = timestamp(&(particion_unica->fecha));
		particion_unica->ocupado = false;
		list_add(particiones, particion_unica);
	}
	pthread_mutex_unlock(&memoria_cache_mtx);
	list_destroy(particiones_ocupadas); //esto no deberia borrar los elementos, si los borra entonces sacar(?
//	pthread_mutex_unlock(&lista_particiones_mtx); //otro mutex grande :(

	pthread_mutex_lock(&logger_mutex);
	log_info(logger,"Se ejecuto la compactacion");
	pthread_mutex_unlock(&logger_mutex);
}

t_particion* buscar_particion_ff(int tamanio_a_almacenar){ //falta ordenar lista

	t_particion* particion_libre;

	bool _puede_almacenar_y_esta_libre(t_particion* particion){
		return particion->tamanio>= tamanio_a_almacenar && !particion->ocupado && particion->tamanio >= configuracion_cache->tamanio_minimo_p;
	}

//	pthread_mutex_lock(&lista_particiones_mtx);
	ordenar_particiones(particiones);

	particion_libre =  list_find(particiones, (void*) _puede_almacenar_y_esta_libre); //list find agarra el primero que cumpla, asi que el primero que tenga tamanio mayor o igual será

	if(particion_libre != NULL){
		particion_libre->ocupado = true; //si devuelve algo ya lo pongo como ocupado asi ningun otro hilo puede agarrar la misma particion
		pthread_mutex_lock(&id_fifo_mutex);
		id_fifo++;
		particion_libre->id = id_fifo;
		pthread_mutex_unlock(&id_fifo_mutex);
	}else{
		puts("--------------NO ENCUENTRA");
	}

//	pthread_mutex_unlock(&lista_particiones_mtx);
	return particion_libre;
}

t_particion* particion_libre_ff(int tamanio_a_almacenar){

//	pthread_mutex_lock(&lista_particiones_mtx);
	t_particion* particion_libre = buscar_particion_ff(tamanio_a_almacenar);

	int contador = configuracion_cache->frecuencia_compact;

	while(particion_libre == NULL){
		if(contador < configuracion_cache->frecuencia_compact || configuracion_cache->frecuencia_compact == -1){
			consolidar(elegir_victima_particiones(tamanio_a_almacenar)); //aca se elimina la particion (se pone como libre), se consolida y se vuelve a buscar una particion
			particion_libre = buscar_particion_ff(tamanio_a_almacenar);
			contador++;
		}else{
			compactar();
			particion_libre = buscar_particion_ff(tamanio_a_almacenar);
			contador = 0;
		}
	}
//	pthread_mutex_unlock(&lista_particiones_mtx);

	return particion_libre;
}

t_particion* particion_libre_bf(int tamanio_a_almacenar){

//	pthread_mutex_lock(&lista_particiones_mtx);
	t_particion* particion_libre = buscar_particion_bf(tamanio_a_almacenar);

	int contador = configuracion_cache->frecuencia_compact;

	while(particion_libre == NULL){
		if(contador < configuracion_cache->frecuencia_compact || configuracion_cache->frecuencia_compact == -1){
			consolidar(elegir_victima_particiones()); //aca se elimina la particion (se pone como libre), se consolida y se vuelve a buscar una particion
			particion_libre = buscar_particion_bf(tamanio_a_almacenar);
			contador++;
		}else{
			compactar();
			particion_libre = buscar_particion_bf(tamanio_a_almacenar);
			contador = 0;
		}
	}
//	pthread_mutex_unlock(&lista_particiones_mtx);
	return particion_libre;
}

void consolidar(t_particion* particion_liberada){

	//cuando busco la anterior y la siguiente tambien me fijo que esten libres, si no la encuentra (porque no tiene siguiente/anterior o porque esta ocupada) no consolida

	bool _es_la_anterior(t_particion* particion){
		return particion->base + particion->tamanio == particion_liberada->base && !particion->ocupado;
	}

	bool _es_la_siguiente(t_particion* particion){
		return particion_liberada->base + particion_liberada->tamanio == particion->base && !particion->ocupado;
	}

	bool _es_la_particion(t_particion* particion){
		return particion_liberada->base == particion->base;
	}

//	pthread_mutex_lock(&lista_particiones_mtx); //bloqueo aca porque puede pasar que otro hilo quiera ocupar una de las libres antes/mientras esta consolidando

	t_particion* p_anterior = list_find(particiones,(void*) _es_la_anterior);
	t_particion* p_siguiente = list_find(particiones,(void*) _es_la_siguiente);


	if(particion_liberada != NULL){
		if(p_anterior != NULL && p_siguiente != NULL){ //si encuentra particiones libres por los dos lados consolida las 3
			p_anterior->tamanio += particion_liberada->tamanio + p_siguiente->tamanio; //directamente hago la anterior mas grande (?
			list_remove_by_condition(particiones, (void*) _es_la_siguiente);
			list_remove_by_condition(particiones, (void*) _es_la_particion);
		}else if(p_anterior != NULL && p_siguiente == NULL){ //solo encontro de un lado una particion vacia, consolida solo 2
			p_anterior->tamanio += particion_liberada->tamanio;
			list_remove_by_condition(particiones, (void*) _es_la_particion);
		}else if(p_anterior == NULL && p_siguiente != NULL){
			particion_liberada->tamanio += p_siguiente->tamanio;
			list_remove_by_condition(particiones, (void*) _es_la_siguiente);
		}
		puts("------------------");
		puts("CONSOLIDO");
		puts("------------------");
	}

//	pthread_mutex_unlock(&lista_particiones_mtx); // re largo el mutex, preguntar que onda aca (?

}

t_particion* buscar_particion_bf(int tamanio_a_almacenar){ //se puede con fold creo

	t_particion* best;

	bool _ordenar_por_tamanio(t_particion* particion_menor, t_particion* particion_mayor){
		return particion_menor->tamanio < particion_mayor->tamanio;
	}

	bool _la_mejor(t_particion* particion){
		return particion->tamanio >= tamanio_a_almacenar && !particion->ocupado; //commo esta ordenada de menor a mayor, la primera que encuentre que tenga tamanio
	}																			 //mayor o igual (y este vacia) será la mejor

//	pthread_mutex_lock(&lista_particiones_mtx);

	list_sort(particiones, (void*)_ordenar_por_tamanio); //ordeno de menor a mayor

	best = list_find(particiones, (void*)_la_mejor);

	if(best != NULL){
		best->ocupado = true; //si devuelve algo ya lo pongo como ocupado asi ningun otro hilo puede agarrar la misma particion
		pthread_mutex_lock(&id_fifo_mutex);
		id_fifo++;
		best->id = id_fifo;
		pthread_mutex_unlock(&id_fifo_mutex);
	}

//	pthread_mutex_unlock(&lista_particiones_mtx);
	return best;

}

void dump_cache (int n){		//para usarla en cosola killall -s USR1 Broker
	if(n == SIGUSR1){
		pthread_mutex_lock(&logger_mutex);
		log_info(logger, "Se realizara el Dump de la cache");
		pthread_mutex_unlock(&logger_mutex);
		switch(configuracion_cache->algoritmo_memoria){
		case BS:
			ver_estado_cache_buddy();
			break;
		case PARTICIONES:
			ver_estado_cache_particiones();
			break;
		}
	}
}

void ver_estado_cache_buddy(){

	bool _orden(t_particion* particion1, t_particion* particion2){
		return particion1->base < particion2->base;
	}
	t_list* dump_buddy = list_create();
	pthread_mutex_lock(&memoria_buddy_mutex);
	list_add_all(dump_buddy,memoria_buddy);
	pthread_mutex_unlock(&memoria_buddy_mutex);
	list_sort(dump_buddy, (void*)_orden);

	FILE* dump_cache = fopen("/home/utnso/workspace/tp-2020-1c-MCLDG/Broker/Dump_cache.txt", "a");

	fseek(dump_cache, 0, SEEK_END); //me paro al final

	time_t fecha = time(NULL);

	struct tm *tlocal = localtime(&fecha);
	char output[128];

	strftime(output, 128, "%d/%m/%Y %H:%M:%S", tlocal);

	fprintf(dump_cache, "Dump:%s\n\n", output);

	int i = 1;

	void _imprimir_datos(t_particion* particion){
		char* cola = cola_segun_cod(particion->cola);
		fprintf(dump_cache, "Partición %d: %p - %p [%d]   Size: %db     LRU: %s     COLA: %s     ID: %d\n",
				i, (void*)particion->base, (void*)(particion->base + particion->tamanio - 1), particion->ocupado, particion->tamanio,
				transformar_a_fecha(particion->fecha), cola, particion->id_mensaje);
		i++;
	}

	list_iterate(dump_buddy, (void*)_imprimir_datos);

	fprintf(dump_cache, "------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n\n");

	fclose(dump_cache);

}

char* cola_segun_cod(op_code cod_op){
	char* cola = string_new();
	switch(cod_op){
	case NEW_POKEMON:
		string_append(&cola, "NEW_POKEMON");
		break;
	case GET_POKEMON:
		string_append(&cola, "GET_POKEMON");
		break;
	case LOCALIZED_POKEMON:
		string_append(&cola, "LOCALIZED_POKEMON");
		break;
	case CATCH_POKEMON:
		string_append(&cola, "CATCH_POKEMON");
		break;
	case CAUGHT_POKEMON:
		string_append(&cola, "CAUGHT_POKEMON");
		break;
	case APPEARED_POKEMON:
		string_append(&cola, "APPEARED_POKEMON");
		break;
	}

	return cola;
}
void ver_estado_cache_particiones(){

	bool _orden(t_particion* particion1, t_particion* particion2){
		return particion1->base < particion2->base;
	}
	t_list* dump_particiones = list_create();
//	pthread_mutex_lock(&lista_particiones_mtx);
	list_add_all(dump_particiones,particiones);
//	pthread_mutex_unlock(&lista_particiones_mtx);
	list_sort(dump_particiones, (void*)_orden);

	FILE* dump_cache = fopen("/home/utnso/workspace/tp-2020-1c-MCLDG/Broker/Dump_cache.txt", "a");

	fseek(dump_cache, 0, SEEK_END); //me paro al final

	time_t fecha = time(NULL);

	struct tm *tlocal = localtime(&fecha);
	char output[128];

	strftime(output, 128, "%d/%m/%Y %H:%M:%S", tlocal);

	fprintf(dump_cache, "Dump:%s\n\n", output);

	int i = 1;

	void _imprimir_datos(t_particion* particion){
		char* cola = cola_segun_cod(particion->cola);
		fprintf(dump_cache, "Partición %d: %p - %p [%d]   Size: %db     LRU: %s     COLA: %s     ID: %d\n",
				i, (void*)particion->base, (void*)(particion->base + particion->tamanio - 1), particion->ocupado, particion->tamanio,
				transformar_a_fecha(particion->fecha), cola, particion->id_mensaje);
		i++;
	}

	list_iterate(dump_particiones, (void*)_imprimir_datos);

	fprintf(dump_cache, "------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n\n");

	fclose(dump_cache);

	list_destroy(dump_particiones);
}

char* transformar_a_fecha(struct timeval tv){

	char buffer[26];
	int millisec;
	struct tm* tm_info;

	millisec = lrint(tv.tv_usec/1000.0); // Round to nearest millisec
	if (millisec>=1000) { // Allow for rounding up to nearest second
		millisec -=1000;
		tv.tv_sec++;
	}

	tm_info = localtime(&tv.tv_sec);

	strftime(buffer, 26, "%d/%m/%Y %H:%M:%S", tm_info);

	char* fecha = string_new();

	string_append_with_format(&fecha,"%s.%03d", buffer, millisec);

	return fecha;
}

t_particion* elegir_victima_particiones(int tamanio_a_almacenar){
	switch(configuracion_cache->algoritmo_reemplazo){
	case LRU:
		return elegir_victima_particiones_LRU();
	case FIFO:
		return elegir_victima_particiones_FIFO();
	}
	return NULL;
}

bool esta_ocupada(t_particion* particion){
	return particion->id_mensaje != 0;
}

t_particion* elegir_victima_particiones_FIFO(){
	t_particion* particion;

	bool _orden(t_particion* particion1, t_particion* particion2){
		return particion1->id < particion2->id;
	}

//	pthread_mutex_lock(&lista_particiones_mtx);
	list_sort(particiones, (void*)_orden);

	bool _esta_ocupada(t_particion* part){
		return part->ocupado;
	}

	particion = list_find(particiones, (void*)_esta_ocupada); //las ordeno por posicion y agarro la primera en la lista que este ocupada

	eliminar_mensaje(particion);

//	pthread_mutex_unlock(&lista_particiones_mtx);

	return particion;
}

t_particion* elegir_victima_particiones_LRU(){

	t_particion* particion;

	bool _orden(t_particion* particion1, t_particion* particion2){
		return particion1->ultimo_acceso < particion2->ultimo_acceso;
	}

//	pthread_mutex_lock(&lista_particiones_mtx);
	list_sort(particiones, (void*)_orden);

	particion = list_find(particiones, (void*)esta_ocupada); //las ordeno por LRU y agarro la primera en la lista que este ocupada

	eliminar_mensaje(particion);

//	pthread_mutex_unlock(&lista_particiones_mtx);

	return particion;

}

void eliminar_mensaje(t_particion* particion){

	bool _buscar_id(t_bloque_broker* bloque){
		return bloque->id == particion->id_mensaje;
	}

	particion->cola = 0;
	particion->ocupado = false;
	particion->id_mensaje = 0;
	particion->ultimo_acceso = timestamp(&(particion->fecha)); //importa esto aca??

	pthread_mutex_lock(&ids_recibidos_mtx);
	list_remove_by_condition(IDS_RECIBIDOS, (void*)_buscar_id);
	pthread_mutex_unlock(&ids_recibidos_mtx);

	pthread_mutex_lock(&logger_mutex);
	log_info(logger,"Se ha eliminado el mensaje de la posicion %p", (void*)particion->base);
	pthread_mutex_unlock(&logger_mutex);


}

void asignar_particion(void* datos, t_particion* particion_libre, int tamanio, op_code codigo_op, uint32_t id){

	pthread_mutex_lock(&memoria_cache_mtx);
	memcpy((void*)particion_libre->base, datos, tamanio); //copio a la memoria
	pthread_mutex_unlock(&memoria_cache_mtx);

//	pthread_mutex_lock(&lista_particiones_mtx);
	if(particion_libre->tamanio != tamanio){ //si no entro justo (mismo tamanio), significa que queda una nueva particion de menor tamanio libre
		t_particion* particion_nueva = malloc(sizeof(t_particion));														//pero si el sobrante es menor a la cantidad minima no se creara una particion nueva
		particion_nueva->base = particion_libre->base + tamanio;
		particion_nueva->tamanio = particion_libre->tamanio - tamanio;
		particion_nueva->id_mensaje = 0;
		particion_nueva->ultimo_acceso = timestamp(&(particion_nueva->fecha));
		particion_nueva->ocupado = false;
		particion_libre->tamanio = tamanio;

		list_add(particiones, particion_nueva);
	}
	particion_libre->ultimo_acceso = timestamp(&(particion_libre->fecha)); //ya viene de antes con el bit de ocupado en true asi que nadie lo va a elegir (no hace falta semaforo)
	particion_libre->cola = codigo_op;
	particion_libre->id_mensaje = id;
//	pthread_mutex_unlock(&lista_particiones_mtx);

}

t_buffer_broker* deserializar_broker_ida(void* buffer, uint32_t size){ //eso hay que probarlo que onda


	t_buffer_broker* buffer_broker = malloc(sizeof(t_buffer_broker));

	uint32_t tamanio_mensaje = size - sizeof(uint32_t);

	buffer_broker->buffer = malloc(tamanio_mensaje); //??????
	buffer_broker->tamanio = tamanio_mensaje;
	int offset = 0;
	int id = 0;

	void* stream = malloc(tamanio_mensaje);

	memcpy(&id, buffer + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream, buffer + offset, tamanio_mensaje);

	buffer_broker->id = id;
	buffer_broker->correlation_id = 0; //no tiene corr id
	buffer_broker->buffer = stream;

	return buffer_broker;
}

t_buffer_broker* deserializar_broker_vuelta(void* buffer, uint32_t size){

	t_buffer_broker* buffer_broker = malloc(sizeof(t_buffer_broker));

	uint32_t tamanio_mensaje = size - sizeof(uint32_t)*2;

	buffer_broker->buffer = malloc(tamanio_mensaje);
	buffer_broker->tamanio = tamanio_mensaje;
	int offset = 0;
	int id = 0;
	int cid = 0;

	void* stream = malloc(tamanio_mensaje);

	memcpy(&id, buffer + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(&cid, buffer + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream, buffer + offset, tamanio_mensaje);

	buffer_broker->id = id;
	buffer_broker->correlation_id = cid;
	buffer_broker->buffer = stream;

	return buffer_broker;
}

t_particion* almacenar_datos_buddy(void* datos, int tamanio,op_code cod_op,uint32_t id_mensaje){

	t_particion* bloque_buddy_particion = eleccion_particion_asignada_buddy(tamanio);

	while(bloque_buddy_particion == NULL){

		eleccion_victima_buddy(tamanio);

		bloque_buddy_particion = eleccion_particion_asignada_buddy(tamanio);
	}
	asignar_particion_buddy(bloque_buddy_particion,datos,tamanio,cod_op,id_mensaje);

	pthread_mutex_lock(&logger_mutex);
	log_info(logger,"Se ha guardado el mensaje %d en la posicion %p", id_mensaje, (void*)bloque_buddy_particion->base);
	pthread_mutex_unlock(&logger_mutex);

	return bloque_buddy_particion;
}


void eleccion_victima_buddy(int tamanio){
	switch(configuracion_cache->algoritmo_reemplazo){
	case FIFO:
		eleccion_victima_fifo_buddy(tamanio);
		break;
	case LRU:
		eleccion_victima_lru_buddy(tamanio);
		break;
	}
}
void asignar_particion_buddy(t_particion* bloque_buddy_particion, void* datos, int tamanio,op_code cod_op,uint32_t id_mensaje){

	pthread_mutex_lock(&memoria_cache_mtx);
	memcpy((void*)bloque_buddy_particion->base, datos, tamanio); //copio a la memoria
	pthread_mutex_unlock(&memoria_cache_mtx);

	bloque_buddy_particion->id_mensaje = id_mensaje;
	bloque_buddy_particion->cola = cod_op;
	bloque_buddy_particion->ultimo_acceso = timestamp(&(bloque_buddy_particion->fecha));
}


t_particion* eleccion_particion_asignada_buddy(int tamanio){

	t_particion* bloque_elegido;
	bool _encontrar_bloque_valido_buddy(void* bloque_buddy){
		return encontrar_bloque_valido_buddy(bloque_buddy,tamanio);
	}

	bool _ordenar_menor_a_mayor(void* bloque_buddy,void* bloque_buddy2){
		return ordenar_menor_a_mayor(bloque_buddy,bloque_buddy2);
	}

	pthread_mutex_lock(&memoria_buddy_mutex);
	t_list* lista_bloques_validos = list_filter(memoria_buddy, (void*)_encontrar_bloque_valido_buddy);

	if(!list_is_empty(lista_bloques_validos)){

		list_sort(lista_bloques_validos, _ordenar_menor_a_mayor);
		bloque_elegido = lista_bloques_validos->head->data;

		//Preguntar si me entra en el bloque, si esta disponible y si el bloque es > a tamanio minimo
		//Si entra divido por 2 y vuelvo a preguntar lo mismo que antes asi hasta llegar al tamanio minimo
		//O hasta llegar a que no entre.
		//Si no entra, lo guardo en la ultima particion que entraba.

		while(puede_partirse(bloque_elegido,tamanio)){
			bloque_elegido = generar_particion_buddy(bloque_elegido);
		}
		bloque_elegido->ocupado = true;
		pthread_mutex_lock(&id_fifo_mutex);
		id_fifo++;
		bloque_elegido->id = id_fifo;
		pthread_mutex_unlock(&id_fifo_mutex);
	}else{
		bloque_elegido = NULL;
	}
	list_destroy(lista_bloques_validos);
	pthread_mutex_unlock(&memoria_buddy_mutex);
	return bloque_elegido;
}

bool encontrar_bloque_valido_buddy(t_particion* bloque_buddy,int tamanio){
	return (!(bloque_buddy->ocupado) &&	(bloque_buddy->tamanio) >= tamanio);
}

bool ordenar_menor_a_mayor(t_particion* bloque_buddy,t_particion* bloque_buddy2){
	return bloque_buddy->tamanio < bloque_buddy2->tamanio ;
}

bool puede_partirse(t_particion* bloque_buddy,int tamanio){

	int bloque_tamanio_partido= bloque_buddy->tamanio / 2;
	return bloque_tamanio_partido >= tamanio && bloque_tamanio_partido >= configuracion_cache->tamanio_minimo_p;
}

t_particion* generar_particion_buddy(t_particion* bloque_buddy){
	uint32_t base_vieja = bloque_buddy->base;

	t_particion* bloque_buddy2 = malloc(sizeof(t_particion));

	uint32_t nuevo_tamanio = bloque_buddy->tamanio / 2;
	bloque_buddy->tamanio = nuevo_tamanio;
	bloque_buddy2->tamanio = nuevo_tamanio;
	bloque_buddy2->base = base_vieja + nuevo_tamanio;
	bloque_buddy->ocupado = false;
	bloque_buddy->base = base_vieja;
	bloque_buddy2->ocupado = false;
	bloque_buddy->id_mensaje = 0;
	bloque_buddy->ultimo_acceso = timestamp(&(bloque_buddy->fecha));
	bloque_buddy2->id_mensaje = 0;
	bloque_buddy2->ultimo_acceso = timestamp(&(bloque_buddy2->fecha));

	list_add(memoria_buddy,bloque_buddy2);

	return bloque_buddy2;
}


bool mismo_id_buddy(t_particion* bloque_buddy,uint32_t id_viejo){
	return bloque_buddy->id == id_viejo;
}

void eleccion_victima_fifo_buddy(int tamanio){

	bool _sort_byId_memoria_buddy(t_particion* bloque_buddy,t_particion* bloque_buddy2){
		return sort_byId_memoria_buddy(bloque_buddy,bloque_buddy2);
	}

	bool _esta_ocupada(t_particion* bloque_buddy){
		return bloque_buddy->ocupado;
	}

	pthread_mutex_lock(&memoria_buddy_mutex);
	list_sort(memoria_buddy,(void*)_sort_byId_memoria_buddy);
	t_particion* victima_elegida = list_find(memoria_buddy, (void*)_esta_ocupada);
	pthread_mutex_unlock(&memoria_buddy_mutex);

	eliminar_mensaje(victima_elegida);

	//consolidar
	consolidar_buddy(victima_elegida,memoria_buddy);
}

bool remove_by_id(t_particion* bloque_buddy,uint32_t id_remover){
	return bloque_buddy->id == id_remover;
}

void consolidar_buddy(t_particion* bloque_buddy_old,t_list* lista_fifo_buddy){

	bool _validar_condicion_fifo_buddy(t_particion* bloque_buddy){

		return validar_condicion_fifo_buddy(bloque_buddy, bloque_buddy_old);
	}

	t_particion* buddy = list_find(lista_fifo_buddy,(void*)_validar_condicion_fifo_buddy);

	while(buddy != NULL){
		bloque_buddy_old = encontrar_y_consolidar_buddy(buddy, bloque_buddy_old);
		if(bloque_buddy_old != NULL)
			buddy = list_find(lista_fifo_buddy,(void*)_validar_condicion_fifo_buddy);
		else buddy = NULL;
	}
}

bool validar_condicion_fifo_buddy(t_particion* bloque_buddy,t_particion* bloque_buddy_old){
	return (bloque_buddy->tamanio == bloque_buddy_old->tamanio)
			&& ((bloque_buddy->base - (int)memoria_cache) == ((bloque_buddy_old->base -(int)memoria_cache) ^ bloque_buddy->tamanio)) &&
			((bloque_buddy_old->base - (int)memoria_cache) == (((bloque_buddy->base - (int)memoria_cache)) ^ bloque_buddy_old->tamanio));
}

t_particion* encontrar_y_consolidar_buddy(t_particion* bloque_buddy,t_particion* bloque_buddy_old){

	if(!bloque_buddy->ocupado){

		puts("encontro buddy libre");

		t_particion* bloque_buddy_new = malloc(sizeof(t_particion));
		bloque_buddy_new->tamanio = bloque_buddy_old->tamanio * 2;
		bloque_buddy_new->ocupado = false;
		bloque_buddy_new->id_mensaje = 0;
		bloque_buddy_new->cola = 0;
		bloque_buddy_new->ultimo_acceso = timestamp(&(bloque_buddy_new->fecha));

		puts("armo nuevo buddy");

		pthread_mutex_lock(&logger_mutex);
		log_info(logger,"Se asociaron las siguientes particiones del buddy: %p - %p", (void*)bloque_buddy_old->base, (void*)bloque_buddy->base);
		pthread_mutex_unlock(&logger_mutex);

		if(bloque_buddy_old->base < bloque_buddy->base){

			bloque_buddy_new->base = bloque_buddy_old->base;

		}else{

			bloque_buddy_new->base = bloque_buddy->base;

		}

		puts("puso la base del buddy");

		//
		bool _mismo_id_buddy1(t_particion* bloque_buddy_1){
			return mismo_id_buddy(bloque_buddy_1,bloque_buddy_old->id);
		}

		bool _mismo_id_buddy2(t_particion* bloque_buddy_2){
			return mismo_id_buddy(bloque_buddy_2,bloque_buddy->id);
		}

		pthread_mutex_lock(&memoria_buddy_mutex);

		t_particion* bloque_a_remover_1 = list_remove_by_condition(memoria_buddy,(void*)_mismo_id_buddy1);
		free(bloque_a_remover_1);

		t_particion* bloque_a_remover_2 = list_remove_by_condition(memoria_buddy,(void*)_mismo_id_buddy2);
		free(bloque_a_remover_2);
		pthread_mutex_unlock(&memoria_buddy_mutex);
		//
		pthread_mutex_lock(&memoria_buddy_mutex);
		list_add(memoria_buddy,bloque_buddy_new);
		pthread_mutex_unlock(&memoria_buddy_mutex);

		return bloque_buddy_new;
		puts("termino consolidacion");

	}

	return NULL;
}


bool sort_byId_memoria_buddy(t_particion* bloque_buddy,t_particion* bloque_buddy2){
	return bloque_buddy->id < bloque_buddy2->id;
}

bool sort_by_acceso_memoria_buddy(t_particion* bloque_buddy,t_particion* bloque_buddy2){
	return (bloque_buddy->ultimo_acceso) < (bloque_buddy2->ultimo_acceso);
}

void eleccion_victima_lru_buddy(int tamanio){

	bool _orden(t_particion* bloque_buddy,t_particion* bloque_buddy2){
		return bloque_buddy->ultimo_acceso < bloque_buddy2->ultimo_acceso;
	}

	bool _esta_ocupada(t_particion* bloque_buddy){
		return bloque_buddy->ocupado;
	}

	pthread_mutex_lock(&memoria_buddy_mutex);
	list_sort(memoria_buddy,(void*)_orden);
	t_particion* victima_elegida = list_find(memoria_buddy, (void*)_esta_ocupada);
	pthread_mutex_unlock(&memoria_buddy_mutex);

	eliminar_mensaje(victima_elegida);

	//consolidar
	consolidar_buddy(victima_elegida,memoria_buddy);

}

uint64_t timestamp(struct timeval* valor) {
	gettimeofday(valor, NULL);
	unsigned long long result = (((unsigned long long )(*valor).tv_sec) * 1000 + ((unsigned long) (*valor).tv_usec));
	uint64_t tiempo = result;
	return tiempo;
}

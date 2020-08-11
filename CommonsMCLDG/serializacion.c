#include "serializacion.h"

void enviar_mensaje(t_mensaje* mensaje, int socket){

	t_paquete* paquete = malloc(sizeof(t_paquete));

	t_buffer* buffer_cargado = cargar_buffer(mensaje);

	paquete -> buffer = buffer_cargado;

	paquete -> codigo_operacion = mensaje -> tipo_mensaje;

	int bytes = 0;

	void* a_enviar = serializar_paquete(paquete, &bytes);

	send(socket,a_enviar,bytes,0);

	free(a_enviar);
	free(paquete -> buffer->stream);
	free(paquete->buffer);
	free(paquete);

}

void* serializar_paquete(t_paquete* paquete, int *bytes){

	int size = sizeof(uint32_t) + paquete->buffer->size + sizeof(op_code);

	void* a_enviar = malloc (size);

	memcpy(a_enviar + *bytes, &paquete-> codigo_operacion, sizeof(paquete->codigo_operacion));
	*bytes += sizeof(paquete->codigo_operacion);
	memcpy(a_enviar  + *bytes, &(paquete -> buffer -> size),sizeof(int));
	*bytes += sizeof(int);
	memcpy(a_enviar  + *bytes, paquete -> buffer -> stream, paquete -> buffer -> size);
	*bytes += paquete->buffer->size;

	return a_enviar;
}


t_buffer* cargar_buffer(t_mensaje* mensaje){

	uint32_t proceso = mensaje -> tipo_mensaje;
	char** parametros = mensaje -> parametros;

	switch(proceso){
		case(NEW_POKEMON): return buffer_new_pokemon(parametros);
		case(GET_POKEMON): return buffer_get_pokemon(parametros);
		case(APPEARED_POKEMON): return buffer_position_and_name(parametros);
		case(CATCH_POKEMON): return buffer_position_and_name(parametros);
		case(CAUGHT_POKEMON): return buffer_caught_pokemon(parametros);
		case(LOCALIZED_POKEMON): return buffer_localized_pokemon(parametros);
		case(SUSCRIPCION): return buffer_suscripcion(parametros);
		case(ACK): return buffer_ack(parametros);
	}
	return 0;
}

///////////////////////////SERIALIZAR/////////////////////////////////
t_buffer* buffer_localized_pokemon(char** parametros){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	uint32_t cantidad = atoi(parametros[1]);
	uint32_t cantidadParametros = cantidad*2;
	t_localized_pokemon localized_pokemon;

	localized_pokemon.nombre.largo_nombre = strlen(parametros[0]);
	localized_pokemon.nombre.nombre = parametros[0];
	localized_pokemon.cantidad = cantidad;

	buffer -> size = sizeof(uint32_t)*(cantidadParametros+4) + localized_pokemon.nombre.largo_nombre;
	void* stream = malloc(buffer -> size);
	int offset = 0;

	localized_pokemon.id = atoi(parametros[cantidadParametros+2]);
	localized_pokemon.correlation_id = atoi(parametros[cantidadParametros+3]);
	memcpy(stream + offset, &localized_pokemon.id, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &localized_pokemon.correlation_id, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &localized_pokemon.cantidad, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	int pos_x;
	int pos_y;

	for(int i = 2; cantidadParametros+2>i ; i++){
		pos_x= atoi(parametros[i]);
		i++;
		pos_y = atoi(parametros[i]);

		memcpy(stream + offset, &pos_x, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		memcpy(stream + offset, &pos_y, sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	memcpy(stream + offset, &localized_pokemon.nombre.largo_nombre, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, localized_pokemon.nombre.nombre, localized_pokemon.nombre.largo_nombre);


	buffer -> stream = stream;
	return buffer;
}

t_buffer* buffer_caught_pokemon(char** parametros){

	t_buffer* buffer = malloc(sizeof(t_buffer));
	uint32_t caught = atoi(parametros[0]);

	t_caught_pokemon caught_pokemon;
	caught_pokemon.caught = caught;
	caught_pokemon.id = atoi(parametros[1]);
	caught_pokemon.correlation_id = atoi(parametros[2]);

	buffer -> size = sizeof(uint32_t)*3;
	int offset = 0;
	void* stream = malloc(buffer -> size);

	memcpy(stream + offset, &caught_pokemon.id, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &caught_pokemon.correlation_id, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &caught_pokemon.caught, sizeof(uint32_t));
	offset += sizeof(uint32_t);


	buffer -> stream = stream;

	//free(stream);

	return buffer;
}

t_buffer* buffer_position_and_name(char** parametros){ //para mensajes APPEARED_POKEMON y CATCH_POKEMON

	t_buffer* buffer = malloc(sizeof(t_buffer));

	uint32_t pos_x = atoi(parametros[1]);
	uint32_t pos_y = atoi(parametros[2]);

	t_position_and_name position_and_name;

	position_and_name.nombre.largo_nombre = strlen(parametros[0]);
	position_and_name.nombre.nombre = parametros[0];

	position_and_name.coordenadas.pos_x = pos_x;
	position_and_name.coordenadas.pos_y = pos_y;
	position_and_name.id = atoi(parametros[3]);
	if(parametros[4] == NULL) position_and_name.correlation_id = 0;
	else position_and_name.correlation_id = atoi(parametros[4]);

	buffer -> size = sizeof(uint32_t)*5 + position_and_name.nombre.largo_nombre;

	void* stream = malloc(buffer -> size);
	int offset = 0;
	memcpy(stream + offset, &position_and_name.id, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &position_and_name.correlation_id, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &position_and_name.coordenadas.pos_y, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &position_and_name.coordenadas.pos_x, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &position_and_name.nombre.largo_nombre, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, position_and_name.nombre.nombre, position_and_name.nombre.largo_nombre);

//	free(position_and_name.nombre.nombre);

	buffer -> stream = stream;

	//free(stream);

	return buffer;

}

t_buffer* buffer_get_pokemon(char** parametros){

	t_buffer* buffer = malloc(sizeof(t_buffer));

	t_get_pokemon get_pokemon;

	get_pokemon.nombre.largo_nombre = strlen(parametros[0]);
	get_pokemon.nombre.nombre =  parametros[0];

	get_pokemon.id = atoi(parametros[1]);

	buffer -> size = sizeof(uint32_t)*2 + get_pokemon.nombre.largo_nombre;

	void* stream = malloc(buffer -> size);
	int offset = 0;
	memcpy(stream + offset, &get_pokemon.id, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &get_pokemon.nombre.largo_nombre, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, get_pokemon.nombre.nombre, get_pokemon.nombre.largo_nombre);

//	free(get_pokemon.nombre.nombre);

	buffer -> stream = stream;

	return buffer;
}

t_buffer* buffer_new_pokemon(char** parametros){

	t_buffer* buffer = malloc(sizeof(t_buffer));
//	char* nombre = parametros[0];
	uint32_t pos_x = atoi(parametros[1]);
	uint32_t pos_y = atoi(parametros[2]);
	uint32_t cantidad = atoi(parametros[3]);

	t_new_pokemon new_pokemon;

	new_pokemon.nombre.largo_nombre = strlen(parametros[0]);
//	new_pokemon.nombre.nombre = malloc(new_pokemon.nombre.largo_nombre);
	new_pokemon.nombre.nombre = parametros[0];
	new_pokemon.coordenadas.pos_x = pos_x;
	new_pokemon.coordenadas.pos_y = pos_y;
	new_pokemon.cantidad = cantidad;
	new_pokemon.id = atoi(parametros[4]);

	//free(nombre);

	buffer -> size = sizeof(uint32_t)*5 + new_pokemon.nombre.largo_nombre;
	printf("size del mensaje : %d", buffer->size);
	puts("");

	void* stream = malloc(buffer -> size);
	int offset = 0;
	memcpy(stream + offset, &new_pokemon.id, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &new_pokemon.coordenadas.pos_y, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &new_pokemon.coordenadas.pos_x, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &new_pokemon.cantidad, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &new_pokemon.nombre.largo_nombre, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, new_pokemon.nombre.nombre, new_pokemon.nombre.largo_nombre);

	//free(new_pokemon.nombre.nombre);

	buffer -> stream = stream;

	//free(stream);

	return buffer;
}

t_buffer* buffer_suscripcion(char** parametros){

	t_buffer* buffer = malloc(sizeof(t_buffer));
	op_code cola = codigo_mensaje(parametros[0]);

	t_suscripcion suscripcion;
	suscripcion.cola = cola;
	suscripcion.id_proceso = atoi(parametros[1]);

	buffer -> size = sizeof(op_code) + sizeof(int);

	int offset = 0;

	void* stream = malloc(buffer -> size);

	memcpy(stream + offset, &suscripcion.cola, sizeof(op_code));
	offset += sizeof(op_code);
	memcpy(stream + offset, &suscripcion.id_proceso, sizeof(int));

	buffer -> stream = stream;

	//free(stream);

	return buffer;

}

t_buffer* buffer_ack(char** parametros){
	t_buffer* buffer = malloc(sizeof(t_buffer));

	t_ack ack;
	ack.id_mensaje = atoi(parametros[0]);
	ack.id_proceso = atoi(parametros[1]);

	buffer->size = sizeof(uint32_t) * 2;

	void* stream = malloc(buffer->size);

	int offset = 0;

	memcpy(stream + offset, &ack.id_mensaje, sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream + offset, &ack.id_proceso, sizeof(uint32_t));

	buffer->stream = stream;

	return buffer;
}

///////////////////////////DESERIALIZAR/////////////////////////////////

t_ack* deserializar_ack(void* buffer){
	t_ack* ack = malloc(sizeof(t_ack));

	memcpy(&ack->id_mensaje, buffer, sizeof(uint32_t));
	buffer+= sizeof(uint32_t);
	memcpy(&ack->id_proceso, buffer, sizeof(uint32_t));

	return ack;
}

t_get_pokemon* deserializar_get_pokemon(void* buffer){

	t_get_pokemon* get_pokemon = malloc(sizeof(t_get_pokemon));

	memcpy(&get_pokemon->id, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&get_pokemon->nombre.largo_nombre, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	get_pokemon->nombre.nombre = malloc(get_pokemon->nombre.largo_nombre + 1);
	memcpy(get_pokemon->nombre.nombre, buffer, get_pokemon->nombre.largo_nombre);
	get_pokemon->nombre.nombre[get_pokemon->nombre.largo_nombre] = '\0';

	return get_pokemon;
}

t_localized_pokemon* deserializar_localized_pokemon(void*buffer){

	t_localized_pokemon* localized_pokemon = malloc(sizeof(t_localized_pokemon));

	localized_pokemon->listaCoordenadas = list_create();

	memcpy(&localized_pokemon->id, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&localized_pokemon->correlation_id, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&localized_pokemon->cantidad,buffer,  sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	int cantidadParametros = localized_pokemon->cantidad * 2;
	coordenadas_pokemon* coordenadas;

	int pos_x;
	int pos_y;

	for(int i = 0; cantidadParametros>i ; i+=2){
		coordenadas = malloc(sizeof(coordenadas_pokemon));
		memcpy(&pos_x,buffer, sizeof(uint32_t));
		buffer += sizeof(uint32_t);
		memcpy(&pos_y,buffer, sizeof(uint32_t));
		buffer += sizeof(uint32_t);
		coordenadas->pos_x = pos_x;
		coordenadas->pos_y = pos_y;
		list_add(localized_pokemon->listaCoordenadas, coordenadas);
	}

	memcpy(&localized_pokemon->nombre.largo_nombre, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	localized_pokemon->nombre.nombre = malloc(localized_pokemon->nombre.largo_nombre + 1);
	memcpy(localized_pokemon->nombre.nombre,buffer, localized_pokemon->nombre.largo_nombre);
	buffer += localized_pokemon->nombre.largo_nombre;
	localized_pokemon->nombre.nombre[localized_pokemon->nombre.largo_nombre] = '\0';

	return localized_pokemon;

}

t_new_pokemon* deserializar_new_pokemon(void* buffer){

	t_new_pokemon* new_pokemon = malloc(sizeof(t_new_pokemon));

	memcpy(&new_pokemon->id, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy( &new_pokemon->coordenadas.pos_y, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&new_pokemon->coordenadas.pos_x, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&new_pokemon->cantidad, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&new_pokemon->nombre.largo_nombre, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	new_pokemon->nombre.nombre = malloc(new_pokemon->nombre.largo_nombre + 1);
	memcpy(new_pokemon->nombre.nombre, buffer, new_pokemon->nombre.largo_nombre);
	new_pokemon->nombre.nombre[new_pokemon->nombre.largo_nombre] = '\0';


	return new_pokemon;
}

t_position_and_name* deserializar_position_and_name(void* buffer){

	t_position_and_name* position_and_name = malloc(sizeof(t_position_and_name));
	memcpy(&position_and_name->id, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&position_and_name->correlation_id, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy( &position_and_name->coordenadas.pos_y, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&position_and_name->coordenadas.pos_x, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&position_and_name->nombre.largo_nombre, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	position_and_name->nombre.nombre = malloc(position_and_name->nombre.largo_nombre + 1);
	memcpy(position_and_name->nombre.nombre, buffer, position_and_name->nombre.largo_nombre);
	position_and_name->nombre.nombre[position_and_name->nombre.largo_nombre] = '\0';

	return position_and_name;
}

t_caught_pokemon* deserializar_caught_pokemon(void* buffer){

	t_caught_pokemon* caught_pokemon = malloc(sizeof(t_caught_pokemon));
	memcpy(&caught_pokemon->id, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&caught_pokemon->correlation_id, buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&caught_pokemon->caught, buffer, sizeof(uint32_t));

	return caught_pokemon;
}

t_suscripcion* deserializar_suscripcion(void* buffer){

	op_code cola;
	int id_proceso;

	memcpy(&cola, buffer, sizeof(op_code));
	buffer+=sizeof(op_code);
	memcpy(&id_proceso, buffer, sizeof(int));

	t_suscripcion* suscripcion = malloc(sizeof(t_suscripcion));

	suscripcion->cola = cola;
	suscripcion->id_proceso = id_proceso;

	return suscripcion;
}

void enviar_ack(int socket, uint32_t id, int id_proceso){
	op_code codigo_ack = ACK;

	t_mensaje* ack = malloc(sizeof(t_mensaje));

	char* parametros_ack = string_new();
	string_append_with_format(&parametros_ack, "%d,%d", id, id_proceso);

	ack -> tipo_mensaje = codigo_ack;
	ack -> parametros = string_split(parametros_ack, ",");
	free(parametros_ack);
	enviar_mensaje(ack, socket);
	liberar_vector(ack->parametros);
	free(ack);
}

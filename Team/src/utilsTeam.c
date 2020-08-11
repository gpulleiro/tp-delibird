#include "utilsTeam.h"
t_list* entrenadores;
t_list* objetivoGlobal;
t_list* pokemonsRequeridos;
t_list* pokemonsDeRepuesto;
t_list* especiesNecesarias;
t_list* IDs_get_pokemon;
t_list* ready;
t_queue* colaAppearedPokemon;
t_queue* colaLocalizedPokemon;
t_queue* colaCaughtPokemon;


pthread_mutex_t objetivo, requeridos, mutex_ready, mutex_cola_appeared_pokemon, mutex_cola_caught_pokemon, mutex_cola_localized_pokemon,
log_mutex, mutex_lista_entrenadores, mutex_deadlock, mutex_ejecutar, repuesto;
sem_t conexiones, sem_deteccionDeadlock,  sem_ejecutar,  semAppeared, semCaught, semLocalized, entrenadoresPlanificados,
sem_ready;

t_config* config;
t_log* logger;
t_algoritmo_planificacion algoritmoPlanificacion;

int ciclosCPUGlobal;

char* estado[5];
int cambiosDeContexto;


void iniciarTeam(){
	pthread_mutex_init(&objetivo, NULL);
	pthread_mutex_init(&requeridos, NULL);
	pthread_mutex_init(&repuesto, NULL);
	pthread_mutex_init(&mutex_ready, NULL);
	//pthread_mutex_init(&sem_ready, NULL);
	pthread_mutex_init(&mutex_ejecutar, NULL);
	pthread_mutex_init(&mutex_cola_appeared_pokemon, NULL);
	pthread_mutex_init(&mutex_cola_caught_pokemon, NULL);
	pthread_mutex_init(&mutex_cola_localized_pokemon, NULL);
	pthread_mutex_init(&log_mutex, NULL);
	pthread_mutex_init(&mutex_lista_entrenadores, NULL);
	pthread_mutex_init(&mutex_deadlock, NULL);
	sem_init(&conexiones, 0,0);
	//	sem_init(&sem_ejecutar,0,0);
	sem_init(&sem_deteccionDeadlock, 0, 0);
	sem_init(&semAppeared,0,0);
	sem_init(&semLocalized,0,0);
	sem_init(&semCaught,0,0);
	//	sem_init(&entrenadoresPlanificados,0,0);
	sem_init(&sem_ejecutar,0,0);
	//	sem_init(&mutex_ejecutar,0,1);

	estado[1] = "NEW";
	estado[2] = "READY";
	estado[3] ="EXEC";
	estado[4] = "BLOCK";
	estado[5] ="EXIT";
	cambiosDeContexto = 0;

	entrenadores = list_create();
	objetivoGlobal = list_create();
	especiesNecesarias = list_create();
	pokemonsRequeridos = list_create();
	pokemonsDeRepuesto = list_create();
	IDs_get_pokemon = list_create();
	ready = list_create();
	colaAppearedPokemon = queue_create();
	colaCaughtPokemon = queue_create();
	colaLocalizedPokemon = queue_create();

	ciclosCPUGlobal = 0;

	config = leer_config(PATH);
	//	algoritmoPlanificacion = malloc(sizeof(algoritmoPlanificacion));
	char* algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	if(strcmp(algoritmo, "FIFO") == 0) algoritmoPlanificacion.tipoAlgoritmo = FIFO;
	if(strcmp(algoritmo, "RR") == 0) algoritmoPlanificacion.tipoAlgoritmo = RR;
	if(strcmp(algoritmo, "SJF-CD") == 0) algoritmoPlanificacion.tipoAlgoritmo = SJFCD;
	if(strcmp(algoritmo, "SJF-SD") == 0) algoritmoPlanificacion.tipoAlgoritmo = SJFSD;
	//	char* alpha = config_get_string_value(config, "ALPHA");

	algoritmoPlanificacion.alpha = config_get_double_value(config, "ALPHA");
	algoritmoPlanificacion.estimacion_inicial = config_get_double_value(config, "ESTIMACION_INICIAL");
	algoritmoPlanificacion.quantum = config_get_int_value(config, "QUANTUM");

	printf("alpha: %f estimacion inicial: %f quantum: %d", algoritmoPlanificacion.alpha, algoritmoPlanificacion.estimacion_inicial, algoritmoPlanificacion.quantum);

	logger = iniciar_logger(config);

	configurarEntrenadores(config);
	configurarObjetivoGlobal();

	sem_init(&entrenadoresPlanificados,0,0);
	for(int i=0; i<entrenadores->elements_count; i++){
		sem_post(&entrenadoresPlanificados);

	}

	void _agregarEspecie(void* pokemon){
		return agregarEspecie(pokemon, especiesNecesarias);
	}
	list_iterate(objetivoGlobal, (void*)_agregarEspecie);

	pthread_t conexionBroker;
	pthread_create(&conexionBroker, NULL, (void*)crearConexiones, NULL);
	pthread_detach(conexionBroker);

	pthread_t recibirAppearedPokemon;
	pthread_create(&recibirAppearedPokemon, NULL, (void*)appeared_pokemon, NULL);
	pthread_detach(recibirAppearedPokemon);
	pthread_t recibirLocalizedPokemon;
	pthread_create(&recibirLocalizedPokemon, NULL, (void*)localized_pokemon, NULL);
	pthread_detach(recibirLocalizedPokemon);
	pthread_t recibirCaughtPokemon;
	pthread_create(&recibirCaughtPokemon, NULL, (void*)caught_pokemon, NULL);
	pthread_detach(recibirCaughtPokemon);

	pthread_t conexionGameBoy;
	pthread_create(&conexionGameBoy, NULL, (void*)connect_gameboy, NULL);
	pthread_detach(conexionGameBoy);
	pthread_t thread_deadlock;
	pthread_create(&thread_deadlock, NULL, (void*)deteccionDeadlock, NULL);


	pthread_t hiloEntrenador[entrenadores->elements_count];
	t_link_element * aux = entrenadores->head;
	for(int j=0; j<entrenadores->elements_count; j++){
		pthread_create(&hiloEntrenador[j],NULL, (void*)entrenadorMaster,(void*)((aux->data)));
		aux = aux->next;
		//join o detatch del hilo ??
	}
	planificar();
	pthread_join(thread_deadlock, NULL);

}

void agregarEspecie(char* pokemon, t_list* especiesNecesarias){//funciona
	bool _mismaEspecie(char* especie){
		return mismaEspecie(especie, pokemon);
	}
	if(!list_any_satisfy(especiesNecesarias, (void*) _mismaEspecie )) list_add(especiesNecesarias, pokemon);
}

void terminarTeam(int conexion)//revisar memoria y probar si funciona
{
	pthread_mutex_lock(&log_mutex);
	t_link_element* aux = entrenadores->head;
	t_entrenador* entrenador;
	char* ciclosPorEntrenador = string_new();
	for(int j =0; j<entrenadores->elements_count; j++){

		entrenador = aux->data;
		string_append_with_format(&ciclosPorEntrenador, ", ciclos entrenador %d: %d", entrenador->ID, entrenador->CiclosCPU);
		aux= aux->next;
	}
	log_info(logger, "Cantidad total de ciclos %d, Cantidad de cambios de contexto %d%s", ciclosCPUGlobal, cambiosDeContexto, ciclosPorEntrenador); //agregar variables
	pthread_mutex_unlock(&log_mutex);
	free(ciclosPorEntrenador);
	void _entrenadorDestroy(void* entrenador){
		return entrenadorDestroy( entrenador);
	}
	for(int i = 0; i<entrenadores->elements_count ; i++){
		//destruir hilos
	}
	pthread_mutex_destroy(&requeridos);
	pthread_mutex_destroy(&objetivo);
	pthread_mutex_destroy(&mutex_ready);
	pthread_mutex_destroy(&mutex_ejecutar);
	pthread_mutex_destroy(&mutex_cola_appeared_pokemon);
	pthread_mutex_destroy(&mutex_cola_caught_pokemon);
	pthread_mutex_destroy(&mutex_cola_localized_pokemon);
	pthread_mutex_destroy(&log_mutex);
	pthread_mutex_destroy(&mutex_lista_entrenadores);
	pthread_mutex_destroy(&mutex_deadlock);
	pthread_mutex_destroy(&repuesto);

	sem_destroy(&conexiones);
	sem_destroy(&sem_deteccionDeadlock);
	sem_destroy(&sem_ejecutar);
	sem_destroy(&semAppeared);
	sem_destroy(&semCaught);
	sem_destroy(&semLocalized);
	sem_destroy(&entrenadoresPlanificados);
	sem_destroy(&sem_ready);

//	void _destruirPokemon(t_pokemon* pokemon){
////		free(pokemon->especie);
//		free(pokemon);
//	}

//	list_destroy_and_destroy_elements(entrenadores, _entrenadorDestroy);
//	list_destroy(objetivoGlobal);
//	list_destroy(ready);
//	list_destroy(pokemonsRequeridos);
//	list_destroy(pokemonsDeRepuesto);
//	list_destroy(IDs_get_pokemon);
//	list_destroy(especiesNecesarias);
//
//	queue_destroy(colaAppearedPokemon);
//	queue_destroy(colaLocalizedPokemon);
//	queue_destroy(colaCaughtPokemon);


	config_destroy(config);
	//liberar_conexion(conexion);
	pthread_mutex_lock(&log_mutex);
	log_info(logger,"-----------LOG END--------"); //borrar
	pthread_mutex_unlock(&log_mutex);
	log_destroy(logger);
}

void entrenadorDestroy(t_entrenador * entrenador) { //probar con valgrind

	//	void destruirElemento(char* elemento){
	//		free(elemento);
	//	}
	//	list_destroy_and_destroy_elements(entrenador->objetivos, (void*) destruirElemento);
	list_destroy(entrenador->objetivos);
	//	list_destroy_and_destroy_elements(entrenador->pokemons, (void*) destruirElemento);
	list_destroy(entrenador->pokemons);
	//	list_destroy_and_destroy_elements(entrenador->pokemonsNoNecesarios, (void*) destruirElemento); se rompe si lo uso
	list_destroy(entrenador->pokemonsNoNecesarios);
	free(entrenador);
}

void configurarEntrenadores(){ //funciona

	char** posiciones = config_get_array_value(config, "POSICIONES_ENTRENADORES");
	char** pokemonEntrenadores = config_get_array_value(config, "POKEMON_ENTRENADORES");
	char** objetivos = config_get_array_value(config, "OBJETIVOS_ENTRENADORES");
	bool hayMasPokemons = true;
	for(int i=0; posiciones[i];i++){
		t_entrenador* entrenador;
		if(pokemonEntrenadores[i] == NULL) {
			hayMasPokemons =false;
		}
		if(hayMasPokemons){
			entrenador = crearEntrenador(posiciones[i], pokemonEntrenadores[i], objetivos[i], i);
		}else{
			entrenador = crearEntrenador(posiciones[i], NULL, objetivos[i], i);
		}
		list_add(entrenadores, entrenador);
	}

	liberar_vector(posiciones);
	liberar_vector(pokemonEntrenadores);
	liberar_vector(objetivos);

	return ;
}

t_entrenador* crearEntrenador(char* posicion, char* pokemonsEntrenador, char* objetivos, int ID){ //funciona
	t_entrenador* entrenador = malloc(sizeof(t_entrenador));
	entrenador->ID = ID;
	entrenador->estado = BLOCK;
	char** objetivosEntrenador = string_split(objetivos,"|");
	entrenador->objetivos = configurarPokemons(objetivosEntrenador);
	char** coordenadas = string_split(posicion,"|");
	entrenador->coordx = atoi(coordenadas[0]);
	entrenador->coordy = atoi(coordenadas[1]);
	if(pokemonsEntrenador != NULL){
		char** pokemons = string_split(pokemonsEntrenador,"|");
		entrenador->pokemons = configurarPokemons(pokemons);
		entrenador->pokemonsNoNecesarios = list_duplicate(entrenador->pokemons);
		void _eliminarPokemonsObjetivo(void* pokemon){
			return eliminarPokemonsObjetivoParticular(pokemon, entrenador->pokemonsNoNecesarios);
		}
		list_iterate(entrenador->objetivos, (void*)_eliminarPokemonsObjetivo);
	}else{
		entrenador->pokemons = list_create();
		entrenador->pokemonsNoNecesarios = list_create();
	}
	entrenador->pokemonACapturar = NULL;
	entrenador->intercambio = NULL;
	entrenador->CiclosCPU = 0;
	entrenador->rafagaReal = 0;
	entrenador->catch_id = 0;
	entrenador->estimacion = algoritmoPlanificacion.estimacion_inicial;
	entrenador->restoEstimacion = algoritmoPlanificacion.estimacion_inicial;
	printf("estimacion inicial entrenador %d : %f", entrenador->ID, entrenador->estimacion);
	pthread_mutex_init(&(entrenador->mutex), NULL);
	pthread_mutex_lock(&(entrenador->mutex));

	liberar_vector(coordenadas);


	return entrenador;
}


void eliminarPokemonsObjetivoParticular(char* pokemon, t_list* pokemonsNoNecesarios){
	bool _mismaEspecie(void* pokemonNoNecesario){
		return mismaEspecie(pokemonNoNecesario, pokemon);
	}
	t_pokemon* pokemonAEliminar = list_remove_by_condition(pokemonsNoNecesarios, _mismaEspecie);
//	if(pokemonAEliminar != NULL) free(pokemonAEliminar->especie);
//	free(pokemonAEliminar);
}

t_list* configurarPokemons(char** pokemons){ //funciona //objetivo global
	t_list* listaPokemons = list_create();
	for(int i=0; pokemons[i];i++){
		if(pokemons[i]!= NULL){
			list_add(listaPokemons, (pokemons[i]));
		}
	}
	free(pokemons);
	return listaPokemons;
}

void cambiarEstado (t_entrenador** entrenador,t_estado nuevoEstado, char* razonDeCambio){ //funciona
	if((*entrenador)->estado == nuevoEstado) return;
	if(nuevoEstado == EXEC) cambiosDeContexto ++;
	if(cambioEstadoValido((*entrenador)->estado, nuevoEstado)){
		(*entrenador)->estado = nuevoEstado;
		pthread_mutex_lock(&log_mutex);
		log_info(logger, "Se cambio al entrenador %d a la cola %s porque %s", (*entrenador)->ID, stringEstado((*entrenador)->estado), razonDeCambio);
		pthread_mutex_unlock(&log_mutex);
	}else {
		printf("Estado invalido");
	}
}

bool cambioEstadoValido(t_estado estadoViejo,t_estado nuevoEstado){ //funciona
	switch (estadoViejo){
	case NEW:
		if(nuevoEstado == READY) return true;
		else return false;
		break;
	case READY:
		if(nuevoEstado == EXEC) return true;
		else return false;
		break;
	case EXEC:
		if(nuevoEstado == READY || nuevoEstado == BLOCK || nuevoEstado == EXIT || nuevoEstado == EXEC) return true;
		else return false;
		break;
	case BLOCK:
		if(nuevoEstado == READY || nuevoEstado == BLOCK || nuevoEstado == EXIT) return true;
		else return false;
		break;
	case EXIT:
		return false;
		break;
		//	return true;
	}
	return false;
}

char* stringEstado(t_estado estado){
	switch (estado){
	case NEW:
		return "NEW";
		break;
	case READY:
		return "READY";
		break;
	case EXEC:
		return "EXEC";
		break;
	case BLOCK:
		return "BLOCK";
		break;
	case EXIT:
		return "EXIT";
		break;
	}
}

bool cumpleObjetivoGlobal(){ //funciona
	bool _esEstadoExit(void* entrenador){
		return esEstadoExit(entrenador);
	}
	//	pthread_mutex_lock(&mutex_lista_entrenadores);
	return list_all_satisfy(entrenadores,_esEstadoExit);
	//	pthread_mutex_unlock(&mutex_lista_entrenadores);

}

bool esEstadoExit(t_entrenador* entrenador){//funciona
	return entrenador->estado == EXIT;
}

bool cumpleObjetivoParticular (t_entrenador* entrenador){//funciona
	if(entrenador->pokemons->elements_count != entrenador->objetivos->elements_count) return false;
	return listasIguales(entrenador->objetivos, entrenador->pokemons);
}

bool tieneMenosElementos (t_list* listaChica, t_list* lista ){	//funciona
	if(listaChica->elements_count < lista->elements_count) return true;
	return false;
}

bool listasIguales(t_list* lista1, t_list* lista2){ //funciona
	bool _criterioOrden(void* elem1 , void* elem2){
		return criterioOrden(elem1, elem2);
	}
	list_sort(lista1, _criterioOrden);
	list_sort(lista2, _criterioOrden);
	t_link_element* list1 = lista1->head;
	t_link_element* list2 = lista2->head;

	while(list1){
		if(string_equals_ignore_case(list1->data, list2->data)) {
			list1 = list1->next;
			list2 = list2->next;
		}
		else return false;
	}
	return true;
}

bool criterioOrden(char* elem1, char* elem2){ //funciona
	string_to_lower(elem1);
	string_to_lower(elem2);
	return (0 < strcmp(elem1, elem2));
}

bool puedeAtraparPokemon(t_entrenador* entrenador){ //funciona
	return ((entrenador->estado == NEW || entrenador->estado == BLOCK) && (tieneMenosElementos (entrenador->pokemons, entrenador->objetivos)));
}

bool tieneEspacioYEstaEnExec(t_entrenador* entrenador){ //funciona
	return (entrenador->estado == EXEC && (tieneMenosElementos (entrenador->pokemons, entrenador->objetivos)));
}


bool mismoPokemon(t_pokemon* pokemon,t_pokemon* pokemon2){

	return(pokemon->coordx == pokemon2->coordx && pokemon->coordy == pokemon2->coordy &&
			string_equals_ignore_case(pokemon->especie, pokemon2->especie) &&
			pokemon->planificado == true);

}

bool necesitaPokemon(t_entrenador* entrenador, char* pokemon){ //funciona
	bool _mismaEspecie(char* especie){
		return mismaEspecie(especie, pokemon);
	}

	int necesarios = list_count_satisfying(entrenador->objetivos, (void*)_mismaEspecie);
	int capturados = list_count_satisfying(entrenador->pokemons, (void*)_mismaEspecie);

	if(necesarios == 0) return false;
	if(necesarios > capturados) return true;
	else return false;
}

bool mismaEspecie(char* especie,char* especie2){

	return(string_equals_ignore_case(especie, especie2));

}

void configurarObjetivoGlobal(){ //funciona
	t_link_element *entrenador = entrenadores->head;
	t_link_element *aux = NULL;
	char* pokemon = NULL;
	t_list* pokemons = list_create();
	while(entrenador!= NULL){
		aux = entrenador->next;
		t_entrenador* entre = entrenador->data;
		list_add_all(objetivoGlobal, entre->objetivos);
		list_add_all(pokemons, entre->pokemons);
		entrenador = aux;
	}

	while(!(list_is_empty(pokemons))){
		pokemon = list_remove(pokemons, 0);
		removerPokemon(pokemon,objetivoGlobal);
		//		free(pokemon);
	}
	free(pokemons);

}

char* removerPokemon(char* pokemon, t_list* lista){ //funciona
	bool mismoPokemon(char* pokemon1 ){
		return string_equals_ignore_case(pokemon, pokemon1);
	}
	return list_remove_by_condition(lista, (void*)mismoPokemon);

}

int distancia(int x1, int y1, int x2, int y2){ //funciona

	return abs(x2-x1)+abs(y2-y1);

}

void moverEntrenador(t_entrenador** entrenador, int x, int y){ //probar si funciona
	int moverse = 1;

	if((*entrenador)->coordx != x){
		if(x < (*entrenador)->coordx)	moverse = -1;
		(*entrenador)->coordx += moverse;
		sleep(config_get_int_value(config,"RETARDO_CICLO_CPU"));
		ciclosCPUGlobal ++;
		(*entrenador)->CiclosCPU ++;
		(*entrenador)->rafagaReal ++;
		(*entrenador)->restoEstimacion --;
		pthread_mutex_lock(&log_mutex);
		log_info(logger,"Se ha movido al entrenador %d a la posicion (%d,%d)", (*entrenador)->ID,(*entrenador)->coordx, (*entrenador)->coordy);
		pthread_mutex_unlock(&log_mutex);
	}else{
		if((*entrenador)->coordy != y){
			if(y < (*entrenador)->coordy)	moverse = -1;
			(*entrenador)->coordy += moverse;
			sleep(config_get_int_value(config,"RETARDO_CICLO_CPU"));
			(*entrenador)->CiclosCPU ++;
			(*entrenador)->rafagaReal ++;
			(*entrenador)->restoEstimacion --;
			ciclosCPUGlobal ++;
			pthread_mutex_lock(&log_mutex);
			log_info(logger,"Se ha movido al entrenador %d a la posicion (%d,%d)", (*entrenador)->ID,(*entrenador)->coordx, (*entrenador)->coordy);
			pthread_mutex_unlock(&log_mutex);
		}
	}
}



void intercambiarPokemon(t_entrenador** entrenador){ // Funciona
	t_intercambio* intercambio = (*entrenador)->intercambio;
	removerPokemon(intercambio->pokemonAEntregar, (*entrenador)->pokemons);
	char* pokemon = removerPokemon(intercambio->pokemonAEntregar, (*entrenador)->pokemonsNoNecesarios);
//	free(pokemon);
	if(!necesitaPokemon(intercambio->entrenador, intercambio->pokemonAEntregar))
		list_add(intercambio->entrenador->pokemonsNoNecesarios, intercambio->pokemonAEntregar);
	list_add(intercambio->entrenador->pokemons, intercambio->pokemonAEntregar);
	removerPokemon(intercambio->pokemonARecibir, intercambio->entrenador->pokemons);
	pokemon = removerPokemon(intercambio->pokemonARecibir, intercambio->entrenador->pokemonsNoNecesarios);
	free(pokemon);
	if(!necesitaPokemon((*entrenador), intercambio->pokemonARecibir))
		list_add((*entrenador)->pokemonsNoNecesarios, intercambio->pokemonARecibir);
	list_add((*entrenador)->pokemons, intercambio->pokemonARecibir);
	pthread_mutex_lock(&log_mutex);
	log_info(logger,"Se ha realizado un intercambio entre el entrenador %d y el entrenador %d", (*entrenador)->ID, intercambio->entrenador->ID);
	pthread_mutex_unlock(&log_mutex);
	(*entrenador)->intercambio = NULL;


	//	if(cumpleObjetivoParticular((*entrenador))) cambiarEstado(entrenador, EXIT, "cumplio su objetivo particular");
	//	else cambiarEstado(entrenador, BLOCK, "termino un intercambio");
	//	if(cumpleObjetivoParticular(intercambio->entrenador)) cambiarEstado(&(intercambio->entrenador), EXIT, "cumplio su objetivo particular");
	//	else cambiarEstado(&(intercambio->entrenador), BLOCK, "termino un intercambio");
	sleep(config_get_int_value(config,"RETARDO_CICLO_CPU")*5);
	(*entrenador)->CiclosCPU += 5;
	ciclosCPUGlobal += 5;
}

//void planificar(){//funciona
//	char* algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
//	if(strcmp(algoritmo, "FIFO") == 0) planificarFIFO();
////	if(strcmp(algoritmo, "RR") == 0) planificarRR();
////	if(strcmp(algoritmo, "SJF-CD") == 0) planificarSJF_CD();
////	if(strcmp(algoritmo, "SJF-SD") == 0) planificarSJF_SD();
//}


void planificar(){
	pthread_t llenaReady;
	pthread_t ejecuta;


	//hilo que llena la lista de ready

	pthread_create(&llenaReady, NULL, (void*)llenarColaReady, NULL);
	pthread_detach(llenaReady);
	//hilo que va decidiendo quien ejecuta
	pthread_create(&ejecuta, NULL, (void*)ejecutaEntrenadores, NULL);
	pthread_detach(ejecuta);
}

//void planificarFIFO(){// funciona
//	pthread_t llenaReady;
//	pthread_t ejecuta;
//
//
//	//hilo que llena la lista de ready
//
//	pthread_create(&llenaReady, NULL, (void*)llenarColaReady, NULL);
//	pthread_detach(llenaReady);
//	//hilo que va decidiendo quien ejecuta
//	pthread_create(&ejecuta, NULL, (void*)ejecutaEntrenadores, NULL);
//	pthread_detach(ejecuta);
//
//}

void ejecutaEntrenadores(){ //agregar semaforos y probar
	t_entrenador* entrenador;

	while(!cumpleObjetivoGlobal()){
		sem_wait(&sem_ejecutar);
		puts("entra a ejecutar");
		pthread_mutex_lock(&mutex_ejecutar);
		//			sem_wait(&mutex_ejecutar);
		pthread_mutex_lock(&mutex_ready);
		entrenador = ready->head->data;//list_get(ready, 0);
		pthread_mutex_unlock(&mutex_ready);
		cambiarEstado(&entrenador, EXEC, "va a ejecutar");
		pthread_mutex_unlock(&((entrenador)->mutex));

	}
	pthread_exit(NULL);
}


void replanificar(){
	t_entrenador* entrenadorActual = ready->head->data;
	t_entrenador* aux;
	bool _mismoId(void* entrenador){
		return mismoID(entrenador, entrenadorActual->ID);
	}
	bool _esEstadoExec(t_entrenador* entrenador){
		return (entrenador->estado == EXEC);
	}

	switch(algoritmoPlanificacion.tipoAlgoritmo){
	case FIFO:
		break;
	case RR:
		entrenadorActual->quantum_usado ++;
		if(entrenadorActual->quantum_usado >= algoritmoPlanificacion.quantum && ready->elements_count>1){
			puts("replanifica por fin de quantum");
			pthread_mutex_lock(&mutex_ready);
			entrenadorActual->quantum_usado = 0;
			cambiarEstado(&entrenadorActual, READY, "fin de quantum");
			list_remove_by_condition(ready, (void*)_mismoId);
			list_add(ready, entrenadorActual);
			pthread_mutex_unlock(&mutex_ready);
		}
		break;
	case SJFCD:
		pthread_mutex_lock(&mutex_ready);
		entrenadorActual = ready->head->data;
		if(entrenadorActual->estado != EXEC){
			aux = list_find(ready, (void*)_esEstadoExec);
			if(aux != NULL){
				cambiarEstado(&aux, READY, "fue desalojado por un proceso con menor estimacion");
			}

		}
		pthread_mutex_unlock(&mutex_ready);

		break;
	case SJFSD:
		break;
	}
	return;
}


void llenarColaReady(){ // probar
	sem_init(&sem_ready,0,0);
	t_entrenador* entrenadorAPlanificar;
	t_pokemon* pokemon;

	//	bool puedePlanificar(t_entrenador* entrenador){
	//		return tieneMenosElementos (entrenador->pokemons, entrenador->objetivos) && entrenador->pokemonACapturar == NULL;
	//	}
	t_link_element * aux;
	t_entrenador* entre;
	bool asignado;
	while(!entrenadoresTienenElInventarioLleno()){
		asignado =false;
		sem_wait(&entrenadoresPlanificados);
		bool _noEstaPlanificado(void* pokemon){
			return noEstaPlanificado(pokemon);
		}
		//		pthread_mutex_lock(&sem_ready);
		sem_wait(&sem_ready);

		puts("entra a llena cola ready");
		pthread_mutex_lock(&requeridos);
		pokemon = list_find(pokemonsRequeridos, _noEstaPlanificado);
		pthread_mutex_unlock(&requeridos);
		bool _menorDistancia(void* elem1 , void* elem2){
			return menorDistancia(elem1, elem2, pokemon);
		}
		bool _puedeSerPlanificado(void* entrenador){
			return puedeSerPlanificado(entrenador);
		}
		//		pthread_mutex_lock(&mutex_lista_entrenadores);
		list_sort(entrenadores, _menorDistancia);
		entrenadorAPlanificar = list_find(entrenadores, _puedeSerPlanificado);

		bool _estaAMismaDistanciaYLoNecesita(t_entrenador* entrenador1){
			return (puedeSerPlanificado(entrenador1) && necesitaPokemon(entrenador1, pokemon->especie)
					&& distancia(entrenadorAPlanificar->coordx, entrenadorAPlanificar->coordy, pokemon->coordx, pokemon->coordy) ==
							distancia(entrenador1->coordx, entrenador1->coordy, pokemon->coordx, pokemon->coordy));
		}
		if(list_any_satisfy(entrenadores, (void*)_estaAMismaDistanciaYLoNecesita)){
			entrenadorAPlanificar = list_find(entrenadores, (void*)_estaAMismaDistanciaYLoNecesita);
		}
		pthread_mutex_unlock(&mutex_lista_entrenadores);
		pthread_mutex_lock(&requeridos);
		//		(entrenadorAPlanificar)->pokemonACapturar = malloc(sizeof(t_pokemon));
		entrenadorAPlanificar->pokemonACapturar = pokemon;
		pokemon->planificado = true;
		entrenadorAPlanificar->quantum_usado = 0;
		pthread_mutex_unlock(&requeridos);
		cambiarEstado((&entrenadorAPlanificar), READY, "se le asigno el pokemon a atrapar");
		pthread_mutex_lock(&mutex_ready);
		if(algoritmoPlanificacion.tipoAlgoritmo == SJFCD){
			aux=ready->head;
			for(int i =0; i<ready->elements_count; i++){
				entre = aux->data;
				if(entrenadorAPlanificar->restoEstimacion < entre->restoEstimacion){
					list_add_in_index(ready, i, entrenadorAPlanificar);
					i = ready->elements_count;
					asignado = true;
				}else{
					aux = aux->next;
				}
			}

			if(asignado == false){
				list_add(ready, (entrenadorAPlanificar));
			}

			pthread_mutex_unlock(&mutex_ready);
			sem_post(&sem_ejecutar);
		}else{
			if(algoritmoPlanificacion.tipoAlgoritmo == SJFSD && ready->elements_count>1){
				aux=ready->head->next;
				for(int i =1; i<ready->elements_count; i++){
					entre = aux->data;
					if(entrenadorAPlanificar->estimacion < entre->estimacion){
						list_add_in_index(ready, i, entrenadorAPlanificar);
						i = ready->elements_count;
						asignado = true;
					}else{
						aux = aux->next;
					}

				}

				if(asignado == false){
					list_add(ready, (entrenadorAPlanificar));
				}
				printf("cantidad de entrenadores en el if en ready: %d", ready->elements_count);


				pthread_mutex_unlock(&mutex_ready);
				sem_post(&sem_ejecutar);
			}else{
				//		pthread_mutex_lock(&mutex_ready);
				list_add(ready, (entrenadorAPlanificar));
				pthread_mutex_unlock(&mutex_ready);
				sem_post(&sem_ejecutar);
				printf("cantidad de entrenadores en ready: %d", ready->elements_count);
			}
		}
	}
	return;
}

bool puedeSerPlanificado(t_entrenador* entrenador){
	return estadoNewOBlock(entrenador) && (tieneMenosElementos (entrenador->pokemons, entrenador->objetivos) && entrenador->pokemonACapturar == NULL);
}

bool noEstaPlanificado(t_pokemon* pokemon){
	return !pokemon->planificado;
}

bool estadoNewOBlock(t_entrenador* entrenador){ //funciona
	return entrenador->estado == NEW || entrenador->estado == BLOCK;
}

bool menorDistancia(t_entrenador* elem1, t_entrenador* elem2, t_pokemon* pokemon){ //funciona
	return (distancia(elem1->coordx, elem1->coordy, pokemon->coordx, pokemon->coordy)<distancia(elem2->coordx, elem2->coordy, pokemon->coordx, pokemon->coordy));
}

void* entrenadorMaster(void* entre){// Probar y agregar semaforos

	t_entrenador* entrenador = entre;
	t_intercambio* intercambio;
	int coordx = 0;
	int coordy = 0;
	bool _mismoID(void* entre){
		mismoID(entre, entrenador->ID);
	}

	while((entrenador)->estado != EXIT){
		pthread_mutex_lock(&((entrenador)->mutex));
		puts("ejecuta un entrenador");
		//		puts(string_itoa(entrenador->ID));
		intercambio = (entrenador)->intercambio;
		if(tieneEspacioYEstaEnExec(entrenador)){
			coordx = (entrenador)->pokemonACapturar->coordx;
			coordy = (entrenador)->pokemonACapturar->coordy;
		}else{
			puts("intercambio");
			coordx = (intercambio->entrenador->coordx);
			coordy = (intercambio->entrenador->coordy);
		}
		pthread_mutex_unlock(&(entrenador->mutex));
		while((coordx != entrenador->coordx	|| coordy != entrenador->coordy)){
			pthread_mutex_lock(&(entrenador->mutex));
			moverEntrenador(&entrenador, coordx, coordy);
			//ver si cambiar a block
			replanificar();
			sem_post(&sem_ejecutar);
			pthread_mutex_unlock(&mutex_ejecutar);
		}
		pthread_mutex_lock(&((entrenador)->mutex));
		if(tieneEspacioYEstaEnExec(entrenador)){

			//			cambiarEstado(&entrenador, BLOCK, "esta esperando el resultado de intentar atrapar pokemon");
			catch_pokemon(config_get_string_value(config, "IP_BROKER"), config_get_string_value(config, "PUERTO_BROKER"), &entrenador);
			puts("mando catch");
			pthread_mutex_unlock(&mutex_ejecutar);
			//			sem_post(&mutex_ejecutar);
			pthread_mutex_lock(&(entrenador->mutex));
			//ver si el resultado es positivo o negativo
			if(entrenador->respuesta_catch){
				capturoPokemon(&entrenador);
				entrenador->quantum_usado = 0;
			}else{
				noCapturoPokemon(&entrenador);
				entrenador->quantum_usado = 0;
			}
		}else { //if(tieneInventarioLlenoOEstaEnExit(entrenador))
			//			pthread_mutex_lock(&((entrenador)->mutex));
			pthread_mutex_lock(&mutex_ready);
			list_remove_by_condition(ready, _mismoID);
			pthread_mutex_unlock(&mutex_ready);
			puts("va a intercambiar");
			intercambiarPokemon(&entrenador);
			pthread_mutex_unlock(&mutex_deadlock);
			pthread_mutex_unlock(&mutex_ejecutar);
			//			sem_post(&mutex_ejecutar);
		}
		if(cumpleObjetivoParticular(entrenador)) cambiarEstado(&entrenador, EXIT, "cumplio su objetivo particular");
	}
	pthread_exit(NULL);
	return 0;
}




void process_request(int cod_op, int cliente_fd) { //funciona

	int size = 0;
	void* buffer = recibir_mensaje(cliente_fd, &size);

	t_position_and_name* appeared;
	t_localized_pokemon* localized;
	t_caught_pokemon* caught;

	bool _mismaEspecie(char* especie1){
		return mismaEspecie(especie1, appeared->nombre.nombre);
	}
	bool _tieneMismoIDCatch(void* entrenador){
		return tienemismoIdCatch(entrenador, caught->correlation_id);
	}

	switch (cod_op) {
	case APPEARED_POKEMON:
		appeared = deserializar_position_and_name(buffer);
		send(cliente_fd,&(appeared->id),sizeof(uint32_t),0);
		if(list_any_satisfy(objetivoGlobal, (void*)_mismaEspecie)){
			puts("llega appeared_pokemon");
			pthread_mutex_lock(&log_mutex);
			log_info(logger, "Mensaje %d Appeared_pokemon %s %d %d", appeared->id, appeared->nombre.nombre, appeared->coordenadas.pos_x, appeared->coordenadas.pos_y);
			pthread_mutex_unlock(&log_mutex);
			pthread_mutex_lock(&mutex_cola_appeared_pokemon);
			queue_push(colaAppearedPokemon,appeared);
			pthread_mutex_unlock(&mutex_cola_appeared_pokemon);
			sem_post(&semAppeared);
		}else{
			free(appeared->nombre.nombre);
			free(appeared);
		}
		break;

	case CAUGHT_POKEMON:
		caught = deserializar_caught_pokemon(buffer);
		send(cliente_fd,&(caught->id),sizeof(uint32_t),0);
		if(list_any_satisfy(entrenadores, (void*)_tieneMismoIDCatch)){
			pthread_mutex_lock(&log_mutex);
			log_info(logger, "Mensaje Caught_pokemon %d %d", caught->caught, caught->correlation_id);
			pthread_mutex_unlock(&log_mutex);
			//caught_pokemon();
			pthread_mutex_lock(&mutex_cola_caught_pokemon);
			queue_push(colaCaughtPokemon,caught);
			pthread_mutex_unlock(&mutex_cola_caught_pokemon);
			sem_post(&semCaught);
		}else{
			free(caught);
		}

		break;

	case LOCALIZED_POKEMON:

		puts("recibe localized_pokemon");
		send(cliente_fd,&(localized->id),sizeof(uint32_t),0);
		localized = deserializar_localized_pokemon(buffer);
		bool mismoIdMensaje(int* ID){
			return  (localized->correlation_id == *ID);
		}
		if(list_any_satisfy(especiesNecesarias, (void*)_mismaEspecie) && list_any_satisfy(IDs_get_pokemon, (void*)mismoIdMensaje)){
			char* mensaje = string_new();
			string_append_with_format(&mensaje, "Mensaje %d localized_pokemon %s %d", localized->id, localized->nombre.nombre, localized->cantidad);
			coordenadas_pokemon* coord;
			for(int i = 0; i<localized->cantidad; i++){
				coord = list_get(localized->listaCoordenadas, i);
				string_append_with_format(&mensaje, " %d %d", coord->pos_x, coord->pos_y);
			}
			string_append_with_format(&mensaje, " correlation id: %d", localized->correlation_id);
			pthread_mutex_lock(&log_mutex);
			log_info(logger, mensaje);
			pthread_mutex_unlock(&log_mutex);
			pthread_mutex_lock(&mutex_cola_localized_pokemon);
			queue_push(colaLocalizedPokemon,localized);
			pthread_mutex_unlock(&mutex_cola_localized_pokemon);
			sem_post(&semLocalized);
		}
		else{
			list_destroy_and_destroy_elements(localized->listaCoordenadas, (void*)destruirElemento);
			free(localized->nombre.nombre);
			free(localized);
		}
		puts("deserializo");

		break;
	case 0:
		pthread_exit(NULL);
	case -1:
		pthread_exit(NULL);
	}
	free(buffer);

}

void esperar_cliente(int servidor){ //funciona
	pthread_t thread;
	struct sockaddr_in direccion_cliente;

	unsigned int tam_direccion = sizeof(struct sockaddr_in);

	int cliente = accept (servidor, (void*) &direccion_cliente, &tam_direccion);

	pthread_create(&thread,NULL,(void*)serve_client,cliente);
	pthread_detach(thread);
}

void serve_client(int socket) //funciona
{
	int cod_op;
	int _recv = recv(socket, &cod_op, sizeof(int), MSG_WAITALL);
	if(_recv == -1 || _recv == 0)
		cod_op = -1;
	process_request(cod_op, socket);
}





void socketEscucha(char* ip, char* puerto){ //funciona
	int servidor = iniciar_servidor(ip, puerto);
	int i = 1;
	while(1){
		esperar_cliente(servidor);
		i++;
	}
}

void deteccionDeadlock(){ //funciona
	//	sem_wait(&sem_deteccionDeadlock); //probar
	sem_wait(&sem_deteccionDeadlock);
	pthread_mutex_lock(&log_mutex);
	log_info(logger,"Se ha iniciado el algoritmo de deteccion de deadlock");
	pthread_mutex_unlock(&log_mutex);
	int cantDeadlocks = 0;
	bool _puedeEstarEnDeadlock(void* entrenador){
		return puedeEstarEnDeadlock(entrenador);
	}
	t_list* entrenadoresDeadlock; // = list_create();
	//	pthread_mutex_lock(&mutex_lista_entrenadores);
	entrenadoresDeadlock = list_filter(entrenadores, _puedeEstarEnDeadlock);
	//	pthread_mutex_unlock(&mutex_lista_entrenadores);
	t_entrenador* entrenador;
	t_entrenador* entrenadorAIntercambiar;
	int ID;
	char* pokemon;
	t_intercambio* intercambio;

	bool _tienePokemonNoNecesario(void* entrenador){
		return tienePokemonNoNecesario(entrenador, pokemon);
	}

	bool _necesitaPokemon(void* pokemon){
		return necesitaPokemon(entrenador, pokemon);
	}
	bool _mismoID(t_entrenador* entrenador1){
		return mismoID(entrenador1, ID);
	}
	pthread_mutex_lock(&mutex_deadlock);
	while(list_size(entrenadoresDeadlock) > 0){

		//		puts(string_itoa(list_size(entrenadoresDeadlock)));
		entrenador = entrenadoresDeadlock->head->data;
		while(list_size(entrenador->pokemonsNoNecesarios) > 0 ){
			pokemon = list_find(entrenador->objetivos, _necesitaPokemon); //devuelve el primer pokemon que necesita el entrenador
			entrenadorAIntercambiar = list_find(entrenadoresDeadlock, _tienePokemonNoNecesario); //devuelve entrenador que tenga al pokemon en pokemons no necesarios
			intercambio = malloc(sizeof(t_intercambio));
			intercambio->entrenador = entrenadorAIntercambiar;
			intercambio->pokemonARecibir = pokemon;
			intercambio->pokemonAEntregar = entrenador->pokemonsNoNecesarios->head->data;
			entrenador->intercambio = intercambio;
			printf("%s \n",intercambio->pokemonAEntregar);
			printf("%s \n",intercambio->pokemonARecibir);

			if(necesitaPokemon(intercambio->entrenador, intercambio->pokemonAEntregar)) cantDeadlocks++;
			cambiarEstado(&entrenador, READY, "va a moverse para hacer intercambio");
			pthread_mutex_lock(&mutex_ready);
			list_add(ready, entrenador);
			pthread_mutex_unlock(&mutex_ready);
			puts("Agrega a cola ready para intercambio");
			sem_post(&sem_ejecutar);
			//			pthread_mutex_unlock(&mutex_ejecutar); //ver si funciona
			pthread_mutex_lock(&mutex_deadlock);
			//wait (espera a que termine de moverse el entrenador)
			if(cumpleObjetivoParticular(entrenadorAIntercambiar)){
				puts("cumple objetivo Entrenador a intercambiar");
				cambiarEstado(&entrenadorAIntercambiar, EXIT, "cumplio su objetivo particular");
				ID = entrenadorAIntercambiar->ID;
				list_remove_by_condition(entrenadoresDeadlock, (void*)_mismoID);
			}
			else{
				cambiarEstado(&entrenadorAIntercambiar, BLOCK, "termino de intercambiar pokemon");
			}
			if(cumpleObjetivoParticular(entrenador)) {
				puts("cumple objetivo entrenador");
				cambiarEstado(&entrenador, EXIT, "cumplio su objetivo particular");
				ID = entrenador->ID;
				list_remove_by_condition(entrenadoresDeadlock,(void*)_mismoID);
			}
			else{
				cambiarEstado(&entrenador, BLOCK, "termino de intercambiar pokemon");
			}
//			free(intercambio->pokemonAEntregar);
//			free(intercambio->pokemonARecibir);
			free(intercambio);
			free(entrenador->intercambio);

		}
	}
	puts("termina deteccion de deadlock");
	pthread_mutex_lock(&log_mutex);
	log_info(logger, "Cantidad de deadlocks Detectados: %d Cantidad de deadlocks Resueltos: %d", cantDeadlocks, cantDeadlocks);
	pthread_mutex_unlock(&log_mutex);
	list_destroy(entrenadoresDeadlock);
}

bool tienePokemonNoNecesario(t_entrenador* entrenador, char* pokemon){ //funciona
	bool _mismaEspecie(char* especie){
		return mismaEspecie(especie, pokemon);
	}
	return list_any_satisfy(entrenador->pokemonsNoNecesarios, (void*)_mismaEspecie);
}

bool puedeEstarEnDeadlock(t_entrenador* entrenador){//funciona
	return (entrenador->estado == BLOCK && (entrenador->pokemons->elements_count ==  entrenador->objetivos->elements_count));
}

bool mismoID(t_entrenador* entrenador, int ID){ //funciona
	return entrenador->ID == ID;
}

void connect_appeared(){
	op_code codigo_operacion = SUSCRIPCION;
	int socket_broker = iniciar_cliente_team(config_get_string_value(config, "IP_BROKER"),config_get_string_value(config, "PUERTO_BROKER"));

	int id_proceso = config_get_int_value(config, "ID_PROCESO");

	t_mensaje* mensaje = malloc(sizeof(t_mensaje));

	char* linea_split[2] = {"APPEARED_POKEMON", string_itoa(id_proceso)};
	mensaje -> tipo_mensaje = codigo_operacion;
	mensaje -> parametros = linea_split;


	enviar_mensaje(mensaje, socket_broker);
	//	liberar_vector(mensaje->parametros);
	free(mensaje);
	puts("envia mensaje");

	int size = 0;
	t_position_and_name* appeared;
	bool _mismaEspecie(char* especie1){
		return mismaEspecie(especie1, appeared->nombre.nombre);
	}
	op_code cod_op;
	int _recv;
	while(!(cumpleObjetivoGlobal())){
		_recv = recv(socket_broker, &cod_op, sizeof(op_code), MSG_WAITALL);

		printf("codigo operacion appeared %d", cod_op);
		puts("");

		if(_recv == 0){
			pthread_mutex_lock(&log_mutex);
			log_info(logger,"Se ha perdido la conexion con el proceso Broker");
			pthread_mutex_unlock(&log_mutex);
			liberar_conexion(socket_broker);
			sem_post(&conexiones);
			pthread_exit(NULL);
		}
		if(_recv == -1){
			puts("error");
			pthread_exit(NULL);
		}

		void* buffer = recibir_mensaje(socket_broker,&size);

		if(cod_op == APPEARED_POKEMON){

			puts("recibe mensaje");
			appeared = deserializar_position_and_name(buffer);

			uint32_t id_ack = appeared->id;

			enviar_ack(socket_broker, id_ack, id_proceso);

			if(list_any_satisfy(objetivoGlobal, (void*)_mismaEspecie)){
				pthread_mutex_lock(&log_mutex);
				log_info(logger, "Mensaje Appeared_pokemon %s %d %d", appeared->nombre.nombre, appeared->coordenadas.pos_x, appeared->coordenadas.pos_y);
				pthread_mutex_unlock(&log_mutex);
				//appeared_pokemon(appeared) ; //hacer dentro de un hilo?
				pthread_mutex_lock(&mutex_cola_appeared_pokemon);
				queue_push(colaAppearedPokemon,appeared);
				pthread_mutex_unlock(&mutex_cola_appeared_pokemon);
				sem_post(&semAppeared);
			}else{
				free(appeared->nombre.nombre);
				free(appeared);
			}
			//			puts(appeared->nombre.nombre);
			puts("deserializo");
		}
		free(buffer);
	}

	liberar_conexion(socket_broker); //hacer en finalizar team?
}
void get_pokemon(char*especie, int socket_broker, t_list* IDs){
	int _recv;
	op_code codigo_operacion = GET_POKEMON;
	puts(especie);
	t_mensaje* mensaje = malloc(sizeof(t_mensaje));
	char* linea_split = string_new();
	string_append_with_format(&linea_split, "%s,%s", especie, "0");
	puts(linea_split);
	mensaje -> tipo_mensaje = codigo_operacion;
	mensaje -> parametros = string_split(linea_split, ",");
	puts("antes enviar");
	enviar_mensaje(mensaje, (socket_broker));
	liberar_vector(mensaje->parametros);
	free(linea_split);
	free(mensaje);
//	uint32_t id;
	puts("manda get pokemon");
//	_recv = recv(socket_broker, &id, sizeof(uint32_t), MSG_WAITALL);
//	if(_recv == 0 || _recv == -1) puts("error al recibir id get pokemon");
//	else{
//		list_add(IDs, id);
//		puts(string_itoa(id));
//	}
}

void connect_localized_pokemon(){

	op_code codigo_operacion = SUSCRIPCION;
	int socket_broker = iniciar_cliente_team(config_get_string_value(config, "IP_BROKER"),config_get_string_value(config, "PUERTO_BROKER"));
	int id_proceso = config_get_int_value(config, "ID_PROCESO");

	t_mensaje* mensaje = malloc(sizeof(t_mensaje));

	char* linea_split[2] = {"LOCALIZED_POKEMON", string_itoa(id_proceso)};
	mensaje -> tipo_mensaje = codigo_operacion;
	mensaje -> parametros = linea_split;



	enviar_mensaje(mensaje, socket_broker);
	puts("envia mensaje");
	//	liberar_vector(linea_split);
	//	liberar_vector(mensaje -> parametros);
	free(mensaje);


	void _get_pokemon(void* especie){
		return get_pokemon(especie, socket_broker, IDs_get_pokemon);
	}
	if(list_is_empty(IDs_get_pokemon))	list_iterate(especiesNecesarias, (void*)_get_pokemon); //ver si funciona esa condicion
	int size = 0;
	t_localized_pokemon* localized;
	t_ack* ack;
	bool _mismaEspecie(char* especie1){
		return mismaEspecie(especie1, localized->nombre.nombre);
	}
	bool mismoIdMensaje(int ID){
		return  (localized->correlation_id == ID);
	}

	op_code cod_op;
	int _recv;
	while(!(cumpleObjetivoGlobal())){

		_recv = recv(socket_broker, &cod_op, sizeof(op_code), MSG_WAITALL);
		if(_recv == 0){
			pthread_mutex_lock(&log_mutex);
			log_info(logger,"Se ha perdido la conexion con el proceso Broker");
			pthread_mutex_unlock(&log_mutex);
			liberar_conexion(socket_broker);
			sem_post(&conexiones);
			pthread_exit(NULL);
		}
		if(_recv == -1){
			puts("error");
			pthread_exit(NULL);
		}
		void* buffer = recibir_mensaje(socket_broker,&size);
		printf("codigo operacion localized %d", cod_op);
		puts("");

		if(cod_op == ACK){
			puts("recibe ack");
			ack = deserializar_ack(buffer);
			list_add(IDs_get_pokemon, ack->id_mensaje);

//			puts(string_itoa(ack->id_mensaje));
			free(ack);

		}else if(cod_op == LOCALIZED_POKEMON){

			puts("recibe mensaje");

			localized = deserializar_localized_pokemon(buffer);

			uint32_t id_ack = localized->id;

			enviar_ack(socket_broker, id_ack, id_proceso);

			if(list_any_satisfy(especiesNecesarias, (void*)_mismaEspecie) && list_any_satisfy(IDs_get_pokemon, (void*)mismoIdMensaje)){
				printf("cantidad de ids recibidos %d", IDs_get_pokemon->elements_count);
				if(list_any_satisfy(especiesNecesarias, (void*)_mismaEspecie)) puts("es espcie necesaria");
				if(list_any_satisfy(IDs_get_pokemon, (void*)mismoIdMensaje)) puts("es correlation ID correcto");

				char* mensaje = string_new();
				string_append_with_format(&mensaje, "Mensaje %d localized_pokemon %s %d", localized->id, localized->nombre.nombre, localized->cantidad);
				coordenadas_pokemon* coord;
				for(int i = 0; i<localized->cantidad; i++){
					coord = list_get(localized->listaCoordenadas, i);
					string_append_with_format(&mensaje, " %d %d", coord->pos_x, coord->pos_y);
				}
				string_append_with_format(&mensaje, " correlation id: %d", localized->correlation_id);
				pthread_mutex_lock(&log_mutex);
				log_info(logger, mensaje);
				pthread_mutex_unlock(&log_mutex);
				pthread_mutex_lock(&mutex_cola_localized_pokemon);
				queue_push(colaLocalizedPokemon,localized);
				pthread_mutex_unlock(&mutex_cola_localized_pokemon);
				sem_post(&semLocalized);
				free(mensaje);
			}else{
				list_destroy_and_destroy_elements(localized->listaCoordenadas, (void*)destruirElemento);
				free(localized->nombre.nombre);
				free(localized);
			}
			//			puts(localized->nombre.nombre);
			puts("deserializo");
		}
		free(buffer);
	}

	liberar_conexion(socket_broker); //hacer en finalizar team?
}

void destruirElemento(void* elemento){
		free(elemento);
	}

void connect_caught_pokemon(){
	op_code codigo_operacion = SUSCRIPCION;
	int socket_broker = iniciar_cliente_team(config_get_string_value(config, "IP_BROKER"),config_get_string_value(config, "PUERTO_BROKER"));
	int id_proceso = config_get_int_value(config, "ID_PROCESO");

	t_mensaje* mensaje = malloc(sizeof(t_mensaje));
	char* linea_split[2] = {"CAUGHT_POKEMON", string_itoa(id_proceso)};
	mensaje -> tipo_mensaje = codigo_operacion;
	mensaje -> parametros = linea_split;



	enviar_mensaje(mensaje, socket_broker);
	//	liberar_vector(mensaje -> parametros);
	free(mensaje);
	puts("envia mensaje");

	int size = 0;
	t_caught_pokemon* caught;

	bool _tieneMismoIDCatch(void* entrenador){
		return tienemismoIdCatch(entrenador, caught->correlation_id);
	}
	op_code cod_op;
	int _recv;
	while(!(cumpleObjetivoGlobal())){
		_recv = recv(socket_broker, &cod_op, sizeof(op_code), MSG_WAITALL);

		printf("codigo operacion caught %d", cod_op);
		puts("");
		if(_recv == 0 ){
			pthread_mutex_lock(&log_mutex);
			log_info(logger,"Se ha perdido la conexion con el proceso Broker");
			pthread_mutex_unlock(&log_mutex);
			liberar_conexion(socket_broker);
			sem_post(&conexiones);
			pthread_exit(NULL);
		}if(_recv == -1){
			pthread_mutex_lock(&log_mutex);
			log_info(logger,"Se ha perdido la conexion con el proceso Broker");
			pthread_mutex_unlock(&log_mutex);
			liberar_conexion(socket_broker);
			sem_post(&conexiones);
			pthread_exit(NULL);
		}
		printf("codigo de operacion: %d", cod_op);
		puts("recibe un mensaje adentro de caught pokemon");

		void* buffer = recibir_mensaje(socket_broker,&size);
		if(cod_op == CAUGHT_POKEMON){
			puts("recibe mensaje");
			caught = deserializar_caught_pokemon(buffer);
			uint32_t id_ack = caught->id;

			enviar_ack(socket_broker, id_ack, id_proceso);

			if(list_any_satisfy(entrenadores, (void*)_tieneMismoIDCatch)){
				pthread_mutex_lock(&log_mutex);
				log_info(logger, "Mensaje Caught_pokemon %d %d correlation id: %d", caught->caught, caught->id, caught->correlation_id);
				pthread_mutex_unlock(&log_mutex);
				pthread_mutex_lock(&mutex_cola_caught_pokemon);
				queue_push(colaCaughtPokemon,caught);
				pthread_mutex_unlock(&mutex_cola_caught_pokemon);
				sem_post(&semCaught);
			}else{
				free(caught);
			}
		}
		free(buffer);
	}
}


bool tienemismoIdCatch(t_entrenador* entrenador, uint32_t correlation_id){
	return entrenador->catch_id == correlation_id;
}

void connect_gameboy(){
	socketEscucha(config_get_string_value(config, "IP_TEAM"), config_get_string_value(config, "PUERTO_TEAM"));
}

int iniciar_cliente_team(char* ip, char* puerto){
	struct sockaddr_in direccion_servidor;

	direccion_servidor.sin_family = AF_INET;
	direccion_servidor.sin_addr.s_addr = inet_addr(ip);
	direccion_servidor.sin_port = htons(atoi(puerto));
	int cliente = socket(AF_INET, SOCK_STREAM, 0);

	if(connect(cliente, (void*) &direccion_servidor, sizeof(direccion_servidor)) !=0){
		pthread_mutex_lock(&log_mutex);
		log_info(logger, "No se pudo realizar la conexion");
		pthread_mutex_unlock(&log_mutex);
		liberar_conexion(cliente);
		sem_post(&conexiones);
		pthread_exit(NULL);
	}else{
		pthread_mutex_lock(&log_mutex);
		log_info(logger,"Se ha establecido una conexion con el proceso Broker");
		pthread_mutex_unlock(&log_mutex);
		return cliente;
	}
}


void catch_pokemon(char* ip, char* puerto, t_entrenador** entrenador){ //probar

	struct sockaddr_in direccion_servidor;
	op_code codigo_operacion = CATCH_POKEMON;
	t_mensaje* mensaje = malloc(sizeof(t_mensaje));
	uint32_t ID;
	char* linea_split = string_new();
	string_append_with_format(&linea_split, "%s,%d,%d,%d,%d", (*entrenador)->pokemonACapturar->especie,(*entrenador)->pokemonACapturar->coordx, (*entrenador)->pokemonACapturar->coordy, 0,0);

	mensaje -> tipo_mensaje = codigo_operacion;
	mensaje -> parametros = string_split(linea_split,",");;
	free(linea_split);
	direccion_servidor.sin_family = AF_INET;
	direccion_servidor.sin_addr.s_addr = inet_addr(ip);
	direccion_servidor.sin_port = htons(atoi(puerto));

	pthread_mutex_lock(&log_mutex);
	log_info(logger, "Atrapar %s en la posicion (%d,%d)", (*entrenador)->pokemonACapturar->especie, (*entrenador)->pokemonACapturar->coordx,(*entrenador)->pokemonACapturar->coordy);
	pthread_mutex_unlock(&log_mutex);
	cambiarEstado(entrenador, BLOCK, "esta esperando el resultado de intentar atrapar pokemon");
	puts("va a atrapar pokemon");
	sleep(config_get_int_value(config, "RETARDO_CICLO_CPU"));
	(*entrenador)->CiclosCPU ++;
	(*entrenador)->rafagaReal ++;
	(*entrenador)->restoEstimacion --;
	ciclosCPUGlobal ++;
	bool _mismoID(void* entre){
		mismoID(entre, (*entrenador)->ID);
	}
	pthread_mutex_lock(&mutex_ready);
	list_remove_by_condition(ready, _mismoID); //saca al entrenador de la lista de ready
	pthread_mutex_unlock(&mutex_ready);

	int socket_broker = socket(AF_INET, SOCK_STREAM, 0);
	int _recv;
	if(connect(socket_broker, (void*) &direccion_servidor, sizeof(direccion_servidor)) !=0){
		pthread_mutex_lock(&log_mutex);
		log_info(logger, "Se atrapara al pokemon por default porque no se pudo conectar con el Broker");
		pthread_mutex_unlock(&log_mutex);
		//		capturoPokemon(entrenador);
		liberar_conexion(socket_broker);
		(*entrenador)->respuesta_catch = true;
		liberar_vector(mensaje->parametros);
		free(mensaje);
		pthread_mutex_unlock(&((*entrenador)->mutex));
		return;
	}else{
		enviar_mensaje(mensaje, socket_broker);
		_recv = recv(socket_broker, &ID, sizeof(uint32_t), MSG_WAITALL);
		if(_recv == 0){
			pthread_mutex_lock(&log_mutex);
			log_info(logger, "Se atrapara al pokemon por default porque no se pudo conectar con el Broker");
			pthread_mutex_unlock(&log_mutex);
			liberar_conexion(socket_broker);
			(*entrenador)->respuesta_catch = true;
			liberar_vector(mensaje->parametros);
			free(mensaje);
			//			capturoPokemon(entrenador);
			pthread_mutex_unlock(&((*entrenador)->mutex));
			return;
		}
		if(_recv == 1){
			puts("error agregar lo que tenga que hacer aca");
			liberar_vector(mensaje->parametros);
			free(mensaje);
			return;
		}
		(*entrenador)->catch_id = ID;
		printf("Id del catch pokemon : %d o %d", (*entrenador)->catch_id, ID);
		liberar_conexion(socket_broker);
	}
	liberar_vector(mensaje->parametros);
	free(mensaje);
	//	pthread_mutex_lock(&(*entrenador)->mutex);
}

void crearConexiones(){
	int tiempoReconexion = config_get_int_value(config, "TIEMPO_RECONEXION");
	pthread_t appeared_pokemon_thread;
	pthread_t localized_pokemon_thread;
	pthread_t caught_pokemon_thread;
	while(!entrenadoresTienenElInventarioLleno()){
		pthread_create(&appeared_pokemon_thread,NULL,(void*)connect_appeared,NULL);
		pthread_detach(appeared_pokemon_thread);
		pthread_create(&localized_pokemon_thread,NULL,(void*)connect_localized_pokemon,NULL);
		pthread_detach(localized_pokemon_thread);
		pthread_create(&caught_pokemon_thread,NULL,(void*)connect_caught_pokemon,NULL);
		pthread_detach(caught_pokemon_thread);
		sem_wait(&conexiones);
		sem_wait(&conexiones);
		sem_wait(&conexiones);
		sleep(tiempoReconexion);
		pthread_mutex_lock(&log_mutex);
		log_info(logger, "Inicio Reintento de todas las conexiones");
		pthread_mutex_unlock(&log_mutex);
	}
	sem_destroy(&conexiones);
	return;
}

bool entrenadoresTienenElInventarioLleno(){
	bool resultado;
	bool _tieneInventarioLlenoOEstaEnExit(void* entrenador){
		return tieneInventarioLlenoOEstaEnExit(entrenador);
	}
	//	pthread_mutex_lock(&mutex_lista_entrenadores);
	resultado = list_all_satisfy(entrenadores, (void*)_tieneInventarioLlenoOEstaEnExit);
	//	pthread_mutex_unlock(&mutex_lista_entrenadores);
	return resultado;
}


bool tieneInventarioLlenoOEstaEnExit(t_entrenador* entrenador){
	return (puedeEstarEnDeadlock(entrenador) || entrenador->estado == EXIT);
}
void appeared_pokemon(){
	while(!(cumpleObjetivoGlobal())){
		sem_wait(&semAppeared);
		puts("entra appeared_pokemon");
		pthread_mutex_lock(&mutex_cola_appeared_pokemon);
		t_position_and_name* appeared = queue_pop(colaAppearedPokemon);
		pthread_mutex_unlock(&mutex_cola_appeared_pokemon);
		t_pokemon* pokemonNuevo = malloc(sizeof(t_pokemon));
		pokemonNuevo->coordx = appeared->coordenadas.pos_x;
		pokemonNuevo->coordy = appeared->coordenadas.pos_y;
		pokemonNuevo->especie = malloc(appeared->nombre.largo_nombre+1);
		strcpy(pokemonNuevo->especie, appeared->nombre.nombre);
		pokemonNuevo->planificado = false;
		free(appeared->nombre.nombre);
		free(appeared);
		bool _mismaEspecie(char* especie){
			return mismaEspecie(especie, pokemonNuevo->especie);
		}
		bool _mismaPokemon(t_pokemon* pokemon){
			return mismaEspecie(pokemon->especie, pokemonNuevo->especie);
		}

		if(list_any_satisfy(especiesNecesarias, (void*)_mismaEspecie))
			list_remove_by_condition(especiesNecesarias, (void*)_mismaEspecie); //para marcar que ya nos llego un appeared de esta especie

		pthread_mutex_lock(&objetivo);
		int necesarios = list_count_satisfying(objetivoGlobal, (void*)_mismaEspecie);
		pthread_mutex_unlock(&objetivo);
		pthread_mutex_lock(&requeridos);
		int pokemonsACapturar = list_count_satisfying(pokemonsRequeridos, (void*)_mismaPokemon);
		pthread_mutex_unlock(&requeridos);
		if(necesarios>pokemonsACapturar){
			pthread_mutex_lock(&requeridos);
			list_add(pokemonsRequeridos, pokemonNuevo);
			pthread_mutex_unlock(&requeridos);
			//			pthread_mutex_unlock(&sem_ready);
			sem_post(&sem_ready);
			puts("--------------appeared pokemon-------------");
		}else{

			if(necesarios>0){
				pthread_mutex_lock(&repuesto);
				list_add(pokemonsDeRepuesto, &pokemonNuevo);
				pthread_mutex_unlock(&repuesto);
			}

		}
	}
}

void localized_pokemon(){
	t_localized_pokemon* localized;
	t_position_and_name* appeared;
	void _llenarAppearedPokemon(void* coordenada){
		return llenarAppearedPokemon(coordenada, localized, appeared);
	}

	while(!(cumpleObjetivoGlobal())){
		sem_wait(&semLocalized);
		puts("llego localized");
		pthread_mutex_lock(&mutex_cola_localized_pokemon);
		localized = queue_pop(colaLocalizedPokemon);
		pthread_mutex_unlock(&mutex_cola_localized_pokemon);
		list_iterate(localized->listaCoordenadas, (void*)_llenarAppearedPokemon);
		list_destroy_and_destroy_elements(localized->listaCoordenadas, (void*)destruirElemento);
		free(localized->nombre.nombre);
		free(localized);
	}

}
void llenarAppearedPokemon(coordenadas_pokemon* coord ,t_localized_pokemon* localized, t_position_and_name* appeared){
	if(localized->cantidad>0){
	appeared = malloc(sizeof(t_position_and_name));
	appeared->coordenadas.pos_x = coord->pos_x;
	appeared->coordenadas.pos_y = coord->pos_y;
	appeared->nombre.nombre = malloc(localized->nombre.largo_nombre +1);
	strcpy(appeared->nombre.nombre, localized->nombre.nombre);
	appeared->nombre.largo_nombre = localized->nombre.largo_nombre;
	pthread_mutex_lock(&mutex_cola_appeared_pokemon);
	queue_push(colaAppearedPokemon,appeared);
	pthread_mutex_unlock(&mutex_cola_appeared_pokemon);
	sem_post(&semAppeared);
	}
}

void caught_pokemon(){
	t_entrenador* entrenador;
	t_caught_pokemon* caught;
	bool _tieneMismoIDCatch(void* entrenador){
		return tienemismoIdCatch(entrenador, caught->correlation_id);
	}
	while(!(cumpleObjetivoGlobal())){
		sem_wait(&semCaught);
		pthread_mutex_lock(&mutex_cola_caught_pokemon);
		caught = queue_pop(colaCaughtPokemon);
		pthread_mutex_unlock(&mutex_cola_caught_pokemon);
		//		pthread_mutex_lock(&mutex_lista_entrenadores);
		entrenador = list_find(entrenadores, (void*)_tieneMismoIDCatch);
		//		pthread_mutex_unlock(&mutex_lista_entrenadores);
		entrenador->catch_id = 0;
		if(caught->caught == 1){
			//			entrenador = list_find(entrenadores, (void*)_tieneMismoIDCatch);
			//			capturoPokemon(&entrenador);
			entrenador->respuesta_catch =true;
		}
		else{
			//			noCapturoPokemon(&entrenador);
			entrenador->respuesta_catch =false;
		}
		//		printf("pokemons requeridos: %d",pokemonsRequeridos->elements_count);
		pthread_mutex_unlock(&(entrenador->mutex));
		free(caught);
	}
}
void capturoPokemon(t_entrenador** entrenador){ // ejecuta luego de que capturo un pokemon
	//	float aux;
	puts("capturo pokemon");
	char* especiePokemon = malloc(strlen((*entrenador)->pokemonACapturar->especie)+1);
	strcpy(especiePokemon,(*entrenador)->pokemonACapturar->especie );
	if(!necesitaPokemon(*(entrenador), especiePokemon))
		list_add((*entrenador)->pokemonsNoNecesarios, especiePokemon);

	list_add((*entrenador)->pokemons, especiePokemon);
	printf("Se agrego al pokemon %s a la lista", (*entrenador)->pokemonACapturar->especie);

	pthread_mutex_lock(&objetivo);
	removerPokemon((*entrenador)->pokemonACapturar->especie,objetivoGlobal);
	pthread_mutex_unlock(&objetivo);
	if(tieneMenosElementos ((*entrenador)->pokemons, (*entrenador)->objetivos)){
		cambiarEstado(entrenador, BLOCK, "capturo un pokemon");
	}else{
		if(cumpleObjetivoParticular(*entrenador)){
			cambiarEstado(entrenador, EXIT, "cumplio su objetivo particular");
		}
		else{
			cambiarEstado(entrenador, BLOCK, "capturo un pokemon y no puede capturar mas pokemons");
		}
	}
	bool _mismoPokemon(t_pokemon* pokemon ){
		return mismoPokemon(pokemon,(*entrenador)->pokemonACapturar);
	}
//		void destruirPokemon(t_pokemon* pokemonADestruir){
//			free(pokemonADestruir->especie);
////			free(pokemonADestruir);
//		}

	pthread_mutex_lock(&requeridos);
//	list_remove_and_destroy_by_condition(pokemonsRequeridos, (void*)_mismoPokemon ,(void*) destruirPokemon);
	list_remove_by_condition(pokemonsRequeridos, (void*)_mismoPokemon);
	pthread_mutex_unlock(&requeridos);
	free((*entrenador)->pokemonACapturar->especie);
	free((*entrenador)->pokemonACapturar);
	(*entrenador)->pokemonACapturar = NULL;

	if((*entrenador)->objetivos->elements_count > (*entrenador)->pokemons->elements_count){
		(*entrenador)->estimacion = ((algoritmoPlanificacion.alpha) * ((*entrenador)->rafagaReal) + (1- (algoritmoPlanificacion.alpha))* ((*entrenador)->estimacion));
		//		aux = (1- (algoritmoPlanificacion->alpha))* ((*entrenador)->rafagaReal);
		//		(*entrenador)->estimacion = aux;
		(*entrenador)->restoEstimacion = (*entrenador)->estimacion;
		(*entrenador)->rafagaReal = 0;
		sem_post(&entrenadoresPlanificados);
	}
	if(entrenadoresTienenElInventarioLleno()){ //probar
		puts("entrenadores tiene inventario lleeno");
		sem_post(&sem_deteccionDeadlock);
	}

}

void noCapturoPokemon(t_entrenador** entrenador){//Probar
	puts("no capturo pokemon");
	t_pokemon* pokemonNuevo;
	bool _mismoPokemon(t_pokemon* pokemon ){
		return mismoPokemon(pokemon,(*entrenador)->pokemonACapturar);
	}
	pthread_mutex_lock(&requeridos);
	list_remove_by_condition(pokemonsRequeridos, (void*)_mismoPokemon);
	pthread_mutex_unlock(&requeridos);

	bool _mismaEspecie(t_pokemon* pokemon2){
		return mismaEspecie(pokemon2->especie, (*entrenador)->pokemonACapturar->especie);
	}
	if(list_any_satisfy(pokemonsDeRepuesto,(void*)_mismaEspecie)) {
		pthread_mutex_lock(&repuesto);
		pokemonNuevo = list_remove_by_condition(pokemonsDeRepuesto, (void*)_mismaEspecie);
		pthread_mutex_unlock(&repuesto);

		pthread_mutex_lock(&requeridos);
		list_add(pokemonsRequeridos, pokemonNuevo);
		pthread_mutex_unlock(&requeridos);
	}
	free((*entrenador)->pokemonACapturar->especie);
	free((*entrenador)->pokemonACapturar);
	(*entrenador)->pokemonACapturar = NULL;

	(*entrenador)->estimacion = ((algoritmoPlanificacion.alpha) * ((*entrenador)->rafagaReal) + (1- (algoritmoPlanificacion.alpha))* ((*entrenador)->estimacion));
	(*entrenador)->restoEstimacion = (*entrenador)->estimacion;
	(*entrenador)->rafagaReal = 0;
	cambiarEstado(entrenador, BLOCK, "no capturo a un pokemon");
	sem_post(&entrenadoresPlanificados);
}




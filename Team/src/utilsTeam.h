#ifndef UTILSTEAM_H_
#define UTILSTEAM_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<semaphore.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<commons/collections/node.h>
#include<commons/collections/list.h>
#include<commons/string.h>
#include<stdbool.h>
#include<../CommonsMCLDG/utils.h>
#include<../CommonsMCLDG/socket.h>
//#include<../CommonsMCLDG/serializacion.h>
#include <commons/collections/queue.h>
#include<pthread.h>
#include<math.h>



#define PATH "/home/utnso/workspace/tp-2020-1c-MCLDG/Team/TEAM.config"




typedef enum
{
	NEW = 1,
	READY = 2,
	EXEC = 3,
	BLOCK = 4,
	EXIT = 5,
}t_estado;

typedef enum{
	FIFO = 1,
	RR = 2,
	SJFCD =3,
	SJFSD = 4,
}tipo_algoritmo;

typedef struct t_algoritmo_planificacion{
	tipo_algoritmo tipoAlgoritmo;
	int quantum;
	float alpha;
	float estimacion_inicial;
}t_algoritmo_planificacion;


typedef struct t_pokemon
{
	char* especie;
	int coordx; // coordenada x
	int coordy;//coordenada y
	bool planificado;
} t_pokemon;


typedef struct t_entrenador
{
	int ID;
	int coordx; // coordenada x
	int coordy; //coordenada y
	t_list* pokemons; // lista string
	t_list* objetivos; // lista string
	t_estado estado; //para saber si esta en ready o block
	t_pokemon* pokemonACapturar;
	void* intercambio; //siempre castear como t_intercambio
	t_list* pokemonsNoNecesarios; // lista string
	int CiclosCPU;
	pthread_mutex_t mutex;
	uint32_t catch_id;
	bool respuesta_catch;
	int quantum_usado;
	float estimacion;
	float restoEstimacion;
	int rafagaReal;
} t_entrenador;

typedef struct t_intercambio{
	t_entrenador* entrenador; //saber como es que hay que destruir el struct con punteros
	char* pokemonARecibir;
	char* pokemonAEntregar;
}t_intercambio;




void iniciarTeam();
//Libera memoria
void terminarTeam(int conexion);
//Lee el config y pasa los datos de entrenadores a configurarEntrenadores();
void configurarEntrenadores();
//Crea un entreandor
t_entrenador* crearEntrenador(char* posiciones, char* pokemonEntrenadores, char* objetivos, int ID);
//Llena una lista de pokemons que recibe en un vector de strings
t_list* configurarPokemons(char** pokemons);
//Cambia de estado a los entrenadores
void cambiarEstado (t_entrenador** entrenador,t_estado nuevoEstado, char* razonDeCambio);
//Verifica si el cambio de estado es valido
bool cambioEstadoValido(t_estado estadoViejo,t_estado nuevoEstado);
//Verifica si se cumplio el Objetivo global (todos los entrenadores estan en exit)
bool cumpleObjetivoGlobal();
//Verifica si un entrenador esta en estado exit
bool esEstadoExit(t_entrenador* entrenador);
//Verifica si dos listas son iguales
bool listasIguales(t_list* lista1, t_list* lista2);
//Para ordenar alfabeticamente
bool criterioOrden(char* elem1, char* elem2);
//Verifica si los objetivos son iguales a los pokemons de un entrenador
bool cumpleObjetivoParticular (t_entrenador* entrenador);
//libera la memoria de un entrenador
void entrenadorDestroy(t_entrenador * entrenador);
//Verifica si una lista
bool tieneMenosElementos (t_list* listaChica, t_list* lista );
//Verifica si tien espacio en el inventario para atrapar un pokemon un pokemon y su estado
bool puedeAtraparPokemon(t_entrenador* entrenador);
//Se ejecuta cuando el entrenador capturo al pokemon (cuando recibe caught 1 o reaccion por default)
void capturoPokemon(t_entrenador** entrenador);
//Se ejecuta cuando el entrenador no capturo al pokemon
void noCapturoPokemon(t_entrenador** entrenador);
//Agrega los pokemon que necesita cada entrenador
void configurarObjetivoGlobal();
//remueve un pokemon de una lista de char* segun especie
char* removerPokemon(char* pokemon, t_list* lista);
//Funcion del hilo de cada entrenador (se mueve, manda catch, intercambio)
void* entrenadorMaster(void* entrenador);
//calcula la distancia entre dos coordenadas
int distancia(int x1, int y1, int x2, int y2);
//mueve a un entrenador hacia una coordenada de a una posicion
void moverEntrenador(t_entrenador** entrenador, int x, int y);
//intercambia pokemons
void intercambiarPokemon(t_entrenador** entrenador);
//
//void atraparPokemon(t_entrenador* entrenador);
//planifica a los entrenadores

void planificar();

//replanifica segun algoritmo de planificacion
void replanificar();
//Devuelve true si el primer entrenador esta mas cerca del pokemon que el segundo
bool menorDistancia(t_entrenador* elem1, t_entrenador* elem2, t_pokemon* pokemon);
//Planifica al entrenador mas cercano al pokemon y lo agrega a la cola ready
void llenarColaReady();
//saca de a un entrenador de la cola de ready y lo pone en EXEC
void ejecutaEntrenadores();
//Verifica si el estado de un entrenado es NEW o BLOCK
bool estadoNewOBlock(t_entrenador* entrenador);
//Ejecuta la reaccion a appeared_pokemon
void appeared_pokemon();

//conexion Gameboy
void connect_gameboy();
void esperar_cliente(int servidor);
void serve_client(int socket);
void socketEscucha(char* ip, char* puerto);
void process_request(int cod_op, int cliente_fd);

//Verifica si un pokemon no ha sido planificado
bool noEstaPlanificado(t_pokemon* pokemon);
//Verifica si dos pokemons son el mismo
bool mismoPokemon(t_pokemon* pokemon,t_pokemon* pokemon2);
//Verifica si un entrenador necesita una especie de pokemon
bool necesitaPokemon(t_entrenador* entrenador, char* especie);
//Verifica si dos string son iguales
bool mismaEspecie(char* especie,char* especie2);
//Verifica si un entrenador puede estar en deadlock (estado BLOCK y inventario lleno)
bool puedeEstarEnDeadlock(t_entrenador* entrenador);
//funcion hilo de deteccion de deadlock
void deteccionDeadlock();
//Verifica si un entrenador tiene un pokemon que no necesita
bool tienePokemonNoNecesario(t_entrenador* entrenador, char* pokemon);
//Elimina los pokemons que estan en el objetivo de la lista de pokemons No Necesarios
void eliminarPokemonsObjetivoParticular(char* pokemon, t_list* pokemonsNoNecesarios);
//Verifica si un entrenador tiene un ID
bool mismoID(t_entrenador* entrenador, int ID);
//Se usa para llenar la lista de especies que necesita el proceso globalmente
//(para mandar el get_pokemon y verificar si recibio un localized_pokemon o appeared_pokemon antes)
void agregarEspecie(char* pokemon, t_list* especiesNecesarias);
//Se suscribe a la cola appeared_pokemon y recibe los mensajes
void connect_appeared();
//Se suscribe a la cola localized_pokemon, manda get_pokemon y recibe los mensajes
void connect_localized_pokemon();
//Se suscribe a la cola caught_pokemon y recibe los mensajes
void connect_caught_pokemon();
//crea el socket para las conexiones con el broker
int iniciar_cliente_team(char* ip, char* puerto);
//Manda mensaje catch pokemon (crea socket, manda mensaje, recibe id ,cierra la conexion y bloquea al entrenador)
void catch_pokemon(char* ip, char* puerto, t_entrenador** entrenador);
//Menda un mensaje de get_pokemon de la especie que recibe por parametro y guarda el id en la lista IDs
void get_pokemon(char*especie, int socket_broker, t_list* IDs);
//Verifica si un entrenador tiene el Inventario LLeno o esta en exit
bool tieneInventarioLlenoOEstaEnExit(t_entrenador* entrenador);
//Verifica si todos los entrenadores tienen el
bool entrenadoresTienenElInventarioLleno();
//Crea los hilos de la conexiones y reintenta las conexiones
void crearConexiones();
//Verifica si un entrenador mando un mensaje catch con el ID que recibe por parametro
bool tienemismoIdCatch(t_entrenador* entrenador, uint32_t correlation_id);
//Reaccion al mensaje caught pokemon
void caught_pokemon();

void llenarAppearedPokemon(coordenadas_pokemon* coord,t_localized_pokemon* localized, t_position_and_name* appeared);
//Reaccion al mensaje localized pokemon
void localized_pokemon();

bool tieneEspacioYEstaEnExec(t_entrenador* entrenador);

bool puedeSerPlanificado(t_entrenador* entrenador);

char* stringEstado(t_estado estado);

void destruirElemento(void* elemento);

#endif /* UTILSTEAM_H_ */

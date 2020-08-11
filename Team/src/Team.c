/*
 ============================================================================
 Name        : Team.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "Team.h"

int main(void) {

	iniciarTeam();




	//Intento de envio de mensaje

//		t_mensaje* mensaje = malloc(sizeof(t_mensaje));
//		mensaje -> tipo_mensaje = APPEARED_POKEMON;
//		char* hola = "gastooooon 1 2 3";
//		char** hola2 = string_split(hola," ");
//
//		mensaje -> parametros = hola2;
//		mensaje -> id = 2;
//		int socket_broker = iniciar_cliente("127.0.0.1","6009");//IP y Puerto del broker
//		enviar_mensaje(mensaje, socket_broker);
	//

	/*Prueba intercambiar
	t_entrenador* entrenador1 = entrenadores->head->data;
	t_entrenador* entrenador2 = entrenadores->head->next->data;


	printf("Prueba: %s \n", entrenador1->pokemons->head->data);
	printf("Prueba: %s \n", entrenador1->pokemons->head->next->data);


	t_intercambio* inter = malloc(sizeof(t_intercambio));
	inter->pokemonAEntregar = "Pikachu";
	inter->pokemonARecibir = "Charmander";
	inter->entrenador = entrenador2;

	entrenador2 = inter->entrenador;
	printf("%d", entrenador2->coordx);
	printf("%d", entrenador2->coordy);
	puts("llego");

	entrenador1->intercambio = inter;

	inter = entrenador1->intercambio;
	printf("%s",inter->pokemonAEntregar);
	printf("%s",inter->pokemonARecibir);
	intercambiarPokemon(&entrenador1);
	printf("%s \n", entrenador1->pokemons->head->data);
	printf("%s \n", entrenador1->pokemons->head->next->data);
	printf("%s \n", entrenador1->pokemons->head->next->next->data);
	printf("%s \n", entrenador2->pokemons->head->data);
	printf("%s \n", entrenador2->pokemons->head->next->data);

*************************FIN Prueba Intercambiar***********************
*/


/*Prueba capturo pokemon

		t_entrenador* entrenador = entrenadores->head->data;
		entrenador->pokemonACapturar = malloc(sizeof(t_pokemon));
		entrenador->pokemonACapturar->especie = "Pidgey";
		capturoPokemon(&(entrenador));
		printf("%s", entrenador->pokemonsNoNecesarios->head->data);


		*************************FIN Prueba capturo pokemon***********************/

//Prueba necesita pokemon

//	t_entrenador* entrenador = entrenadores->head->next->data;
//
//	if(list_is_empty(entrenador->pokemonsNoNecesarios)) puts("esta vacia\n");
//	else{
//	printf("%d \n", entrenador->pokemonsNoNecesarios->elements_count);
//	printf("%s \n",entrenador->pokemonsNoNecesarios->head->data);
//	}
	//	if(tienePokemonNoNecesario(entrenador, "Squirtle")) puts("No necesita squirtle");
//	if(puedeEstarEnDeadlock(entrenador)) puts("puede estar en deadlock");


//*************************FIN Prueba necesita pokemon***********************/



//	deteccionDeadlock();
//	if(puedeEstarEnDeadlock(entrenadores->head->data)) puts("puede estar en deadlock");
//	if(cumpleObjetivoGlobal()) puts("cumple objetivo global");



	//planificar();

	terminarTeam(1);
	puts("termina");

	exit(0);
	return EXIT_SUCCESS;
}

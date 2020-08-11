/*
 * Broker.h
 *
 *  Created on: 18 abr. 2020
 *      Author: MCLDG
 */

#ifndef BROKER_H_
#define BROKER_H_

#include "utils_broker.h"

#define PATH "/home/utnso/workspace/tp-2020-1c-MCLDG/Broker/BROKER.config"

void iniciar_broker();
void terminar_broker(t_log*, t_config*);




void socket_mensajes();

#endif /* BROKER_H_ */

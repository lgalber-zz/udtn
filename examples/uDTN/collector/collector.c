/*
 * NO ME ROBBEN!
 *
 * La idea del ejemplo es que se trata de un nodo sensor de una red. Cada cierto tiempo se genera un dato que se debe almacenar.
 * Cuando se recibe una señal determinada, se deben empezar a enviar los datos hacia el nodo coordinador.
 * Se va a almacenar en dos arreglos la hora del dato y el valor del mismo. Asi en el lugar 0 del primer arreglo vas a tener una
 *  hora y en el 0 del segundo arreglo un dato. Si ambos bundles tienen tamaño de 16, vamos a tener que para el bundle de horas,
 *  16*4=64 (son uint32_t) y mas los 16 de datos (uint8_t) tenemos 80 (Recordemos que el máximo de bytes que se puede almacenar
 *  en un bundle es 80).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contiki.h"
#include "process.h"

#include "net/uDTN/bundle.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/api.h"

/* Definicion del Proceso */
PROCESS(collector_process, "Collector process");
AUTOSTART_PROCESSES(&collector_process);

/* Definicion de variables */
#define COORDINADOR			1
#define SERVICIO_DESTINO	5
#define LONGITUD			16
#define NODO_DESTINO		0
#define LIFETIME			3600

/*
 * Función para guardar los bundles con el dato adentro.
 * Devuelve 1 en caso exitoso y 0 en caso de fallo.
 */
int guardar_dato(uint32_t nodo_destino, uint32_t servicio_destino,uint32_t * payload1,uint8_t * payload2)
{
	// Vamos a almacenar los datos obtenidos en un bundle.
	struct mmem * paquete_de_datos = NULL;
	uint32_t tmp;
	int n,m;

	// Generamos el bundle, y verificamos que lo hayamos podido crear
	paquete_de_datos= bundle_create_bundle();

	if( paquete_de_datos == NULL ) {
			//printf("No hay memoria para almacenar el bundle\n");
			return -1;
		}
	// Seteamos atributos
	bundle_set_attr(paquete_de_datos, DEST_NODE, &nodo_destino);
	bundle_set_attr(paquete_de_datos, DEST_SERV, &servicio_destino);
	// Tomamos el endpoint como un singleton endpoint
	tmp = BUNDLE_FLAG_SINGLETON;
	bundle_set_attr(paquete_de_datos, FLAGS, &tmp);
	// El tiempo de vida fijado, por defecto es de una hora
	tmp = LIFETIME;
	bundle_set_attr(paquete_de_datos, LIFE_TIME, &tmp);

	// Se agregan los 2 bloques payload
	n = bundle_add_block(paquete_de_datos, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, payload1, LONGITUD);
	m = bundle_add_block(paquete_de_datos, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, payload2, LONGITUD);
	if( n == -1  || m == -1) {
		//printf("Memoria insuficiente para agregar el bloque payload\n");
		// En caso de que no haya habido suficiente memoria
			bundle_decrement(paquete_de_datos);
			return -1;
		}


	return 1;
}

/* Especificacion del hilo */
PROCESS_THREAD(collector_process, ev, data)
{
	static struct registration_api registration;
	uint8_t nodo_destino;
	uint16_t i=0;
	udtn_timeval_t time;
	uint8_t crear_bundles=1;

	PROCESS_BEGIN();

	// Esperar a la inicialización del agente.
	PROCESS_PAUSE();

	// Lo primero que hay que hacer es registrarnos.
	/* Register our endpoint to send and receive bundles */
	registration.status = APP_ACTIVE;
	registration.application_process = PROCESS_CURRENT();
	registration.app_id = 5;
	process_post(&agent_process, dtn_application_registration_event, &registration);

	/* Seteamos un timer cada cuanto vamos a guardar un dato.*/
	etimer_set(&timer, CLOCK_SECOND * 5);

	/* Creamos los arreglos que vamos a llenar con informacion.*/
	uint32_t payload1[LONGITUD];
	uint8_t payload2[LONGITUD];
	memset(payload1, 0, LONGITUD);
	memset(payload2, 0, LONGITUD);

	while(1)
	{

	 PROCESS_WAIT_EVENT();

	 /* Cuando expira el timer guardamos un dato y su timesdtamp. */
	 if( etimer_expired(&timer))
	 {
		 udtn_gettimeofday(&time);
		 payload1[i]=time.tv_sec;
		 payload2[i]=i;
		 etimer_reset(&timer);
	 }

	 /* Se guardo el bundle */
	 if( ev == dtn_bundle_stored )
	 {
	 crear_bundles = 1;
	 etimer_restart(&timer);
	 }

	 /* No se guardo el bundle */
	 if( ev == dtn_bundle_store_failed )
	 {
	 crear_bundles = 0;
	 etimer_restart(&timer);
	 }

	 /* LLego el recolector! */
	 if( ev == dtn_collector_arrived )
	 {
	  crear_bundles = 0;

	 }

	 /* Si se llena el bundle, lo guardamos. */
	/* if(i == 15)
	 {
		 if(guardar_dato(NODO_DESTINO,SERVICIO_DESTINO,payload1,payload2))
		 {
		  printf("Se guardo el bundle.");
		 }
		 else
		 {
		  printf("No se pudo guardar el bundle.\n");
		 }
	 }*/

	}


	PROCESS_END();
}


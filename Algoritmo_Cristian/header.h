#ifndef HEADER_CRISTIAN_H
#define HEADER_CRISTIAN_H

#define PUERTO     3000
#define DIR_SERV   "127.0.0.1"
#define N_CLIENTES 10

#include <sys/time.h> 
#include <netinet/in.h>
#include <time.h>

// ----- Estructura para almacenar el tiempo físico y visual -----
typedef struct {
    int horas;
    int minutos;
    int segundos;
    int milisegundos;
} tiempo;

// ----- Estructura del mensaje de Cristian -----------
typedef struct {
    int  id_proceso;            
    char peticion[200];         
    tiempo tiempo_envio;      // T0: La estampa de tiempo al momento de enviar
    tiempo tiempo_servidor;     // Ts: La estampa de tiempo oficial devuelta
    tiempo tiempo_respuesta;      // T1: La estampa de tiempo al momento de responder
} Mensaje_C;

#endif
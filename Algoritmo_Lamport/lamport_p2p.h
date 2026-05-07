#ifndef LAMPORT_P2P_H
#define LAMPORT_P2P_H

#define NUM_NODOS 3     // Numero de computadoras a crear
#define NUM_EVENT0 7    // Numero de eventos totales
#define CMD_EX "EXIT"   // Comando de apagado
#define CMD_EN "ENVIAR" // Comando para que la computadora mande un mensaje
#define CMD_IN "LOCAL"  // Comando de para que realice un evento interno
#define DIRECTION "127.0.0.1" // Direccion ip a la que se comunica

// ------- Estructura del mensaje de Lamport ------------
typedef struct {
    int  id_proceso;       // Id del proceso que envia el mensaje
    int  indice;           // El indice al que corresponde
    int  reloj;            // El Reloj de Lamport escalar (un solo contador)
    char peticion[200];    // Texto del mensaje a enviar
} Mensaje_L;

// --------- Declaraciones ----------------
int mayor(int a, int b);
void conexion_p2p(Mensaje_L user, int dir[]);
void enviar_msj(int puerto_destino, Mensaje_L datos);
void *hilo_escucha(void *arg);

#endif
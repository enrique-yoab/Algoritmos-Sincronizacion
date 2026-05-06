#ifndef VECTOR_H
#define VECTOR_H

#define NUM_NODOS 3    // Numero de computadoras a crear
#define CMD_EX "EXIT"  // Comando de apagado global
#define CMD_EN "ENVIAR" // Comando de inyección (Plano de control)

// ------- Estructura del mensaje de vectores (Relojes Vectoriales) ------------
// A diferencia de Lamport (escalar), aquí llevamos el registro de la causalidad de TODOS los procesos de la red.
typedef struct {
    int  id_proceso;             // ID del proceso que envia el mensaje
    int  indice;                 // Posición de este proceso dentro del arreglo vectorial
    int  vector[NUM_NODOS];      // El mapa causal completo: [ C0, C1, C2 ]
    char peticion[200];          // Texto del mensaje a enviar
} Mensaje_V;

// --------- Declaraciones de funciones ----------------
int mayor(int a, int b);
void conexion_p2p(Mensaje_V user, int dir[]);
void enviar_msj(int puerto_destino, Mensaje_V datos);
void imprimeV(int arr[]);

#endif
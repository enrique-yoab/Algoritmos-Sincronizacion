#ifndef VECTOR_H
#define VECTOR_H

#define NUM_NODOS 3    // Numero de computadoras a crear
#define CMD_EX "EXIT"  // Comando de apagado
#define CMD_EN "ENVIAR" // Comando para que la computadora mande un mensaje a otro puerto.

// ------- Estructura del mensaje de vectores (relojes logicos vectoriales) ------------
typedef struct {
    int  id_proceso;             // Id del proceso que envia el mensaje
    int  indice;                 // Índice de este proceso dentro del vector
    int  vector[NUM_NODOS];      // Vector completo: una entrada por proceso
    char peticion[200];          // Texto del mensaje a enviar
} Mensaje_V;

// --------- Declaración: retorna el mayor de dos enteros ----------------
int mayor(int a, int b);
// ---------------------- Crea una conexion p2p -------------------------- 
void conexion_p2p(Mensaje_V user, int dir[]);
// -------------- Se envia un mensaje a un puerto especifico ----------------
void enviar_msj(int puerto_destino, Mensaje_V datos);
// ----------------------- Imprime un vector -----------------------------
void imprimeV(int arr[]);

#endif
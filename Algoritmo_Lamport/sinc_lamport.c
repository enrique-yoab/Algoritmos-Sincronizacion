#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "lamport_p2p.h"

Mensaje_L instancia;
int directorio[NUM_NODOS];     

int main()
{
    // Variable para guardar la ejecucion de fork y saber si es correcto.
    pid_t pid = fork();
    if(pid < 0) exit(1);

    // Proceso hijo creara los n nodos clientes con conexion peer to peer
    if(pid == 0)
    {
        // Llenamos el directorio que almacena los puertos de cada nodo
        for(int i = 0; i < NUM_NODOS; i++) directorio[i] = 3000 + i;

        // For para crear N nodos cada quien con su respectivo puerto
        for(int i = 0; i < NUM_NODOS; i++)
        {
            // El proceso nieto rellena el mensaje que sera almacenado en la maquina p2p
            if(fork() == 0)
            {
                // Le registramos un id
                instancia.id_proceso = 5 * (i + 1);
                // Almacenamos el indice en el arreglo de puertos
                instancia.indice = i;
                // Iniciamos su reloj en 0
                instancia.reloj = 0;
                // Limpiamos el buffer de peticiones
                memset(instancia.peticion, 0, sizeof(instancia.peticion));
                // Pasamos la configuracion y el arreglo de puertos a cada maquina nueva
                conexion_p2p(instancia, directorio);
                // Cuando termina el bucle infinito termina el proceso nieto
                exit(0);
            }
        }
        // Esperamos a cada uno de los nietos creados
        for(int i = 0; i < NUM_NODOS; i++) wait(NULL);
        // Terminamos el proceso hijo
        exit(0);
    }
    // El padre se encargara de inyectar el comando de envio a un nodo aleatorio
    // Ademas de inyectar el comando de apagado a todos los nodos.
    else
    {
        // Semilla aleatoria para cada proceso
        srand(time(NULL) ^ getpid() << 16);
        // Damos 1 segundo para que todas las conexiones p2p esten listas
        sleep(1); 
        
        printf("\n=======================================================\n");
        printf(" [PADRE] INICIANDO SIMULACIÓN P2P (RELOJES DE LAMPORT) \n");
        printf("=======================================================\n");
        // Creamos una estructura que captura el mensaje a inyectarse en un nodo aleatorio
        Mensaje_L inyeccion;
        // Creamos un for que inyectara n eventos de envio a un nodo aleatorio
        for(int i = 0; i < NUM_EVENT0; i++) 
        {
            // Elegimos un indice aleatorio que seria un puerto
            int indice_elegido = rand() % NUM_NODOS;
            // Construimos el puerto del indice (3000 + indice)
            int puerto_elegido = 3000 + indice_elegido;
            // Limpiamos la estructura para construir la inyeccion al nodo
            memset(&inyeccion, 0, sizeof(Mensaje_L));
            // Agregamos el id del padre para evitar que cuente su reloj
            inyeccion.id_proceso = 99;
            // No tenemos un reloj ya que es "voluntariamente" la accion de enviar un mensaje
            inyeccion.reloj = 0; // EL PROCESO PADRE NO ALTERA EL TIEMPO
            // La escribimos en el buffer de peticion el comando de enviar
            if(rand() % 2 == 1) strcpy(inyeccion.peticion, CMD_EN);
            else strcpy(inyeccion.peticion, CMD_IN); 
            // Imprime la computadora a la que le ordenara realizar la accion y el puerto
            printf("\n[PADRE] Ordenando a la computadora %02d con el puerto %d a realizar un evento...\n", (indice_elegido + 1) * 5, puerto_elegido);
            // Enviamos el mensaje al puerto construido
            enviar_msj(puerto_elegido, inyeccion);
            // Se duerme el evento 0.1 s para que alcance a hacer el envio a la otra computadora
            usleep(100000);
        }

        printf("\n[PADRE] Simulación terminada. Esperando a que la red se estabilice...\n");

        printf("\n=======================================================\n");
        printf(" [PADRE] INICIANDO PROTOCOLO DE APAGADO GLOBAL \n");
        printf("=======================================================\n\n");

        // Ahora inyectamos el comando de apagado a todas las maquinas p2p
        for(int i = 0; i < NUM_NODOS; i++) 
        {
            memset(&inyeccion, 0, sizeof(Mensaje_L));
            inyeccion.id_proceso = 99; 
            inyeccion.reloj = 0;
            strcpy(inyeccion.peticion, CMD_EX); 
            
            printf("\n[PADRE] Enviando comando de apagado a puerto %d...\n", 3000 + i);
            enviar_msj(3000 + i, inyeccion);
            usleep(50000); 
        }

        wait(NULL); 
        printf("\n[SISTEMA GLOBAL] Todos los nodos P2P han finalizado exitosamente. Fin del programa.\n");
    }
    return 0;
}
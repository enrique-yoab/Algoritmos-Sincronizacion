#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "vector.h"

Mensaje_V instancia;
int directorio[NUM_NODOS];     // Agenda global de la red

int main()
{
    pid_t pid = fork();
    if(pid < 0) exit(1);

    // PROCESO HIJO (COORDINADOR): Levanta la infraestructura de red P2P
    if(pid == 0)
    {
        for(int i = 0; i < NUM_NODOS; i++) directorio[i] = 3000 + i;
        fflush(stdout);
        
        for(int i = 0; i < NUM_NODOS; i++)
        {
            if(fork() == 0)
            {
                // Configuración de fábrica del nodo
                instancia.id_proceso = 5 * (i + 1);
                instancia.indice = i;
                memset(instancia.vector, 0, sizeof(instancia.vector));
                memset(instancia.peticion, 0, sizeof(instancia.peticion));
                
                conexion_p2p(instancia, directorio);
                exit(0);
            }
        }
        for(int i = 0; i < NUM_NODOS; i++){ wait(NULL); }
        exit(0);
    }
    
    // PROCESO PADRE (SIMULADOR DE CAOS): Inyecta los eventos aleatoriamente
    else
    {
        srand(time(NULL) ^ getpid() << 16);
        sleep(1); // Damos 1 segundo para levantar sockets
        
        printf("\n=======================================================\n");
        printf(" [PADRE] INICIANDO SIMULACIÓN P2P (RELOJES VECTORIALES)\n");
        printf("=======================================================\n");

        Mensaje_V inyeccion;
        
        for(int i = 0; i < NUM_EVENTO; i++) 
        {
            int indice_elegido = rand() % NUM_NODOS;
            int puerto_elegido = 3000 + indice_elegido;
            
            memset(&inyeccion, 0, sizeof(Mensaje_V));
            
            // El Padre se identifica como "99".
            // El memset previo asegura que su vector enviado es [0,0,0].
            // Este proceso inyecta las instrucciones sin afectar los relojes
            inyeccion.id_proceso = 99; 
            if (rand() % 2 == 1) strcpy(inyeccion.peticion, CMD_EN); 
            else strcpy(inyeccion.peticion, CMD_IN);
            
            printf("\n[PADRE] Ordenando a la computadora C%02d (Puerto %d) que actúe...\n", (indice_elegido + 1) * 5, puerto_elegido);
                   
            enviar_msj(puerto_elegido, inyeccion);
            usleep(150000); // Ritmo al que se inyecta los eventos de envio o internos
        }

        printf("\n[PADRE] Simulación terminada. Esperando a que la red se estabilice...\n");
        sleep(1); 

        // APAGADO GLOBAL
        printf("\n=======================================================\n");
        printf(" [PADRE] INICIANDO PROTOCOLO DE APAGADO GLOBAL \n");
        printf("=======================================================\n\n");

        for(int i = 0; i < NUM_NODOS; i++) 
        {
            memset(&inyeccion, 0, sizeof(Mensaje_V));
            inyeccion.id_proceso = 99; 
            // Vector vacío garantizado por el memset
            strcpy(inyeccion.peticion, CMD_EX); 
            
            printf("[PADRE] Enviando comando de apagado a puerto %d...\n", 3000 + i);
            enviar_msj(3000 + i, inyeccion);
            usleep(50000); 
        }

        wait(NULL); 
        printf("\n[SISTEMA GLOBAL] Todos los nodos P2P han finalizado exitosamente.\n");
        printf("----> Simulacion Terminada.\n");
    }
    return 0;
}
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
    pid_t pid = fork();
    
    // Proceso hijo creara los n nodos clientes con conexion peer to peer
    if(pid == 0)
    {
        for(int i = 0; i < NUM_NODOS; i++) {
            directorio[i] = 3000 + i;
        }

        fflush(stdout);

        for(int i = 0; i < NUM_NODOS; i++)
        {
            if(fork() == 0)
            {
                instancia.id_proceso = 5 * (i + 1);
                instancia.indice = i;
                instancia.reloj = 0;
                memset(instancia.peticion, 0, sizeof(instancia.peticion));
                // Pasamos el índice (i) para que la función sepa qué puerto del directorio le toca
                conexion_p2p(instancia, directorio);
                exit(0);
            }
        }
        for(int i = 0; i < NUM_NODOS; i++){ wait(NULL); }
        exit(0);
    }
    // El padre se encargara de inyectar el comando de envio a un nodo aleatorio
    // Ademas de inyectar el comando de apagado a todos los nodos.
    else
    {
        srand(time(NULL) ^ getpid());
        // Damos 1 segundo para que todas las conexiones p2p esten listas
        sleep(1); 
        
        printf("\n=======================================================\n");
        printf(" [PADRE] INICIANDO SIMULACIÓN P2P (RELOJES DE LAMPORT) \n");
        printf("=======================================================\n");

        Mensaje_L msj_control;
        
        int num_eventos = 5; 
        for(int i = 0; i < num_eventos; i++) 
        {
            int indice_elegido = rand() % NUM_NODOS;
            int puerto_elegido = 3000 + indice_elegido;
            
            memset(&msj_control, 0, sizeof(Mensaje_L));
            msj_control.id_proceso = (indice_elegido * 5) + 1;
            msj_control.reloj = 0; // EL PADRE NO ALTERA EL TIEMPO
            strcpy(msj_control.peticion, CMD_EN); 
            
            printf("\n[PADRE] Ordenando a la computadora %02d con el puerto %d que mande un mensaje...\n", msj_control.id_proceso, puerto_elegido);
            enviar_msj(puerto_elegido, msj_control);
            
            usleep(10000); // Se duerme el evento 0.1 s para que alcance a hacer el envio a la otra computadora
        }

        printf("\n[PADRE] Simulación terminada. Esperando a que la red se estabilice...\n");
        sleep(1); 

        printf("\n=======================================================\n");
        printf(" [PADRE] INICIANDO PROTOCOLO DE APAGADO GLOBAL \n");
        printf("=======================================================\n\n");

        for(int i = 0; i < NUM_NODOS; i++) 
        {
            Mensaje_L msj_exit;
            memset(&msj_exit, 0, sizeof(Mensaje_L));
            msj_exit.id_proceso = 99; 
            msj_exit.reloj = 0;
            strcpy(msj_exit.peticion, CMD_EX); 
            
            printf("\n[PADRE] Enviando comando de apagado a puerto %d...\n", 3000 + i);
            enviar_msj(3000 + i, msj_exit);
            usleep(50000); 
        }

        wait(NULL); 
        printf("\n[SISTEMA GLOBAL] Todos los nodos P2P han finalizado exitosamente. Fin del programa.\n");
        exit(0);
    }
}
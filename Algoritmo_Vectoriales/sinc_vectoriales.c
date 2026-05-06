#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "vector.h"

Mensaje_V instancia;
int directorio[NUM_NODOS];     // Vector que almacenara el puerto de cada proceso

int main()
{
    pid_t pid = fork();
    
    // Proceso Hijo: Creara los clientes con conexion Peer to Peer
    if(pid == 0)
    {
        for(int i = 0; i < NUM_NODOS; i++)
        {
            directorio[i] = 3000 + i;
        }
        
        fflush(stdout);
        
        for(int i = 0; i < NUM_NODOS; i++)
        {
            if(fork() == 0)
            {
                instancia.id_proceso = 5 * (i + 1);
                instancia.indice = i;
                memset(instancia.vector, 0, sizeof(instancia.vector));
                memset(instancia.peticion, 0, sizeof(instancia.peticion));
                conexion_p2p(instancia, directorio);
                exit(0);
            }
        }
        // El hijo espera a los proceso nietos (conexiones p2p)
        for(int i = 0; i < NUM_NODOS; i++){wait(NULL);}
        exit(0);
    }
    // Proceso Padre: Se encargara de que las computadoras se envien mensajes aletorios entre si
    // Para poder lograr que el algoritmo de sincronzacion vectorial sea legitimo.
    // Proceso Padre: Simulara el Caos en la Red P2P
    else
    {
        // Semilla para los comandos aleatorios del Padre
        srand(time(NULL) ^ getpid());
        
        // Damos 3 segundos para que todos los hijos levanten sus puertos
        sleep(1); 
        
        printf("\n=======================================================\n");
        printf(" [PADRE] INICIANDO SIMULACIÓN DE TRÁFICO ALEATORIO P2P \n");
        printf("=======================================================\n");

        Mensaje_V msj_control;
        // 1. GENERAR EVENTOS ALEATORIOS EN LA RED (Ej. 5 mensajes cruzados)
        int num_eventos = 5; // Puedes subir este número para más caos
        for(int i = 0; i < num_eventos; i++) 
        {
            // El padre elige un nodo al azar para ordenarle que hable
            int indice_elegido = rand() % NUM_NODOS;
            int puerto_elegido = 3000 + indice_elegido;
            
            memset(&msj_control, 0, sizeof(Mensaje_V));
            msj_control.id_proceso = (indice_elegido + 1) * 5; // ID del nodo que envia mensaje
            strcpy(msj_control.peticion, CMD_EN); // Comando ENVIAR
            
            printf("\n[PADRE] Ordenando a la computadora C%02d en puerto %d que mande un mensaje...\n", msj_control.id_proceso, puerto_elegido);
            enviar_msj(puerto_elegido, msj_control);
            
            usleep(100000); // Se duerme el evento 0.5 s para que alcance a hacer el envio a la otra computadora
        }

        printf("\n[PADRE] Simulación terminada. Esperando a que la red se estabilice...\n");
        sleep(1); // 1 segundo de gracia para que lleguen los últimos paquetes

        // 2. APAGADO GLOBAL ORDENADO
        printf("\n=======================================================\n");
        printf(" [PADRE] INICIANDO PROTOCOLO DE APAGADO GLOBAL \n");
        printf("=======================================================\n\n");

        for(int i = 0; i < NUM_NODOS; i++) 
        {
            Mensaje_V msj_exit;
            memset(&msj_exit, 0, sizeof(Mensaje_V));
            msj_exit.id_proceso = 99; // EL ID del padre para cerrar conexiones p2p
            strcpy(msj_exit.peticion, CMD_EX); // Comando EXIT
            
            printf("\n[PADRE] Enviando comando de apagado a puerto %d...\n", 3000 + i);
            enviar_msj(3000 + i, msj_exit);
            usleep(100000); // Pequeña pausa entre envios para no saturar los sockets
        }

        wait(NULL); // Solo espera a que el proceso hijo acabe con los nietos
        
        printf("\n[SISTEMA GLOBAL] Todos los nodos P2P han finalizado exitosamente.\n");
        printf("----> Simulacion Terminada.\n");
        exit(0);
    }
}
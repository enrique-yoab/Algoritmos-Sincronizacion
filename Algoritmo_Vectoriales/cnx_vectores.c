#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "vector.h"

// Variables globales aisladas para cada proceso P2P gracias a fork()
int PORT = 0;               // Puerto asignado a este nodo
Mensaje_V local;            // Estructura que mantiene el vector y estado interno
Mensaje_V externo;          // Buffer para recibir mensajes de la red
int puertos[NUM_NODOS];     // Directorio de la red P2P (Agenda de vecinos)

int encendido = 1;          // Bandera de ciclo de vida del hilo principal
int enviar = 0;             // Bandera disparadora de eventos de envío

// Función auxiliar para calcular el máximo entre dos números
int mayor(int a, int b){return a > b ? a : b;}

// Esta funcion se encargara de enviar un mensaje a un puerto en especifico
void enviar_msj(int puerto_destino, Mensaje_V datos) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    // Creacion del socket tipo TCP para garantizar el envio y recepcion de mensaje
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return;

    serv_addr.sin_port = htons(puerto_destino);
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // Tratamos de conectarnos si no puede nos salimos del programa
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error de conexion");
        close(sock);
        return;
    }
    // Enviamos el mensaje al conectarnos en el puerto definido
    send(sock, &datos, sizeof(Mensaje_V), 0);
    // Cerramos el socket ya que enviamos.
    close(sock);
}

// Hilo Servidor: Se encarga exclusivamente de escuchar y reaccionar
void *hilo_escucha(void *arg)
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    printf("[C%02d] Escuchando por el puerto %d...\n", local.id_proceso, puertos[local.indice]);

    while (encendido)
    {
        int addrlen = sizeof(address);
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

        if (new_socket < 0) break;

        memset(&externo, 0, sizeof(Mensaje_V));
        read(new_socket, &externo, sizeof(Mensaje_V));

        // ALGORITMO DE FIDGE-MATTERN (RELOJES VECTORIALES) - REGLA DE RECEPCIÓN
        // Filtro de Inyección: Solo evaluamos matemáticamente si NO viene del Padre
        if (externo.id_proceso != 99) 
        {
            // 1. Regla del Merge: El conocimiento se absorbe tomando el mayor valor
            //    de cada componente entre nuestro vector y el vector entrante.
            for (int i = 0; i < NUM_NODOS; i++) {
                local.vector[i] = mayor(local.vector[i], externo.vector[i]);
            }
            // 2. Evento de Recepción: Incrementamos nuestro propio índice para marcar
            //    que ocurrió un evento en nuestra línea temporal.
            local.vector[local.indice]++;
        }

        printf("\nInicio -> Mensaje Computadora [C%02d] <-\n", local.id_proceso);
        printf("--- ID Proceso: %d\n", externo.id_proceso);
        printf("--- Peticion: %s\n", externo.peticion);
        printf("--- Reloj Externo: "); imprimeV(externo.vector);
        printf("--- Reloj Local:   "); imprimeV(local.vector);
        printf("Fin -> Mensaje Computadora [C%02d] <-\n", local.id_proceso);

        // Intérprete de Comandos de Control (Plano de Control)
        if (strcmp(externo.peticion, CMD_EX) == 0)
        {
            printf("\n[C%02d] Apagando por comando remoto...\n\n", local.id_proceso);
            encendido = 0; // Rompe el while de ambos hilos
        }
        else if(strcmp(externo.peticion, CMD_EN) == 0)
        {
            enviar = 1; // Dispara el evento de envío en el hilo principal
        }
        close(new_socket);
    }
    close(server_fd);
    return NULL;
}

// Hilo Principal: Motor del nodo P2P
void conexion_p2p(Mensaje_V user, int dir[])
{
    pthread_t thread_id;
    
    // Inyección de configuración local (El genoma del nodo)
    memcpy(&puertos, dir, sizeof(puertos));
    PORT = dir[user.indice];
    local.id_proceso = user.id_proceso;
    local.indice = user.indice;
    
    // Todo vector inicia en [0,0,0]. El encendido del proceso cuenta como el Evento 1
    local.vector[local.indice]++;
    
    // Desplegamos el "Oído" del nodo en un hilo concurrente
    pthread_create(&thread_id, NULL, hilo_escucha, NULL);
    
    srand(time(NULL) ^ getpid());

    while (encendido)
    {
        if (enviar == 1)
        {
            enviar = 0;
            
            // ALGORITMO DE FIDGE-MATTERN - REGLA DE ENVÍO
            // Todo evento local interno (como decidir enviar algo) avanza el reloj propio
            local.vector[local.indice]++; 

            // Decisión libre: El nodo decide aleatoriamente a quién contactar
            int indice_new;
            do {
                indice_new = rand() % NUM_NODOS;
            } while (indice_new == local.indice); // Evita hacerse ecos a sí mismo
            
            printf("\n[C%02d] Enviare mensaje a la computadora %02d con puerto %d...\n", 
                   local.id_proceso, (indice_new + 1) * 5, puertos[indice_new]);
                   
            memset(local.peticion, 0, sizeof(local.peticion));
            snprintf(local.peticion, sizeof(local.peticion), "Soy la computadora %02d", local.id_proceso);
            
            // Transmisión asíncrona del conocimiento causal
            enviar_msj(puertos[indice_new], local);
        }
        usleep(50000); // Pequeña pausa para no saturar el CPU en el ciclo while
    }
    pthread_join(thread_id, NULL);
}

// Función auxiliar de formateo visual
void imprimeV(int arr[])
{
    printf("[");
    for (int i = 0; i < NUM_NODOS; i++) { printf(" %d ", arr[i]); }
    printf("]\n");
}
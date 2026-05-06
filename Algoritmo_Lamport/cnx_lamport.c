#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "lamport_p2p.h"

int PORT = 0;       
Mensaje_L local;       
Mensaje_L externo;     
int puertos[NUM_NODOS];     

int encendido = 1;
int enviar = 0;

int mayor(int a, int b){return a > b ? a : b;}

void enviar_msj(int puerto_destino, Mensaje_L datos) {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(puerto_destino);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error de conexion");
        close(sock);
        return;
    }
    send(sock, &datos, sizeof(Mensaje_L), 0);
    close(sock);
}

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

    printf("[C%02d] Escuchando Mensaje_L en puerto %d...\n", local.id_proceso, PORT);

    while (encendido)
    {
        int addrlen = sizeof(address);
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

        if (new_socket < 0) break;

        memset(&externo, 0, sizeof(Mensaje_L));
        read(new_socket, &externo, sizeof(Mensaje_L));

        // =========================================================
        // REGLA DE LAMPORT: max(local, externo) + 1
        // =========================================================
        local.reloj = mayor(local.reloj, externo.reloj) + 1;

        printf("\nInicio -> Mensaje Computadora [C%02d] <-\n", local.id_proceso);
        printf("--- ID_proceso: %d\n", externo.id_proceso);
        printf("--- Peticion: %s\n", externo.peticion);
        printf("--- Reloj Externo (Ts): %d\n", externo.reloj);
        printf("--- Reloj Local Actualizado: %d\n", local.reloj);
        printf("Fin -> Mensaje Computadora [C%02d] <-\n", local.id_proceso);

        if (strcmp(externo.peticion, CMD_EX) == 0)
        {
            printf("\n[C%02d] Apagando por comando remoto...\n", local.id_proceso);
            encendido = 0;
        }else if(strcmp(externo.peticion, CMD_EN) == 0)
        {
            enviar = 1;
        }
        close(new_socket);
    }
    close(server_fd);
    return NULL;
}

void conexion_p2p(Mensaje_L user, int dir[])
{
    pthread_t thread_id;
    // Registramos el directorio de puertos
    memcpy(&puertos, dir, sizeof(puertos));
    // Registramos el puerto que desea utilizar y escuchar
    PORT = dir[user.indice]; // Usamos el índice pasado como argumento para saber nuestro puerto
    // Registramos el id que se le asigna
    local.id_proceso = user.id_proceso;
    // Registramos el indice al que corresponde su puerto
    local.indice = user.indice;
    // Inicialización del reloj lógico de Lamport
    local.reloj++; 

    pthread_create(&thread_id, NULL, hilo_escucha, NULL);
    
    srand(time(NULL) ^ getpid()); // Semilla única segura
    
    while (encendido)
    {
        if (enviar == 1)
        {
            enviar = 0;
            
            // REGLA DE ENVÍO DE LAMPORT: Avanzar reloj antes de enviar
            local.reloj++; 

            int indice_new;
            do{
                indice_new = rand() % NUM_NODOS;
            } while (indice_new == local.indice); // Evitamos enviarnos a nosotros mismos
            
            printf("\n[C%02d] Enviare mensaje a la computadora con puerto %d...\n", local.id_proceso, puertos[indice_new]);
            memset(local.peticion, 0, sizeof(local.peticion));
            snprintf(local.peticion, sizeof(local.peticion), "Soy la computadora %02d", local.id_proceso);
            
            enviar_msj(puertos[indice_new], local);
        }
    }
    pthread_join(thread_id, NULL);
}
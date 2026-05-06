#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "vector.h"

int PORT = 0;       // Puerto de escucha
Mensaje_V local;       // Estructura del mensaje de la computadora
Mensaje_V externo;     // Estructura para almacenar el mensaje recibido.
int puertos[NUM_NODOS];     // Vector que almacenara el puerto de cada proceso

// Bandera global para controlar el estado de la "computadora"
int encendido = 1;
int enviar = 0;

int mayor(int a, int b){return a > b ? a : b;}

void enviar_msj(int puerto_destino, Mensaje_V datos) {
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
    // Enviamos la estructura completa como un bloque de bytes
    send(sock, &datos, sizeof(Mensaje_V), 0);
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

    printf("[C%02d] Escuchando por el puerto %d...\n", local.id_proceso, puertos[local.indice]);

    while (encendido)
    {
        int addrlen = sizeof(address);
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

        if (new_socket < 0) break;

        // Limpiamos y leemos la estructura completa
        memset(&externo, 0, sizeof(Mensaje_V));
        read(new_socket, &externo, sizeof(Mensaje_V));

        // 1. Merge: tomar el máximo componente a componente
        for (int i = 0; i < NUM_NODOS; i++) {
            local.vector[i] = mayor(local.vector[i], externo.vector[i]);
        }
        // 2. Luego incrementar el propio índice (evento de recepción)
        local.vector[local.indice]++;

        printf("\nInicio -> Mensaje Computadora [C%02d] <-\n", local.id_proceso);
        printf("--- ID Proceso: %d\n", externo.id_proceso);
        printf("--- Peticion: %s\n", externo.peticion);
        printf("--- Reloj Externo: ");
        imprimeV(externo.vector);
        printf("--- Reloj Local: ");
        imprimeV(local.vector);
        printf("Fin -> Mensaje Computadora [C%02d] <-\n", local.id_proceso);

        // Condición de apagado basada en el texto dentro de la estructura
        if (strcmp(externo.peticion, CMD_EX) == 0)
        {
            printf("\n[C%02d] Apagando por comando remoto...\n\n", local.id_proceso);
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

void conexion_p2p(Mensaje_V user, int dir[])
{
    pthread_t thread_id;
    // Registramos el directorio de puertos
    memcpy(&puertos, dir, sizeof(puertos));
    // Registramos el puerto que desea utilizar y escuchar
    PORT = dir[user.indice];
    // Registramos el id que se le asigna
    local.id_proceso = user.id_proceso;
    // Registramos el indice al que corresponde en el vector
    local.indice = user.indice;
    // Aumentamos su reloj logico por ser un eveno del proceso
    local.vector[local.indice]++;
    // 1. Iniciamos el hilo de escucha (el "Servidor" interno)
    pthread_create(&thread_id, NULL, hilo_escucha, NULL);
    // 2. El hilo principal se queda monitoreando la bandera 'encendido' y 'enviar'
    // Por si necesita volverse cliente y enviar un mensaje a otro proceso o cerrar el proceso en si
    srand(time(NULL) ^ getpid());

    while (encendido)
    {
        if (enviar == 1)
        {
            enviar = 0;
            local.vector[local.indice]++; // se incrementa ya que es un evento de 'envio'

            int indice_new;
            do{
                indice_new = rand() % NUM_NODOS;
            } while (indice_new == local.indice);
            
            printf("\n[C%02d] Enviare mensaje a la computadora computadora %02d con puerto %d...\n", local.id_proceso, (indice_new + 1) * 5, puertos[indice_new]);
            memset(local.peticion, 0, sizeof(local.peticion));
            snprintf(local.peticion, sizeof(local.peticion), "Soy la computadora %02d", local.id_proceso);
            enviar_msj(puertos[indice_new], local);
        }
    }
    // 3. Limpieza y apagado
    pthread_join(thread_id, NULL);
}

void imprimeV(int arr[])
{
    printf("[");
    for (int i = 0; i < NUM_NODOS; i++)
    {
        printf(" %d ", arr[i]);
    }
    printf("]\n");
}
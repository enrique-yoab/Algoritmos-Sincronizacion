#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/wait.h>

#define PUERTO 3000
#define DIR_SERV "127.0.0.1"
#define TAM_MAX 1024

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error en fork");
        exit(1);
    }

    if (pid == 0) {
        /* --- PROCESO HIJO: SERVIDOR --- */
        int desc_socket, tam_dest; // socklen_t es el tipo recomendado por el estandar
        struct sockaddr_in direct, dest;
        char* buffer = "Saludos a todos desde el servidor";
        char men[TAM_MAX];
        
        printf("[Servidor] Iniciando...\n");
        desc_socket = socket(AF_INET, SOCK_DGRAM, 0);

        memset(&direct, 0, sizeof(direct));
        direct.sin_family = AF_INET;
        direct.sin_port = htons(PUERTO);
        direct.sin_addr.s_addr = INADDR_ANY;

        bind(desc_socket, (struct sockaddr *)&direct, sizeof(direct));

        tam_dest = sizeof(dest);
        printf("[Servidor] Esperando peticion...\n");
        
        recvfrom(desc_socket, men, sizeof(men) + 1, 0, (struct sockaddr *)&dest, &tam_dest);
        
        printf("[Servidor] Datos recibidos: %s \n", men);
        sendto(desc_socket, buffer, strlen(buffer) + 1, 0, (struct sockaddr *)&dest, sizeof(dest));
        
        close(desc_socket);
        exit(0); 
    } 
    else {
        /* --- PROCESO PADRE: CLIENTE --- */
        sleep(1); // Espera a que el servidor haga el bind
        printf("[Cliente] Iniciando peticion...\n");
        
        int desc_socket, tam_struct;
        struct sockaddr_in dest;
        char bufer[TAM_MAX];
        char *men = "Envio este mensaje desde el cliente...";

        desc_socket = socket(AF_INET, SOCK_DGRAM, 0);

        memset(&dest, 0, sizeof(dest));
        dest.sin_family = AF_INET;
        dest.sin_port = htons(PUERTO);
        inet_aton(DIR_SERV, &dest.sin_addr);

        sendto(desc_socket, men, strlen(men) + 1, 0, (struct sockaddr *)&dest, sizeof(dest));

        tam_struct = sizeof(dest);
        memset(bufer, 0, TAM_MAX);
        recvfrom(desc_socket, bufer, sizeof(bufer), 0, (struct sockaddr *)&dest, &tam_struct);
        
        printf("[Cliente] Servidor respondio: %s \n", bufer);
        
        close(desc_socket);
        wait(NULL); // Limpia el proceso zombie del hijo
        printf("[Sistema] Comunicacion finalizada exitosamente.\n");
    }
    return 0;
}
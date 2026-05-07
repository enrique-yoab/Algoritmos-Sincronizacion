#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>

#define PUERTO 3001
#define TAM_MAX 1024

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        /* --- SERVIDOR UDP --- */
        int desc_socket;
        struct sockaddr_in direct, dest;
        socklen_t tam_dest = sizeof(dest);
        char buffer_recibo[TAM_MAX];

        desc_socket = socket(AF_INET, SOCK_DGRAM, 0);
        direct.sin_family = AF_INET;
        direct.sin_port = htons(PUERTO);
        direct.sin_addr.s_addr = INADDR_ANY;

        bind(desc_socket, (struct sockaddr *)&direct, sizeof(direct));

        while (1) {
            memset(buffer_recibo, 0, TAM_MAX);
            recvfrom(desc_socket, buffer_recibo, TAM_MAX, 0, (struct sockaddr *)&dest, &tam_dest);
            
            printf("[Servidor UDP] Recibido: %s\n", buffer_recibo);

            if (strcmp(buffer_recibo, "exit") == 0) {
                printf("[Servidor UDP] Cerrando...\n");
                break;
            }

            sendto(desc_socket, "ACK UDP", 8, 0, (struct sockaddr *)&dest, tam_dest);
        }

        close(desc_socket);
        exit(0);
    } 
    else {
        /* --- CLIENTE UDP --- */
        sleep(1);
        int desc_socket = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in dest;
        char buffer_envio[TAM_MAX];
        char buffer_recibo[TAM_MAX];

        dest.sin_family = AF_INET;
        dest.sin_port = htons(PUERTO);
        inet_aton("127.0.0.1", &dest.sin_addr);

        while (1) {
            printf("[Cliente UDP] Mensaje (o 'exit'): ");
            fgets(buffer_envio, TAM_MAX, stdin);
            buffer_envio[strcspn(buffer_envio, "\n")] = 0;

            sendto(desc_socket, buffer_envio, strlen(buffer_envio) + 1, 0, (struct sockaddr *)&dest, sizeof(dest));

            if (strcmp(buffer_envio, "exit") == 0) break;

            socklen_t len = sizeof(dest);
            recvfrom(desc_socket, buffer_recibo, TAM_MAX, 0, (struct sockaddr *)&dest, &len);
            printf("[Cliente UDP] Respuesta: %s\n", buffer_recibo);
        }

        close(desc_socket);
        wait(NULL);
        printf("[Sistema] UDP Finalizado.\n");
    }
    return 0;
}
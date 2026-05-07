#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>

#define PUERTO 3000
#define TAM_MAX 1024

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        /* --- SERVIDOR TCP --- */
        int socket_escucha, socket_cliente;
        struct sockaddr_in serv_addr, cliente_addr;
        socklen_t tam_cliente = sizeof(cliente_addr);
        char buffer_recibo[TAM_MAX];

        socket_escucha = socket(AF_INET, SOCK_STREAM, 0);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PUERTO);
        serv_addr.sin_addr.s_addr = INADDR_ANY;

        bind(socket_escucha, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        listen(socket_escucha, 1);
        
        socket_cliente = accept(socket_escucha, (struct sockaddr *)&cliente_addr, &tam_cliente);

        while (1) {
            memset(buffer_recibo, 0, TAM_MAX);
            recv(socket_cliente, buffer_recibo, sizeof(buffer_recibo), 0);
            
            printf("[Servidor] Recibido: %s\n", buffer_recibo);

            if (strcmp(buffer_recibo, "exit") == 0) {
                printf("[Servidor] Comando de cierre recibido. Saliendo...\n");
                break;
            }

            send(socket_cliente, "Mensaje recibido", 17, 0);
        }

        close(socket_cliente);
        close(socket_escucha);
        exit(0);
    } 
    else {
        /* --- CLIENTE TCP --- */
        sleep(1);
        int desc_socket = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in serv_addr;
        char buffer_envio[TAM_MAX];
        char buffer_recibo[TAM_MAX];

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PUERTO);
        inet_aton("127.0.0.1", &serv_addr.sin_addr);

        connect(desc_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

        while (1) {
            printf("[Cliente] Escribe un mensaje (o 'exit' para salir): ");
            fgets(buffer_envio, TAM_MAX, stdin);
            buffer_envio[strcspn(buffer_envio, "\n")] = 0; // Quitar el salto de línea

            send(desc_socket, buffer_envio, strlen(buffer_envio) + 1, 0);

            if (strcmp(buffer_envio, "exit") == 0) break;

            recv(desc_socket, buffer_recibo, sizeof(buffer_recibo), 0);
            printf("[Cliente] Respuesta: %s\n", buffer_recibo);
        }

        close(desc_socket);
        wait(NULL);
        printf("[Sistema] TCP Finalizado.\n");
    }
    return 0;
}
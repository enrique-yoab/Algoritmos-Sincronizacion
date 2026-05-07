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
        /* --- PROCESO HIJO: SERVIDOR TCP --- */
        int socket_escucha, socket_cliente;
        struct sockaddr_in serv_addr, cliente_addr;
        socklen_t tam_cliente;
        char buffer_envio[] = "Saludos a todos desde el servidor TCP";
        char buffer_recibo[TAM_MAX];

        printf("[Servidor] Iniciando...\n");
        // 1. Crear socket de flujo (SOCK_STREAM para TCP)
        socket_escucha = socket(AF_INET, SOCK_STREAM, 0);

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PUERTO);
        serv_addr.sin_addr.s_addr = INADDR_ANY;

        // 2. Enlazar el socket
        bind(socket_escucha, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

        // 3. Poner el socket en modo escucha
        listen(socket_escucha, 5);
        printf("[Servidor] Esperando conexión...\n");

        // 4. Aceptar la conexión entrante
        tam_cliente = sizeof(cliente_addr);
        socket_cliente = accept(socket_escucha, (struct sockaddr *)&cliente_addr, &tam_cliente);
        
        // 5. Leer datos (usamos recv en lugar de recvfrom)
        recv(socket_cliente, buffer_recibo, sizeof(buffer_recibo), 0);
        printf("[Servidor] Datos recibidos: %s \n", buffer_recibo);

        // 6. Enviar respuesta (usamos send en lugar de sendto)
        send(socket_cliente, buffer_envio, strlen(buffer_envio) + 1, 0);

        close(socket_cliente);
        close(socket_escucha);
        exit(0);
    } 
    else {
        /* --- PROCESO PADRE: CLIENTE TCP --- */
        sleep(1); // Espera a que el servidor esté listo
        printf("[Cliente] Iniciando conexión...\n");

        int desc_socket;
        struct sockaddr_in serv_addr;
        char buffer_recibo[TAM_MAX];
        char *mensaje = "Envio este mensaje desde el cliente vía TCP...";

        // 1. Crear socket de flujo
        desc_socket = socket(AF_INET, SOCK_STREAM, 0);

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PUERTO);
        inet_aton(DIR_SERV, &serv_addr.sin_addr);

        // 2. Conectar al servidor
        if (connect(desc_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("[Cliente] Error al conectar");
            exit(1);
        }

        // 3. Enviar y recibir datos
        send(desc_socket, mensaje, strlen(mensaje) + 1, 0);
        recv(desc_socket, buffer_recibo, sizeof(buffer_recibo), 0);
        
        printf("[Cliente] Servidor respondió: %s \n", buffer_recibo);

        close(desc_socket);
        wait(NULL); // Limpiar proceso hijo
        printf("[Sistema] Comunicación TCP finalizada exitosamente.\n");
    }
    return 0;
}
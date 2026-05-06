#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "lamport_p2p.h"

// Al ser memoria aislada por fork creamos variables globales
// Que identifiquen a la maquina p2p creada con este codigo
int PORT = 0;  // Puerto 0 ya que sera un parametro de entrada
Mensaje_L local;  // Este servira para que la maquina configure su mensaje a enviar 
Mensaje_L externo;   // Este servira para recibir cualquier mensaje externo
int puertos[NUM_NODOS]; // El arreglo que contiene los puertos

int encendido = 1; // Se establece en 1 ya que se quedara encendido
int enviar = 0; // Se queda en 0 hasta que se decida enviar un mensaje

// Esta funcion encuentra el mayor de dos numeros con el perador terniario
int mayor(int a, int b){return a > b ? a : b;}

// Esta funcion se encargara de enviar un mensaje a un puerto en especifico
void enviar_msj(int puerto_destino, Mensaje_L datos) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    // Creacion del socket tipo TCP para garantizar el envio y recepcion de mensaje
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return;

    serv_addr.sin_port = htons(puerto_destino);
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, DIRECTION, &serv_addr.sin_addr);

    // Tratamos de conectarnos si no puede nos salimos del programa
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error de conexion");
        close(sock);
        return;
    }
    // Enviamos el mensaje al conectarnos en el puerto definido
    send(sock, &datos, sizeof(Mensaje_L), 0);
    // Cerramos el socket ya que enviamos.
    close(sock);
}

// Funcion que utilizara el hilo para actuar como servidor
void *hilo_escucha(void *arg)
{
    int server_fd;    // Descriptor del socket servidor (el que escucha conexiones entrantes)
    int new_socket;   // Descriptor del socket aceptado (uno por cada cliente que se conecta)
    struct sockaddr_in address;  // Estructura que almacena la dirección IP y puerto del servidor
    int opt = 1;      // Valor para activar la opción SO_REUSEADDR en el socket

    // Creamos el socket TCP (SOCK_STREAM) que actuará como servidor de este nodo
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Permite reutilizar el puerto inmediatamente después de que el proceso termine,
    // evitando el error "Address already in use" en reinicios rápidos
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family      = AF_INET;       // Familia de direcciones IPv4
    address.sin_addr.s_addr = INADDR_ANY;    // Acepta conexiones de cualquier IP local
    address.sin_port        = htons(PORT);   // Convierte el puerto a orden de bytes de red (big-endian)

    // Asocia el socket al puerto y dirección configurados en 'address'
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    // Pone el socket en modo escucha; admite hasta 3 conexiones en cola de espera
    listen(server_fd, 3);

    printf("[C%02d] Escuchando Mensaje_L en puerto %d...\n", local.id_proceso, PORT);

    // Bucle activo mientras la bandera 'encendido' sea 1
    while (encendido)
    {
        int addrlen = sizeof(address);   // Tamaño de la estructura de dirección (requerido por accept)

        // Espera bloqueante: se desbloquea cuando llega una conexión entrante.
        // Devuelve un nuevo descriptor (new_socket) exclusivo para esa conexión.
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

        // Si accept falla (por ejemplo, el socket fue cerrado externamente) salimos del bucle
        if (new_socket < 0) break;

        // Limpiamos el buffer 'externo' para evitar basura de mensajes anteriores
        memset(&externo, 0, sizeof(Mensaje_L));

        // Leemos del socket el bloque de bytes que forma el Mensaje_L completo
        read(new_socket, &externo, sizeof(Mensaje_L));

        // Verificamos el tipo de petición recibida para actuar en consecuencia
        if(externo.id_proceso == 99)
        {
            if (strcmp(externo.peticion, CMD_EX) == 0)
            {
                // Comando de apagado: se baja la bandera para que ambos hilos terminen
                printf("\n[C%02d] Apagando por comando remoto...\n", local.id_proceso);
                encendido = 0;   // El hilo cliente y este hilo servidor saldrán de sus bucles
            }
            else if (strcmp(externo.peticion, CMD_EN) == 0)
            {
                // Comando de envío: activa la bandera para que el hilo cliente envíe un mensaje
                enviar = 1; // El hilo cliente detectará esto en su próxima iteración
            }
            else if (strcmp(externo.peticion, CMD_IN) == 0)
            {
                printf("\n[C%02d] -> EVENTO INTERNO <- Ejecutando tarea local...\n", local.id_proceso);
                local.reloj++;
            }
        }
        else
        {
            // REGLA DE LAMPORT AL RECIBIR 
            // Si el emisor es el proceso padre (id == 99), es un comando de control,
            // no un evento de la red P2P real, por lo que NO se actualiza el reloj.
            // Si es un nodo real, aplicamos: reloj_local = max(reloj_local, reloj_externo) + 1
            // El +1 representa el evento de recepción del mensaje.
            local.reloj = mayor(local.reloj, externo.reloj) + 1;           
        }

        // Imprimimos el detalle del mensaje recibido y el estado del reloj
        printf("\nInicio -> Mensaje Computadora [C%02d] <-\n", local.id_proceso);
        printf("--- ID_proceso: %d\n",              externo.id_proceso);  // Quién envió el mensaje
        printf("--- Peticion: %s\n",                externo.peticion);    // Contenido del mensaje
        printf("--- Reloj Externo (Ts): %d\n",      externo.reloj);       // Reloj del emisor
        printf("--- Reloj Local Actualizado: %d\n", local.reloj);         // Reloj local tras el merge
        printf("Fin -> Mensaje Computadora [C%02d] <-\n", local.id_proceso);

        // Cerramos el socket de esta conexión específica; el servidor queda listo
        // para aceptar la siguiente conexión en la próxima iteración del while
        close(new_socket);
    }

    // Cerramos el socket servidor al salir del bucle (nodo apagado)
    close(server_fd);
    return NULL;  // El hilo termina y libera sus recursos
}

void conexion_p2p(Mensaje_L user, int dir[])
{
    // Creamos una variable tipo hilo para la creacion del hilo servidor
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

    // Se crea un hilo que actuara como servidor (recibe mensajes)
    pthread_create(&thread_id, NULL, hilo_escucha, NULL);
    
    srand(time(NULL) ^ getpid()); // Semilla única segura
    
    // Actua como otro hilo que se encarga de ser cliente (envia mensajes)
    while (encendido)
    {
        // Si enviar cambia se realiza la accion de enviar mensaje
        if (enviar == 1)
        {
            // Invertimos la condicion ya que es solo una vez
            enviar = 0;
            
            // REGLA DE LAMPORT AL ENVIAR: Avanzar reloj antes de enviar
            local.reloj++; 

            int indice_new;
            // Obtenemos un indice aleatorio y evitamos enviarnos a nosotros mismos si no repite
            do{
                indice_new = rand() % NUM_NODOS;
            } while (indice_new == local.indice); 
            
            printf("\n[C%02d] Enviare mensaje a la computadora con puerto %d...\n", local.id_proceso, puertos[indice_new]);
            // Limpiamos nuestro buffer de peticiones
            memset(local.peticion, 0, sizeof(local.peticion));
            // Le grabamos el mensaje al buffer de peticiones
            snprintf(local.peticion, sizeof(local.peticion), "Soy la computadora %02d", local.id_proceso);
            // Enviamos el mensaje al puerto aleatorio escogido.
            enviar_msj(puertos[indice_new], local);
        }
    }
    // Esperamos a que termine la ejecucion del hilo servidor
    pthread_join(thread_id, NULL);
}
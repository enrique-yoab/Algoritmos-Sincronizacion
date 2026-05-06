#include <stdio.h>        // printf, perror
#include <stdlib.h>       // exit, rand, srand
#include <string.h>       // memset, strcpy, strcmp
#include <unistd.h>       // fork, sleep, usleep, getpid, close
#include <sys/wait.h>     // wait
#include <arpa/inet.h>    // inet_aton, htons
#include <netinet/in.h>   // sockaddr_in
#include <time.h>         // clock_gettime, localtime
#include "header.h"       // estructuras tiempo / Mensaje_C y constantes

// Variables globales para definir el limite en la respuesta
// Y para el retardo maximo del servidor a la hora de enviar respuestas.
#define RTT_MAX_MS  500         // 500 ms como límite razonable en loopback
#define DELAY_SERV  300000      // Retardo máximo del servidor en µs (300 ms < RTT_MAX_MS)

// Esta funcion imprime una estructura tiempo con formato H:M:S:ms
void mostrar_hora(tiempo *t)
{
    printf("[%d:%d:%d:%d]\n",   // formato H:M:S:ms
           t->horas,
           t->minutos,
           t->segundos,
           t->milisegundos);
}

// Lee el reloj del sistema en tiempo real y llena la estructura tiempo
void capturar_tiempo(tiempo *t)
{
    struct timespec ts;                     // estructura con segundos y nanosegundos
    struct tm *tm_info;                     // estructura para desglosar hora local

    clock_gettime(CLOCK_REALTIME, &ts);     // obtiene hora del reloj del sistema
    tm_info = localtime(&ts.tv_sec);        // convierte segundos Unix a hora local

    t->milisegundos = ts.tv_nsec / 1000000; // 1 000 000 ns = 1 ms
    t->segundos     = tm_info->tm_sec;      // segundos (0-59)
    t->minutos      = tm_info->tm_min;      // minutos  (0-59)
    t->horas        = tm_info->tm_hour;     // horas    (0-23)
}

// ─────────────────────────────────────────────────────────────
// SERVIDOR — "El Gestor del Tiempo"
// ─────────────────────────────────────────────────────────────
void ejecutar_servidor_C()
{
    int desc_socket;                 // descriptor del socket UDP del servidor
    struct sockaddr_in direct;       // dirección propia del servidor
    struct sockaddr_in cliente;      // dirección del cliente que llama
    socklen_t tam_cliente;           // tamaño de la dirección del cliente
    Mensaje_C msj;                   // buffer para el mensaje recibido/enviado

    printf("\n[SERVIDOR] Reloj Maestro iniciado en el puerto %d...\n", PUERTO);

    desc_socket = socket(AF_INET, SOCK_DGRAM, 0);   // socket UDP (sin conexión)

    memset(&direct, 0, sizeof(direct));              // limpia la estructura de dirección
    direct.sin_family      = AF_INET;                // familia IPv4
    direct.sin_port        = htons(PUERTO);          // puerto en orden de red
    direct.sin_addr.s_addr = INADDR_ANY;             // acepta peticiones de cualquier IP

    // Se asocia el socket a la direccion "escucha por el puerto"
    bind(desc_socket, (struct sockaddr *)&direct, sizeof(direct)); 

    while (1)                                        // bucle infinito hasta recibir EXIT
    {
        tam_cliente = sizeof(cliente);               // resetea el tamaño antes de cada recvfrom
        memset(&msj, 0, sizeof(Mensaje_C));          // limpia el buffer del mensaje

        // Se queda esperando hasta recibir un datagrama de algun cliente.
        recvfrom(desc_socket, &msj, sizeof(Mensaje_C), 0, (struct sockaddr *)&cliente, &tam_cliente);

        if (strcmp(msj.peticion, "EXIT") == 0)       // comando de apagado remoto
        {
            printf("\n[SERVIDOR] Comando EXIT recibido. Apagando.\n");
            break;                                   // sale del bucle y cierra el servidor
        }
        else if (strcmp(msj.peticion, "ACTUALIZAR") == 0) // petición de sincronización
        {
            // ── CORRECCIÓN 1 ──────────────────────────────────────────────────
            // Ts debe capturarse INMEDIATAMENTE al recibir la petición,
            // antes de cualquier procesamiento o retardo artificial.
            // Si se captura después del usleep, Ts ya refleja el tiempo
            // transcurrido durante el sleep, y sumar RTT/2 empujaría el
            // reloj del cliente hacia el futuro de forma incorrecta.
            capturar_tiempo(&msj.tiempo_servidor);   // ← Ts capturado primero

            usleep(rand() % DELAY_SERV);             // simula latencia de procesamiento
                                                     // (va DESPUÉS de capturar Ts)

            printf("\n[SERVIDOR] Petición de PID %d. Hora despachada: ",
                   msj.id_proceso);
            mostrar_hora(&msj.tiempo_servidor);

            strcpy(msj.peticion, "OK");              // cambia la petición a confirmación

            // Devuelve el mensaje (con Ts dentro) al cliente que lo solicitó
            sendto(desc_socket,
                   &msj, sizeof(Mensaje_C), 0,
                   (struct sockaddr *)&cliente, tam_cliente);
        }
    }

    close(desc_socket);                              // libera el descriptor del socket
}

// ─────────────────────────────────────────────────────────────
// CLIENTE — (El Esclavo que aplica la Matemática)
// ─────────────────────────────────────────────────────────────
void ejecutar_cliente_C(int indice)
{
    srand(time(NULL) ^ (getpid() << 16));   // semilla única por proceso (XOR con PID)
    usleep(rand() % 500000);                // retardo aleatorio para escalonar los clientes

    int desc_socket;             // descriptor del socket UDP del cliente
    struct sockaddr_in dest;     // dirección del servidor destino
    Mensaje_C msj;               // buffer del mensaje enviado/recibido
    tiempo reloj_sincronizado;   // resultado final: hora ajustada por Cristian

    desc_socket = socket(AF_INET, SOCK_DGRAM, 0);   // socket UDP

    memset(&dest, 0, sizeof(dest));         // limpia la estructura de dirección
    dest.sin_family = AF_INET;              // familia IPv4
    dest.sin_port   = htons(PUERTO);        // puerto del servidor en orden de red
    inet_aton(DIR_SERV, &dest.sin_addr);    // convierte la IP del servidor a binario

    // ── CORRECCIÓN 2: bucle de reintento por RTT alto ────────────────────────
    // El algoritmo de Cristian especifica que si el RTT es demasiado grande,
    // la estimación Ts + RTT/2 tiene demasiado error y el resultado no es
    // confiable. En ese caso se debe repetir la petición hasta obtener
    // un RTT dentro del umbral aceptable (RTT_MAX_MS).
    long long rtt_ms;            // RTT medido en este intento
    int intentos = 0;            // contador de intentos realizados

    // Utilizamos un ciclo do-while por si la diferencia de tiempo es mayor al limite establecido
    do
    {
        intentos++; // incrementa el contador de intentos

        memset(&msj, 0, sizeof(Mensaje_C));   // limpia el mensaje antes de cada intento
        msj.id_proceso = getpid();            // identifica al proceso cliente
        strcpy(msj.peticion, "ACTUALIZAR");   // petición de sincronización al servidor

        // ── PASO 1: Capturar T0 y enviar la petición ─────────────────────────
        capturar_tiempo(&msj.tiempo_envio);   // T0: estampa de tiempo de salida
        sendto(desc_socket,
               &msj, sizeof(Mensaje_C), 0,
               (struct sockaddr *)&dest, sizeof(dest));

        // ── PASO 2: Recibir la respuesta del servidor (Ts viaja dentro de msj)
        socklen_t tam_struct = sizeof(dest);
        recvfrom(desc_socket,
                 &msj, sizeof(Mensaje_C), 0,
                 (struct sockaddr *)&dest, &tam_struct);

        // ── PASO 3: Capturar T1 ───────────────────────────────────────────────
        capturar_tiempo(&msj.tiempo_respuesta);  // T1: estampa de tiempo de llegada

        // Usamos long long para numeros de 64 bits y poder tener mejor aproximacion
        // Realizamos la conversion de horas, minutos y segundos a milisegundos
        // 1 hora = 3600 segundos * 1000 ms
        // 1 minuto = 60 segundos * 1000 ms
        // 1 segundo = 1000 ms
        // Convertimos el tiempo de envio a T0 a milisegundos totales desde medianoche
        long long t0_ms = (long long)msj.tiempo_envio.horas * 3600000LL +
                          (long long)msj.tiempo_envio.minutos * 60000LL +
                          (long long)msj.tiempo_envio.segundos * 1000LL +
                          msj.tiempo_envio.milisegundos;

        // Convertimos el tiempo de llegada a T1 a milisegundos totales desde medianoche
        long long t1_ms = (long long)msj.tiempo_respuesta.horas * 3600000LL +
                          (long long)msj.tiempo_respuesta.minutos * 60000LL +
                          (long long)msj.tiempo_respuesta.segundos * 1000LL +
                          msj.tiempo_respuesta.milisegundos;

        rtt_ms = t1_ms - t0_ms; // RTT = T1 - T0 (diferencia de tiempo de ida y vuelta)

        if (rtt_ms > RTT_MAX_MS)  // si el rango supera el umbral muestra mensaje y repite
            printf("[CLIENTE #%02d] RTT = %lld ms demasiado alto, reintentando... (intento %d)\n", indice + 1, rtt_ms, intentos);

    } while (rtt_ms > RTT_MAX_MS);          // repite hasta obtener un RTT aceptable

    // FÓRMULA DE CRISTIAN -> si la respuesta entre el servidor y el cliente estan dentro del limite se corrige el reloj
    // El reloj del servidor mas la diferencia de tiempo ida y vuelta del mensaje entre 2 (asumiendo simetria)
    // hora_corregida = Ts + (T1 - T0) / 2
    // La mitad del RTT aproxima el tiempo que tardó el mensaje en llegar desde el servidor hasta el cliente

    // Convertimos el timepo del servidor Ts a milisegundos totales desde medianoche
    long long ts_ms = (long long)msj.tiempo_servidor.horas * 3600000LL +
                      (long long)msj.tiempo_servidor.minutos * 60000LL +
                      (long long)msj.tiempo_servidor.segundos * 1000LL +
                      msj.tiempo_servidor.milisegundos;

    long long latencia_ms      = rtt_ms / 2;            // latencia estimada = RTT / 2
    long long hora_corregida_ms = ts_ms + latencia_ms;  // hora ajustada por Cristian

    // Reconstruimos la hora corregida en la estructura tiempo (h:m:s:ms)
    reloj_sincronizado.milisegundos = hora_corregida_ms % 1000;          // residuo en ms

    long long seg_totales           = hora_corregida_ms / 1000;          // total en segundos
    reloj_sincronizado.segundos     = seg_totales % 60;                  // segundos (0-59)

    long long min_totales           = seg_totales / 60;                  // total en minutos
    reloj_sincronizado.minutos      = min_totales % 60;                  // minutos (0-59)

    long long horas_totales         = min_totales / 60;                  // total en horas
    reloj_sincronizado.horas        = horas_totales % 24;                // horas formato 24h

    // Imprimimos todos los tiempos y el resultado final
    printf("\n[CLIENTE #%02d | PID %d] ---> T0 (Salida)    : ", indice + 1, getpid());
    mostrar_hora(&msj.tiempo_envio);

    printf("[CLIENTE #%02d | PID %d] ---> T1 (Llegada)   : ", indice + 1, getpid());
    mostrar_hora(&msj.tiempo_respuesta);

    printf("[CLIENTE #%02d | PID %d] ---> Ts (Servidor)  : ", indice + 1, getpid());
    mostrar_hora(&msj.tiempo_servidor);

    printf("[CLIENTE #%02d | PID %d] ---> RTT             : %lld ms (intento(s): %d)\n",
           indice + 1, getpid(), rtt_ms, intentos);   // muestra el RTT final aceptado

    printf("[CLIENTE #%02d | PID %d] ---> Latencia Calc  : %lld ms\n",
           indice + 1, getpid(), latencia_ms);

    printf("[CLIENTE #%02d | PID %d] ---> RELOJ AJUSTADO : ", indice + 1, getpid());
    mostrar_hora(&reloj_sincronizado);

    close(desc_socket);   // libera el descriptor del socket
    usleep(1000000);      // espera 1 s para que el servidor procese antes de salir
    exit(0);              // el proceso hijo termina limpiamente
}

// ─────────────────────────────────────────────────────────────
// MAIN
// ─────────────────────────────────────────────────────────────
int main()
{
    pid_t pid = fork();              // crea el proceso hijo (coordinador de clientes)

    if (pid < 0) exit(1);           // error al hacer fork

    // ── PROCESO PADRE: ejecuta el servidor ───────────────────────────────────
    if (pid > 0)
    {
        ejecutar_servidor_C();       // bloquea aquí hasta recibir EXIT

        // ── CORRECCIÓN 3 ──────────────────────────────────────────────────────
        // El wait(NULL) original podía devolver -1 silenciosamente porque el
        // proceso hijo (coordinador) ya había terminado antes de que el servidor
        // recibiera EXIT. Se usa waitpid con el PID exacto del hijo para evitar
        // esperar a un proceso que ya no existe, y se ignora el error si ya terminó.
        int estado;
        waitpid(pid, &estado, 0);    // espera específicamente al hijo coordinador

        printf("\n[SISTEMA] Simulación finalizada.\n");
        exit(0);
    }

    // ── PROCESO HIJO: coordina y lanza los clientes ──────────────────────────
    else
    {
        sleep(1);                    // espera a que el servidor esté listo

        for (int i = 0; i < N_CLIENTES; i++)       // lanza N_CLIENTES procesos nietos
        {
            if (fork() == 0)                        // cada nieto es un cliente independiente
                ejecutar_cliente_C(i);              // ejecuta el algoritmo de Cristian y hace exit(0)
        }

        while (wait(NULL) > 0);     // espera a que todos los nietos (clientes) terminen

        // Todos los clientes terminaron: ordenar el apagado del servidor
        int sock_exit = socket(AF_INET, SOCK_DGRAM, 0);  // socket temporal para enviar EXIT

        struct sockaddr_in dest_exit;
        memset(&dest_exit, 0, sizeof(dest_exit));         // limpia la dirección destino
        dest_exit.sin_family = AF_INET;                   // familia IPv4
        dest_exit.sin_port   = htons(PUERTO);             // puerto del servidor
        inet_aton(DIR_SERV, &dest_exit.sin_addr);         // IP del servidor

        Mensaje_C msj_exit;
        memset(&msj_exit, 0, sizeof(Mensaje_C));          // limpia el mensaje de salida
        strcpy(msj_exit.peticion, "EXIT");                // comando de apagado

        // Envía el EXIT al servidor para que salga de su bucle
        sendto(sock_exit,
               &msj_exit, sizeof(Mensaje_C), 0,
               (struct sockaddr *)&dest_exit, sizeof(dest_exit));

        close(sock_exit);   // libera el socket temporal
        exit(0);            // el proceso hijo coordinador termina
    }
}
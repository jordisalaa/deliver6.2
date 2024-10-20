#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <pthread.h>

int contador_global;

// Inicialización del mutex para el acceso exclusivo
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int num_conexiones;
int conexiones[100];

void *ManejarCliente(void *socket_cliente)
{
    int sockfd;
    int *sockptr;
    sockptr = (int *) socket_cliente;
    sockfd = *sockptr;
    
    char buffer_peticion[512];
    char buffer_respuesta[512];
    int bytes_leidos;
    
    int desconectar = 0;
    // Bucle para manejar peticiones del cliente hasta desconexión
    while (desconectar == 0)
    {
        // Leer la petición del cliente
        bytes_leidos = read(sockfd, buffer_peticion, sizeof(buffer_peticion));
        printf("Datos recibidos\n");
        
        // Añadir terminador de cadena
        buffer_peticion[bytes_leidos] = '\0';
        
        printf("Petición recibida: %s\n", buffer_peticion);
        
        // Dividir la petición para interpretar el código y parámetros
        char *token = strtok(buffer_peticion, "/");
        int codigo_operacion = atoi(token);
        int id_formulario;
        float temperatura_valor;
        
        if (codigo_operacion != 0)
        {
            token = strtok(NULL, "/");
            id_formulario = atoi(token);
            token = strtok(NULL, "/");
            temperatura_valor = atof(token);
            printf("Código: %d, Temperatura: %.2f\n", codigo_operacion, temperatura_valor);
        }
        
        if (codigo_operacion == 0) // Desconectar cliente
        {
            desconectar = 1;
        }
        else if (codigo_operacion == 1) // Conversión Celsius a Fahrenheit
        {
            float fahrenheit = (temperatura_valor * 9.0 / 5.0) + 32.0;
            sprintf(buffer_respuesta, "1/%d/%.2f Celsius equivale a %.2f Fahrenheit", id_formulario, temperatura_valor, fahrenheit);
        }
        else if (codigo_operacion == 2) // Conversión Fahrenheit a Celsius
        {
            float celsius = (temperatura_valor - 32.0) * 5.0 / 9.0;
            sprintf(buffer_respuesta, "2/%d/%.2f Fahrenheit equivale a %.2f Celsius", id_formulario, temperatura_valor, celsius);
        }
        
        if (codigo_operacion != 0)
        {
            printf("Respuesta enviada: %s\n", buffer_respuesta);
            // Enviar la respuesta al cliente
            write(sockfd, buffer_respuesta, strlen(buffer_respuesta));
        }
        
        // Si la operación fue de conversión (código 1 o 2), actualizar contador
        if ((codigo_operacion == 1) || (codigo_operacion == 2))
        {
            pthread_mutex_lock(&lock); // Bloquear acceso a la sección crítica
            contador_global = contador_global + 1;
            pthread_mutex_unlock(&lock); // Liberar el acceso
            
            // Notificar a todos los clientes conectados el nuevo contador
            char mensaje_notificacion[20];
            sprintf(mensaje_notificacion, "4/%d", contador_global);
            for (int j = 0; j < num_conexiones; j++)
                write(conexiones[j], mensaje_notificacion, strlen(mensaje_notificacion));
        }
    }
    // Cerrar la conexión del cliente
    close(sockfd);
}

int main(int argc, char *argv[])
{
    int sockfd_cliente, sockfd_servidor;
    struct sockaddr_in direccion_servidor;
    
    // INICIALIZACIONES
    // Crear socket
    if ((sockfd_servidor = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("Error al crear socket\n");
    
    // Asignar el puerto al socket
    memset(&direccion_servidor, 0, sizeof(direccion_servidor)); // Poner a cero
    direccion_servidor.sin_family = AF_INET;
    direccion_servidor.sin_addr.s_addr = htonl(INADDR_ANY); // Cualquier dirección local
    direccion_servidor.sin_port = htons(9064); // Puerto 9064
    
    // Asignar la dirección al socket
    if (bind(sockfd_servidor, (struct sockaddr *)&direccion_servidor, sizeof(direccion_servidor)) < 0)
        printf("Error en bind\n");
    
    // Comenzar a escuchar conexiones
    if (listen(sockfd_servidor, 3) < 0)
        printf("Error en listen\n");
    
    contador_global = 0;
    
    pthread_t thread;
    num_conexiones = 0;
    while (1)
    {
        printf("Esperando conexiones\n");
        
        // Aceptar nuevas conexiones
        sockfd_cliente = accept(sockfd_servidor, NULL, NULL);
        printf("Conexión recibida\n");
        
        conexiones[num_conexiones] = sockfd_cliente;
        
        // Crear un hilo para manejar al cliente
        pthread_create(&thread, NULL, ManejarCliente, &conexiones[num_conexiones]);
        num_conexiones = num_conexiones + 1;
    }
}

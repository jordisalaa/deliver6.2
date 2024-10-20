#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <pthread.h>
#include <ctype.h> // Para funciones de conversión de caracteres como tolower() y toupper()

int totalSolicitudes;

// Inicialización del mutex para el acceso exclusivo
pthread_mutex_t control_mutex = PTHREAD_MUTEX_INITIALIZER;

int numClientes;
int conexiones[100];

void *GestionarCliente (void *sock_cliente)
{
	int cliente_sock;
	int *sock;
	sock = (int *) sock_cliente;
	cliente_sock = *sock;
	
	char solicitud[512];
	char respuesta[512];
	int longitud_leida;
	
	int continuar = 1;
	// Bucle para atender peticiones del cliente hasta que se desconecte
	while (continuar == 1)
	{
		// Leer la solicitud del cliente
		longitud_leida = read(cliente_sock, solicitud, sizeof(solicitud));
		printf("Solicitud recibida\n");
		
		// Añadir terminador de cadena
		solicitud[longitud_leida] = '\0';
		
		printf("Solicitud: %s\n", solicitud);
		
		// Procesar la solicitud
		char *token = strtok(solicitud, "/");
		int codigoOperacion = atoi(token);
		int idSolicitud;
		char nombreCliente[20];
		
		if (codigoOperacion != 0)
		{
			token = strtok(NULL, "/");
			idSolicitud = atoi(token);
			token = strtok(NULL, "/");
			strcpy(nombreCliente, token);
			printf("Código: %d, Nombre: %s\n", codigoOperacion, nombreCliente);
		}
		
		if (codigoOperacion == 0) // Solicitud de desconexión
			continuar = 0;
		else if (codigoOperacion == 1) // Longitud del nombre
			sprintf(respuesta, "1/%d/%d", idSolicitud, strlen(nombreCliente));
		else if (codigoOperacion == 2) // Verificar si el nombre es "bonito"
		{
			if ((nombreCliente[0] == 'M') || (nombreCliente[0] == 'S'))
				sprintf(respuesta, "2/%d/SI", idSolicitud);
			else
				sprintf(respuesta, "2/%d/NO", idSolicitud);
		}
		else if (codigoOperacion == 3) // Determinar si es alto o bajo
		{
			token = strtok(NULL, "/");
			float altura = atof(token);
			if (altura > 1.70)
				sprintf(respuesta, "3/%d/%s: eres alto", idSolicitud, nombreCliente);
			else
				sprintf(respuesta, "3/%d/%s: eres bajo", idSolicitud, nombreCliente);
		}
		else if (codigoOperacion == 4) // Verificar si el nombre es un palíndromo
		{
			int len = strlen(nombreCliente);
			int esPalindromo = 1;
			for (int i = 0, j = len - 1; i < j; i++, j--)
			{
				if (tolower(nombreCliente[i]) != tolower(nombreCliente[j]))
				{
					esPalindromo = 0;
					break;
				}
			}
			
			if (esPalindromo)
				sprintf(respuesta, "4/%d/%s es un palíndromo", idSolicitud, nombreCliente);
			else
				sprintf(respuesta, "4/%d/%s no es un palíndromo", idSolicitud, nombreCliente);
		}
		else if (codigoOperacion == 5) // Convertir el nombre a mayúsculas
		{
			for (int i = 0; i < strlen(nombreCliente); i++)
			{
				nombreCliente[i] = toupper(nombreCliente[i]);
			}
			sprintf(respuesta, "5/%d/%s", idSolicitud, nombreCliente);
		}
		
		if (codigoOperacion != 0)
		{
			printf("Respuesta enviada: %s\n", respuesta);
			// Enviar respuesta al cliente
			write(cliente_sock, respuesta, strlen(respuesta));
		}
		
		// Si la solicitud es una operación válida, incrementar contador
		if (codigoOperacion >= 1 && codigoOperacion <= 5)
		{
			pthread_mutex_lock(&control_mutex); // Bloqueo para acceso seguro
			totalSolicitudes++;
			pthread_mutex_unlock(&control_mutex); // Desbloquear después de actualizar
			
			// Notificar a todos los clientes conectados sobre el contador actualizado
			char mensaje_actualizacion[20];
			sprintf(mensaje_actualizacion, "4/%d", totalSolicitudes);
			for (int j = 0; j < numClientes; j++)
				write(conexiones[j], mensaje_actualizacion, strlen(mensaje_actualizacion));
		}
	}
	// Cerrar la conexión del cliente
	close(cliente_sock);
}

int main(int argc, char *argv[])
{
	int sock_cliente, sock_servidor;
	struct sockaddr_in direccion_servidor;
	
	// INICIALIZACIONES
	// Crear socket
	if ((sock_servidor = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("Error al crear socket\n");
	
	// Configurar dirección del servidor
	memset(&direccion_servidor, 0, sizeof(direccion_servidor)); // Poner a cero
	direccion_servidor.sin_family = AF_INET;
	direccion_servidor.sin_addr.s_addr = htonl(INADDR_ANY); // Aceptar conexiones de cualquier IP
	direccion_servidor.sin_port = htons(9050); // Puerto 9050
	
	// Asignar dirección al socket
	if (bind(sock_servidor, (struct sockaddr *) &direccion_servidor, sizeof(direccion_servidor)) < 0)
		printf("Error al hacer bind\n");
	
	// Comenzar a escuchar en el puerto
	if (listen(sock_servidor, 3) < 0)
		printf("Error al escuchar en el puerto\n");
	
	totalSolicitudes = 0;
	
	pthread_t hilo_cliente;
	numClientes = 0;
	while (1)
	{
		printf("Esperando conexiones...\n");
		
		// Aceptar conexiones entrantes
		sock_cliente = accept(sock_servidor, NULL, NULL);
		printf("Conexión recibida\n");
		
		conexiones[numClientes] = sock_cliente;
		
		// Crear hilo para manejar la conexión del cliente
		pthread_create(&hilo_cliente, NULL, GestionarCliente, &conexiones[numClientes]);
		numClientes++;
	}
}

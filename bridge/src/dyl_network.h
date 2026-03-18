#ifndef DYL_NETWORK_H
#define DYL_NETWORK_H
//crossplatform headers
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define PORT_BUFFER 64
#define MAX_CONNECTIONS 3

typedef enum
{
	SOCK_NIL,
	SOCK_CLIENT,
	SOCK_SERVER,

}Socket_Type;

typedef enum
{
	PROTOCOL_TCP,
	PROTOCOL_UDP,
	PROTOCOL_PGM,
}Protocol_Type;



#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h> 

//create an object for connected_socket
typedef struct
{
	size_t id;
	SOCKET socket;
	Socket_Type type;
}Connected_Socket;

typedef struct Dyl_Network{

	WSADATA wsa_data;
	struct addrinfo* result;
	struct addrinfo* ptr;
	struct addrinfo hints;
	Socket_Type socket_type;
	Protocol_Type protocol_type;
	SOCKET d_socket;
	Connected_Socket connected_socket[MAX_CONNECTIONS];
	SOCKET client_socket;
	char* name;
	char port_buffer[PORT_BUFFER];
	int connections;
	int (*dyl_network_send)(struct Dyl_Network*, const void*, size_t size);
	int (*dyl_network_recv)(struct Dyl_Network*, const void*, size_t size);

	


}Dyl_Network;



#else

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>




typedef struct
{
    Socket_Type socket_type;
	Protocol_Type protocol_type;
    char* name;
	//char port_buffer[PORT_BUFFER];
	unsigned int port;
	int connections;
	int sockfd;
	int new_socket;
	int addrlen;
	struct sockaddr_in server_address;


}Dyl_Network;




#endif





void dyl_network_initialize(Dyl_Network* network, Socket_Type type, Protocol_Type protocol_type);
void dyl_set_port(Dyl_Network* network, char* port);
void dyl_set_name(Dyl_Network* network, char* name);
void dyl_create_socket(Dyl_Network* network, char* value);
void dyl_network_bind_socket(Dyl_Network* server);
void dyl_network_listen_socket(Dyl_Network* server);
void dyl_connect_socket(Dyl_Network* network);
int dyl_network_send(Dyl_Network* network, const void* buffer, size_t size);
void dyl_network_recv(Dyl_Network* network);

#endif



#ifdef DYL_NETWORK_IMPLEMENTATION

//NOTE:: Need more checks and failsafe control flow
void dyl_network_initialize(Dyl_Network* network, Socket_Type socket_type, Protocol_Type protocol_type)
{
	assert(network);

	#ifdef _WIN32

	//WINDOWS IMPLEMENTATION
	int check = 0;
	check = WSAStartup(MAKEWORD(2,2), &network->wsa_data);
	if(check != 0) {
		fprintf(stderr, "Winsock has failed to initialize: %s\n", WSAGetLastError());
		return;
	}
	network->result = NULL;
	network->ptr = NULL;
	network->connections = 0;
	memset(network->connected_socket, 0, sizeof(Connected_Socket) * MAX_CONNECTIONS);

	#else 
	//POSIXS IMPLEMENTATION
	memset(&network->server_address, 0, sizeof(struct sockaddr_in));
	network->sockfd = 0;
	network->connections = 0; 
	network->addrlen = sizeof(network->server_address);
	printf("Hello im the goat\n");
    #endif

	network->socket_type = socket_type;
}
void dyl_set_port(Dyl_Network* network, char* port)
{

	if(!network ||!port || strlen(port) >= PORT_BUFFER)
		return;
	#ifdef _WIN32	
	strncpy(network->port_buffer, port, PORT_BUFFER - 1);
	network->port_buffer[PORT_BUFFER - 1] = '\0';
	#else
	network->port = atoi(port);
	#endif
}

void dyl_create_socket(Dyl_Network* network, char* value)
{
	assert(network);
	#ifdef _WIN32
	ZeroMemory(&network->hints, sizeof(network->hints));
	network->hints.ai_socktype  = SOCK_STREAM;
	switch(network->protocol_type)
	{
		case PROTOCOL_TCP:
			network->hints.ai_protocol = IPPROTO_TCP;
			break;
		case PROTOCOL_UDP:
			fprintf(stderr, "No impl\n");
			break;
		case PROTOCOL_PGM:
			fprintf(stderr, "No impl\n");
			break;
	}
	if(network->socket_type == SOCK_CLIENT)
	{

		network->hints.ai_family  = AF_UNSPEC;
		if(!value)
		{

			fprintf(stderr, "Input name for server");
			WSACleanup();
			return;
		}
		int check = getaddrinfo(value, network->port_buffer, &network->hints, &network->result);
		if(check != 0)
		{
			fprintf(stderr, "getaddrinfo failed: %d\n", check);
			WSACleanup();
			return;
		}
		network->d_socket = INVALID_SOCKET;
		network->ptr = network->result;
		network->d_socket = socket(network->ptr->ai_family, network->ptr->ai_socktype, network->ptr->ai_protocol);

		printf("Client has been initialized\n");



	}else if(network->socket_type == SOCK_SERVER)
	{

		network->hints.ai_family  = AF_INET;
		network->hints.ai_flags = AI_PASSIVE;
		int check = getaddrinfo(NULL, network->port_buffer, &network->hints, &network->result);
		if(check != 0)
		{
			fprintf(stderr, "getaddrinfo failed: %d\n", check);
			WSACleanup();
			return;
		}
		network->d_socket = INVALID_SOCKET;
		network->ptr = network->result;
		network->d_socket = socket(network->ptr->ai_family, network->ptr->ai_socktype, network->ptr->ai_protocol);
		if(network->d_socket == INVALID_SOCKET)
		{
			fprintf(stderr, "Unable to initialize listening socket: %ld\n", WSAGetLastError());
			FreeAddrInfo(network->result);
			WSACleanup();
		}
		printf("Server has been initialized\n");
	}
	#else
	if(network->socket_type == SOCK_CLIENT)
	{
		network->sockfd = socket(AF_INET, SOCK_STREAM, 0);
		printf("Socket value: %d\n", network->sockfd);
		network->server_address.sin_family = AF_INET;
		printf("Socket buffer converted to int: %d\n", network->port);
		network->server_address.sin_port = htons(network->port);
		printf("Address: %s\n", value);
		if(inet_aton(value, &network->server_address.sin_addr) == 0)
		{
				fprintf(stderr, "Network address is not valid\n");
				close(network->sockfd);
				return;
		}


	} else if(network->socket_type == SOCK_SERVER)
	{
		if((network->sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
		{
			fprintf(stderr, "Unable to initialize server socket\n");
			return;
		}
		int opt = 1;
		if(setsockopt(network->sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,sizeof(opt)))
		{
			fprintf(stderr, "Unable to setup server options");
			return;
		}
		network->server_address.sin_family = AF_INET;
		network->server_address.sin_addr.s_addr = INADDR_ANY;
		network->server_address.sin_port = htons(network->port);


	}


    #endif

}

void dyl_network_bind_socket(Dyl_Network* server)
{
	assert(server && server->socket_type == SOCK_SERVER);
	#ifdef _WIN32
	int check = bind(server->d_socket, server->result->ai_addr, (int)server->result->ai_addrlen);

	if(check == SOCKET_ERROR)
	{
		FreeAddrInfo(server->result);
		closesocket(server->d_socket);
		WSACleanup();
		return;
	}
	#else
	int check = bind(server->sockfd, (struct sockaddr*)&server->server_address, sizeof(server->server_address));
	if(check < 0)
	{
		fprintf(stderr, "Failed to bind socket\n");
		return;
	}

	#endif
}

void dyl_network_listen_socket(Dyl_Network* server)
{
	assert(server && server->socket_type == SOCK_SERVER);
	#ifdef _WIN32
	if(listen(server->d_socket, MAX_CONNECTIONS) == SOCKET_ERROR)
	{
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(server->d_socket);
		WSACleanup();
	}
	printf("Listening for connections\n");
	#else
	if(listen(server->sockfd, 3) < 0)
	{
		fprintf(stderr, "Failed to listen on server socket\n");
		return;
	}
	printf("Listening for connections\n");
	#endif

}



void dyl_connect_socket(Dyl_Network* network)
{
	assert(network);
	#ifdef _WIN32
	struct addrinfo* ptr;
	assert(network->result);

	SOCKET sock = INVALID_SOCKET;
	switch(network->socket_type)
	{
		case SOCK_CLIENT:
			for(ptr = network->result; ptr != NULL; ptr = ptr->ai_next)
			{
				sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
				if(sock == INVALID_SOCKET) continue;

				if(connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen) != SOCKET_ERROR) 
					break; 

				 closesocket(sock);
				 sock = INVALID_SOCKET;
			}

			if(sock == INVALID_SOCKET)
			{
				fprintf(stderr, "Unable to connect to server\n");
				WSACleanup();
				return;
			}
			network->d_socket = sock;
			FreeAddrInfo(network->result);

			break;
		case SOCK_SERVER:
			int seconds_to_connect = 60;
			for(size_t i = 0; i < MAX_CONNECTIONS; ++i)
			{
				if(network->connected_socket[i].type == SOCK_NIL)
				{
		
					network->connected_socket[i].socket = accept(network->d_socket, NULL,NULL);
					if(network->connected_socket[i].socket == INVALID_SOCKET)
					{
						printf("Accept failed: %ld\n", WSAGetLastError());
						closesocket(network->d_socket);
						WSACleanup();
						return;
					}
					network->connected_socket[i].id = i;
					network->connected_socket[i].type = SOCK_CLIENT;
					network->connections++;
					break;
				}
			}
			printf("Accepted client socket\n");
			printf("Current connections: %d\n", network->connections);
			break;
	}

		#else
		switch(network->socket_type)
		{
				case SOCK_CLIENT:
						if(connect(network->sockfd, (struct sockaddr* )&network->server_address, sizeof(network->server_address)) == -1)
						{
								fprintf(stderr, "Unable to connect to server\n");
								close(network->sockfd);
								return;
						}
				break;
				case SOCK_SERVER:
					printf("Meow\n");
					if((network->new_socket = accept(network->sockfd, (struct sockaddr*)&network->server_address, (socklen_t*)&network->addrlen)) < 0)
					{
								fprintf(stderr, "Unable to connect to client\n");
								close(network->sockfd);
								return;
					}

				printf("Accepted client socket\n");
				break;
		}
		
		#endif

}

//EXAMPLES
#define DEFAULT_BUFFER_LEN 512
int dyl_network_send(Dyl_Network* network, const void* buffer, size_t size)
{
	assert(network);
	

	int success = 0;

	#ifdef _WIN32
	switch(network->socket_type)
	{
		
		case SOCK_CLIENT:
			int result = send(network->d_socket, (const char*)buffer, size, 0);
			if(result == SOCKET_ERROR)
			{
				printf("Send failed: %d\n", WSAGetLastError());
				closesocket(network->d_socket);
				WSACleanup();
				return success;
			}
			printf("Bytes sent: %ld\n", result);
			/*result = shutdown(network->d_socket, SD_SEND);
			if(result == SOCKET_ERROR)
			{
				printf("shutdown failed: %d\n", WSAGetLastError());
				closesocket(network->d_socket);
				WSACleanup();
				return success;
			}
			printf("Client has shutdown\n");*/
			success = 1;
			
		break;
		case SOCK_SERVER:

		break;
		default:
			fprintf(stderr, "Invalid socket\n");
			return success;
	}
	#endif
	return success;
}
void dyl_network_recv(Dyl_Network* network)
{
	assert(network);


	#ifdef _WIN32
	int result, send_result = 0;
	char recv_buffer[DEFAULT_BUFFER_LEN];
	if(network->socket_type == SOCK_SERVER)
	{


		do 
		{
			for(size_t i = 0; i < MAX_CONNECTIONS; ++i)
			{
				if(network->connected_socket[i].type == SOCK_CLIENT)
				{

					result = recv(network->connected_socket[i].socket, recv_buffer, DEFAULT_BUFFER_LEN, 0);
					if(result > 0)
					{
						printf("Bytes received:%d\n", result);
						char message[DEFAULT_BUFFER_LEN];
						snprintf(message, DEFAULT_BUFFER_LEN, "Client %d -> %s", network->connected_socket[i].id, recv_buffer);
						printf("[SERVER LOG]: %s\n", message);
						send_result = send(network->connected_socket[i].socket, message, DEFAULT_BUFFER_LEN, 0);
						if(send_result == SOCKET_ERROR)
						{
							printf("send failed: %d\n", WSAGetLastError());
							closesocket(network->d_socket);
							WSACleanup();
							return;
						}
						printf("Bytes sent: %d\n", send_result);

					} else if(result == 0){
						printf("Connection closing...\n");
					} else{
						printf("recv failed: %d\n", WSAGetLastError());
						WSACleanup();
						return;
					}
			

				}

			}
		}while(result > 0);
	} else if(network->socket_type == SOCK_CLIENT)
	{
		do
		{
			result = recv(network->d_socket, recv_buffer, DEFAULT_BUFFER_LEN, 0);
			if(result > 0)
			{

				printf("Bytes reveived: %d\n", result);
				printf("-> %s\n", recv_buffer);
			}
			else if(result == 0)
				printf("Connection closed\n");
			else 
				printf("recv faied: %d\n", WSAGetLastError());

		}while(result > 0);
	}

	#endif
}
//better to just do callbacks and have the user decide how he wants to send data through the ethos









#endif

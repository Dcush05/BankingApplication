#define DYL_NETWORK_IMPLEMENTATION
#include "dyl_network.h"
#include <windows.h>
#include <errhandlingapi.h>
#include <handleapi.h>
#include <minwinbase.h>
#include <processthreadsapi.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <synchapi.h>
#include <winnt.h>
#include "cJson/cJSON.h"

#define ACCOUNT_ID_SIZE 10
#define NAME_SIZE 10
#define TRANSACTION_TYPE_SIZE 10
typedef enum
{
	TRANSACTION_WITHDRAW,
	TRANSACTION_DEPOSIT,

}Transaction_Type;

#pragma pack(push, 1)
typedef struct
{

	char account_id[ACCOUNT_ID_SIZE];
	char name[NAME_SIZE];
	char transaction_str[TRANSACTION_TYPE_SIZE];
	double amount;
	double result_balance;
	Transaction_Type type;
	int status;
	
}Comma_Area;
#pragma pack(pop)
HANDLE gh_cobol_mutex;

typedef void (__stdcall *COBOL_PROC)(Comma_Area*);

int accept_socket(Dyl_Network* network)
{
	network->client_socket = accept(network->d_socket, NULL, NULL);
	printf("Waiting for client to connect\n");
  if (network->client_socket == INVALID_SOCKET) {
        wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
        closesocket(network->d_socket);
        WSACleanup();
        return 0;
    } else
        wprintf(L"Client connected.\n");
	
  return 1;
}

int network_recv(Dyl_Network* network, const void* data, size_t size)
{
	int i_result = recv(network->client_socket, (char*)data, size, 0);
	if (i_result > 0)
	{
		printf("Bytes received: %d\n", i_result);
	}
	else if (i_result == 0)
	{

		printf("Connection closed\n");
	}
	else
		printf("recv failed: %d\n", WSAGetLastError());
	
	return i_result;
			
}

#define BUFFER_SIZE 1024





typedef struct
{
	Dyl_Network network;
	char json_request[BUFFER_SIZE];

}thread_func_params;

DWORD WINAPI handle_connection(void* data){

	thread_func_params* params = (thread_func_params*)data;

	int bytes = params->network.dyl_network_recv(&params->network, params->json_request, BUFFER_SIZE);
	if(bytes > 0)
	{
		printf("JSON REQUEST LOOKS LIKE THIS: %s", params->json_request);

		//parse the json and send it to the cobol program

		Comma_Area my_data;
		cJSON *root = cJSON_Parse(params->json_request);
		if (root == NULL) {
			const char *error_ptr = cJSON_GetErrorPtr();
			if (error_ptr != NULL) 
			{
				fprintf(stderr, "Error before: %s\n", error_ptr);
			}
			return 1;
		}
		
		cJSON *account_id = cJSON_GetObjectItemCaseSensitive(root, "Account-ID");
		cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "Name");
		cJSON *transaction_type = cJSON_GetObjectItemCaseSensitive(root, "Type");
		cJSON *amount = cJSON_GetObjectItemCaseSensitive(root, "Amount");


		if (account_id) strncpy(my_data.account_id, account_id->valuestring, ACCOUNT_ID_SIZE);
		if (name) strncpy(my_data.name, name->valuestring, NAME_SIZE);
		if (transaction_type) strncpy(my_data.transaction_str, transaction_type->valuestring, TRANSACTION_TYPE_SIZE);
		if (amount) my_data.amount = amount->valuedouble;


		//Debug printing
		printf("Account id: %s\n", my_data.account_id);
		printf("Name: %s\n", my_data.name);
		printf("Transaction str: %s\n", my_data.transaction_str);
		printf("Amount: %f\n", my_data.amount);
				

		HINSTANCE h_get_proc_dll = LoadLibrary("../cobol-core/account-proc.dll");
		if(!h_get_proc_dll)
		{
			printf("Failure to load account-proc.dll\n");
			return 1;
		}

		COBOL_PROC account_proc = (COBOL_PROC)GetProcAddress(h_get_proc_dll,
		"ACCOUNT");

		if(!account_proc)
		{
			printf("Could not find account proc func");
			return 1;
		}
		DWORD dwWaitResult = WaitForSingleObject(gh_cobol_mutex, INFINITE);

		switch (dwWaitResult) 
		{
			case WAIT_OBJECT_0: 
			account_proc(&my_data); 
		
			if (!ReleaseMutex(gh_cobol_mutex)) {
				printf("ReleaseMutex error\n");
			}
			break; 

			case WAIT_ABANDONED: 
				return 1; 
		}

		FreeLibrary(h_get_proc_dll);

		printf("Transaction Status: %d\n", my_data.status);
		printf("New Balance: %.2f\n", my_data.result_balance);

		cJSON *response = cJSON_CreateObject();
		cJSON_AddNumberToObject(response, "Status", my_data.status);
		cJSON_AddNumberToObject(response, "Balance", my_data.result_balance);
		char *out = cJSON_PrintUnformatted(response);
		strcat(out, "\n");
		printf("%s", out);
		int check = send(params->network.client_socket, out, strlen(out), 0);
		
		free(out);
		cJSON_Delete(root);
			
	}
	free(params);
	return 0;
	
}


int main()
{
	//Mutex for the cobol proc

	gh_cobol_mutex = CreateMutex(NULL, FALSE, NULL);
    if (gh_cobol_mutex == NULL) {
        printf("CreateMutex error: %d\n", (int)GetLastError());
        return 1;
    }
	//We recv data from the socket
	Dyl_Network network;
	dyl_network_initialize(&network,SOCK_SERVER, PROTOCOL_TCP);
	dyl_set_port(&network, "8080");
	dyl_create_socket(&network, "127.1.0.0");
	dyl_network_bind_socket(&network);
	dyl_network_listen_socket(&network);
	network.dyl_network_recv = network_recv;


	

	while(1)
	{
		printf("Waiting for connections :p\n");
		int sock_success = accept_socket(&network);
		if(sock_success)
		{
			
			thread_func_params* thread_args = malloc(sizeof(thread_func_params));
			thread_args->network = network;
			DWORD thread_id;
			HANDLE thread = CreateThread(NULL, 0 ,handle_connection, thread_args, 0,  &thread_id);
			Sleep(1000);
			WaitForSingleObject(thread, INFINITE);
			CloseHandle(thread);
			
		}
	}


	return 0;
}
	
	

	
	

	


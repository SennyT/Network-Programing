#include <WinSock2.h>
#include <stdio.h>
#include <WS2tcpip.h>
#include <string.h>
#include <cstdio>


using namespace std;

#pragma comment(lib, "Ws2_32.lib")

#define BUFF_SIZE 2048

int main(int argc, char *argv[]) {

	// Initiate address
	char* serverIP = argv[1];

	int serverPortNumber = 0;
	for (int i = 0; i < (int)strlen(argv[2]); i++) {
		serverPortNumber = serverPortNumber * 10 + (argv[2][i] - '0');
	}

	//Start client
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported.\n");
	printf("Client started!\n");

	//Set up socket, server address
	SOCKET client;

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPortNumber);
	inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

	int ret, serverAddrLen = sizeof(serverAddr);
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);



	if (connect(client, (sockaddr *)&serverAddr, sizeof(serverAddr))) {
		printf("Error! Cannot connect server.");
	}
	//Start function
	int accountLoggedIn = 0;
	char name[30];
	printf("Welcome!\n");

	while (1) {

		//menu
		printf("1. login\n");
		printf("2. post\n");
		printf("3. logout\n");
		printf("Enter number: ");
		int inp;
		scanf("%d", &inp);

		char sendbuff[BUFF_SIZE], recvbuff[BUFF_SIZE];
		switch (inp)
		{
		case 1:
			printf("User name: ");
			scanf("%s", &name);

			strcpy(sendbuff, "USER ");
			strcat(sendbuff, name);
			strcat(sendbuff, ";");


			ret = send(client, sendbuff, strlen(sendbuff), 0);
			if (ret == SOCKET_ERROR) {
				printf("Cannot send message.");
			}
			ret = recv(client, recvbuff, BUFF_SIZE, 0);

			if (ret == SOCKET_ERROR) {
				printf("Cannot recieve message.");
			}

			recvbuff[ret] = 0;

			if (recvbuff[0] == '1') {

				int ans = (recvbuff[1] - '0');

				switch (ans)
				{
				case 0:
					accountLoggedIn = 1;
					printf("Account is logged in.");
					break;
				case 1:
					printf("Account is locked.");
					break;
				case 2:
					printf("Account does not exist.");
					break;
				case 3:
					printf("Client already logged in an account.");
					break;
				default:
					break;
				}
				printf("\n");
			}
			if (recvbuff[0] == '9') {
				if (recvbuff[1] == '9') {
					printf("Unknown message.\n");
					break;
				}
			}
			break;


		case 2:
			char post[BUFF_SIZE];
			printf("Post: "); scanf("%s", post);
			strcpy(sendbuff, "POST ");
			strcat(sendbuff, post);
			strcat(sendbuff, ";");


			ret = send(client, sendbuff, strlen(sendbuff), 0);
			if (ret == SOCKET_ERROR) {
				printf("Cannot send message.\n");
			}
			ret = recv(client, recvbuff, BUFF_SIZE, 0);

			if (ret == SOCKET_ERROR) {
				printf("Cannot recieve message.\n");
			}
			recvbuff[ret] = 0;
			if (recvbuff[0] == '2') {
				int ans = (int)(recvbuff[1] - '0');
				switch (ans)
				{
				case 0:
					printf("Posted succesfully.");
					break;
				case 1:
					printf("User must login before posting.");
					break;
				default:
					break;
				}
				printf("\n");
			}
			if (recvbuff[0] == '9') {
				if (recvbuff[1] == '9') {
					printf("Unknown message.\n");
					break;
				}
			}
			break;
		case 3:
			strcpy(sendbuff, "BYE");
			strcat(sendbuff, ";");


			ret = send(client, sendbuff, strlen(sendbuff), 0);
			if (ret == SOCKET_ERROR) {
				printf("Cannot send message.\n");
			}
			ret = recv(client, recvbuff, BUFF_SIZE, 0);

			if (ret == SOCKET_ERROR) {
				printf("Cannot recieve message.\n");
				break;
			}
			recvbuff[ret] = 0;
			
			if (recvbuff[0] == '2') {
				if (recvbuff[1] == '1') {
					printf("User haven't login yet.\n");
					break;
				}
			}
			if (recvbuff[0] == '3') {
				if (recvbuff[1] == '0') {
					printf("Successfully logout.\n");
					printf("Closing client.\n");
					goto close;
					break;
				}
			}
			if (recvbuff[0] == '9') {
				if (recvbuff[1] == '9') {
					printf("Unknown message.\n");
					break;
				}
			}
			break;
		default:
			break;

		}
	}
	printf("\n");
close:
	closesocket(client);
	WSACleanup();
}



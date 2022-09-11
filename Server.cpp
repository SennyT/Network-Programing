#include "stdio.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#include <string>
#include <fstream>
#include <conio.h>

#define FILE_DIRR "account.txt"
#define SERVER_PORT 5500
#define SERVER_ADDRESS "127.0.0.1"
#define BUFF_SIZE 2048
#define MAX_ACCOUNT 4096
#pragma comment(lib, "Ws2_32.lib")

using namespace std;

struct ACCOUNT_DATA
{
	char name[30];
	int available;
};

struct ACCOUNT
{
	int loginState;
	char name[BUFF_SIZE];
	char buff[BUFF_SIZE];
	SOCKET connSock = NULL;
};

struct ACCOUNT_DATA accountData[MAX_ACCOUNT];
int accountNumber = 0;

struct ACCOUNT session[WSA_MAXIMUM_WAIT_EVENTS];

WSAEVENT	events[WSA_MAXIMUM_WAIT_EVENTS];
DWORD		sessionNumber = 0;
DWORD		index;
WSANETWORKEVENTS sockEvent;

void InitData() {
	string line;
	ifstream file(FILE_DIRR);

	while (getline(file, line)) {
		int i = 0;
		char tempName[30];

		while (line[i] != ' ') {
			tempName[i] = line[i];
			i++;

		}
		tempName[i] = 0;
		strcpy(accountData[accountNumber].name, tempName);

		i++;
		if (line[i] == '1') {
			accountData[accountNumber].available = 1;
		}
		else {
			accountData[accountNumber].available = 0;
		}

		accountNumber++;
	}
	file.close();
}

int CheckLogin(ACCOUNT &account) {
	if (account.loginState == 0) return 1;
	return 0;
}

int login(ACCOUNT &account, char* username) {
	//checking name and availability of account, checking login state of session
	printf("Client logging in\n");
	if (CheckLogin(account)==0) {
		printf("Client already logged in\n");
		return 13;
	}
	for (int i = 0; i < accountNumber; i++) {
		if (strcmp(username, accountData[i].name) == 0) {

			if (accountData[i].available == 0) {
				//init state of session
				account.loginState = 1;
				strcpy(account.name, username);
				printf("Client logged in\n");
				return 10;
			}
			else {
				printf("Client unavailable\n");
				return 11;
			}
		}
	}
	return 12;
}

int Post(ACCOUNT &account, char* post) {
	if (CheckLogin(account)) return 21;
	return 20;
}

int Disconnect(ACCOUNT &account, int index) {
	
	closesocket(account.connSock);
	WSACloseEvent(events[index]);
	printf("%d Disconnected\n", index);
	
	sessionNumber--;

	for (int i = index; i < sessionNumber; i++) {
		session[i] = session[i + 1];
		events[i] = events[i + 1];
	}
	events[sessionNumber] = 0;
	session[sessionNumber].connSock = 0;
	session[sessionNumber].buff[0] = 0;
	session[sessionNumber].loginState = 0;
	session[sessionNumber].name[0] = 0;
	printf("putting the end to 0: %d\n", sessionNumber);

	printf("Client disconnected. Current session: %d\n",sessionNumber - 1);
	return 0;
}

int receive(ACCOUNT &account) {
	//recv
	int ret;
	char recvbuff[BUFF_SIZE];
	ret = recv(account.connSock, recvbuff, BUFF_SIZE, 0);

	if (ret == SOCKET_ERROR) return 0;
	if (ret == 0) return 0;

	//store in server's buffer
	recvbuff[ret] = 0;


	if (strlen(account.buff) != 0)
		strcat(account.buff, recvbuff);
	else strcpy(account.buff, (char*)recvbuff);

	return ret;

}

int logout(ACCOUNT &account) {
	if (CheckLogin(account)) return 21;
	else {
		account.name[0] = 0;
		account.loginState = 0;
		return 30;
	}
}

int process(ACCOUNT &account, int index) {
	//Process the server buffer
	//Return code if successfully processed a request.
	//Return 0 if request is not completely read.
	char reader[5]; //request reader
	strcpy(reader, "\0");
	int ret, isProcess = 0;

	
	char command[BUFF_SIZE];
	strcpy(command, account.buff);
	for (int i = 0; i < strlen(account.buff); i++) {
		if (command[i] == ';') {
			command[i + 1] = 0;
			memmove(account.buff, account.buff + i + 1, (strlen(account.buff) - i));
			isProcess = 1;
			break;
		}
	}
	if (isProcess == 0) return 0;

	if (strlen(command) >= 4) {
		reader[0] = command[0];
		reader[1] = command[1];
		reader[2] = command[2];
		reader[3] = command[3];
		reader[4] = 0;
		printf("Command recieved >=4: %s\n",command);
	}
	else return 99;

	if (strcmp(reader, "USER") == 0) {
		int i = 5;
		char username[BUFF_SIZE];

		strcpy(username, "\0");
		while (command[i] != ';') {
			char tmp[2];
			tmp[0] = command[i];
			tmp[1] = 0;
			strcat(username, tmp);
			i++;
		}

		return login(account, username);
	}
	
	if (strcmp(reader, "POST") == 0) {
		int i = 5;
		char post[BUFF_SIZE];

		strcpy(post, "\0");
		while (command[i] != ';') {
			char tmp[2];
			tmp[0] = command[i];
			tmp[1] = 0;
			strcat(post, tmp);
			i++;
		}

		return Post(account, post);
	}

	if (strcmp(reader, "BYE;") == 0) {
		return logout(account);
	}

	return 99;
}

void respond(ACCOUNT &account, int message) {
	//send
	char sendbuff[BUFF_SIZE];
	string tmp = to_string(message);
	strcpy(sendbuff, tmp.c_str());
	strcat(sendbuff, ";");
	printf("Sending: ""%s""\n", sendbuff);
	send(account.connSock, sendbuff, strlen(sendbuff), 0);

	return;
}


int main(int argc, char* argv[])
{
	//Initiate address
	int serverPortNumber = 0;
	for (int i = 0; i < (int)strlen(argv[1]); i++) {
		serverPortNumber = serverPortNumber * 10 + (argv[1][i] - '0');
	}

	//Inittiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
	{
		printf("WinSockk 2.2 is not supported.");
		return 0;
	}

	//Construct Socket
	SOCKET listenSock;
	SOCKET connSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET)
	{
		printf("ERROR %d: Cannot creat server socket.", WSAGetLastError());
		return 0;
	}

	//Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDRESS, &serverAddr.sin_addr);

	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		printf("ERROR %d: Cannot bind this address.", WSAGetLastError());

		return 0;
	}

	//pre-set IO Mode
	events[0] = WSACreateEvent(); //create new events
	sessionNumber++;
	WSAEventSelect(listenSock, events[0], FD_ACCEPT | FD_CLOSE);

	for (int i = 1; i < WSA_MAXIMUM_WAIT_EVENTS; i++) {
		session[i].connSock = 0;
	}

	//Listen for request
	if (listen(listenSock, 100))
	{
		printf("Error %d: Can not place server socket into state LISTEN.", WSAGetLastError());
		return 0;
	}

	printf("Server started.");

	sockaddr_in clientAddr;
	int ret, clientAddrLen;
	
	InitData();

	//Communicate with client
	while (1) {
		//wait for network events on all socket
		printf("Sockets awaiting: %d\n", sessionNumber);
		index = WSAWaitForMultipleEvents(sessionNumber, events, FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED) {
			printf("Error %d: WSAWaitForMultipleEvents() failed\n", WSAGetLastError());
			break;
		}

		index = index - WSA_WAIT_EVENT_0;
		if (index == 0) {
			WSAEnumNetworkEvents(listenSock, events[0], &sockEvent);
			//reset event
			WSAResetEvent(events[index]);
		}
		else {
			WSAEnumNetworkEvents(session[index].connSock, events[index], &sockEvent);
			//reset event
			WSAResetEvent(events[index]);
		}
		
		//FD_ACCEPT
		if (sockEvent.lNetworkEvents & FD_ACCEPT) {
			//Add new socket into socks array
			clientAddrLen = sizeof(clientAddr);
			printf("Accepting connection.");
			if ((connSock = accept(listenSock, (sockaddr *)&clientAddr, &clientAddrLen)) < 0) {
				printf("\nError! Cannot accept new connection: %d", WSAGetLastError());
				break;
			}
			else {
				int i;
				printf("Accepted connection.\n");
				if (sessionNumber == WSA_MAXIMUM_WAIT_EVENTS) {
					printf("\nToo many clients.");
					closesocket(connSock);
				}
				else {
					for (i = 1; i < WSA_MAXIMUM_WAIT_EVENTS; i++)
						if (session[i].connSock == 0) {
							printf("Connecting into sessions.");
							session[i].connSock = connSock;
							events[i] = WSACreateEvent();
							WSAEventSelect(session[i].connSock, events[i], FD_READ | FD_CLOSE);
							sessionNumber++;
							
							printf("Connected into sessions.\n");
							break;
						}
						
				}
			}
		
			
		}
			
		//FD_READ
		if (sockEvent.lNetworkEvents & FD_READ) {
			//Receive message from client
			if (sockEvent.iErrorCode[FD_READ_BIT] != 0) {
				printf("FD_READ failed with error %d\n", sockEvent.iErrorCode[FD_READ_BIT]);
				break;
			}

			ret = receive(session[index]);

			//Release socket and event if an error occurs
			
			if (ret == 0) {
				WSAResetEvent(events[index]);
				Disconnect(session[index], index);
				continue;
			}
			
		}
			
		//process and respond to client if there is command in buffer
		for (int i = 1; i <= sessionNumber; i++) {
			int ret = strlen(session[i].buff);
			while (ret) {
				ret = process(session[i], i);
				printf("ret = %d\n", ret);
				if (ret != 0) respond(session[i], ret);
				
			}
		}

		//FD_CLOSE
		if (sockEvent.lNetworkEvents & FD_CLOSE) {
			//Release socket and event
			
			Disconnect(session[index], index);
		}

		
	}
	//end communicating

	//Close Socket
	closesocket(listenSock);
	//Terminate Winsock
	WSACleanup();

	return 0;
}
#ifdef _IA64_
#pragma warning (disable: 4311)
#pragma warning (disable: 4312)
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <mstcpip.h>
#include <stdio.h>
#include <String>
#include <iostream>
#include <random>
#include <ctime>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#define DEFAULT_PORT 5223
#define DEFAULT_BUFLEN 512
#define CLOSESOCK(s) \
        if(INVALID_SOCKET != s) {closesocket(s); s = INVALID_SOCKET;}
#define ERR(e) \
        printf("%s:%s failed: %d [%s@%ld]\n",__FUNCTION__,e,WSAGetLastError(),__FILE__,__LINE__)
#define DEFAULT_WAIT    30000
#define WS_VER          0x0202

using namespace std;

int main(int argc, char* argv[])
{
	/*WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;
	srand(time(0));
	int max_length = 1 + rand() % 20;
	char *sendbuf = new char[max_length];
	for (int i(0); i < max_length; i++)
		sendbuf[i] = 32 + rand()%125;
	printf(sendbuf);
	printf("\n");
	// Validate the parameters
	if (argc != 2) {
		printf("usage: %s server-name\n", argv[0]);
		return 1;
	}
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	//Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	//Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
	// Create a SOCKET for connecting to server
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
							ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}
	// Connect to server.
	iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	freeaddrinfo(result);
	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}
	int sleep1 = 2000 + rand() % 8000;
	Sleep(sleep1);
	//Send an initial buffer
	iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}
	printf("Bytes Sent: %ld\n", iResult);
	//shutdown the connection since no more data will be sent
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}
	//Receive until the peer closes the connection
	do {

		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0)
				printf("Bytes received: %d\n", iResult);
			else if (iResult == 0)
				printf("Connection closed\n");
			else
				printf("recv failed with error: %d\n", WSAGetLastError());

	} while (iResult > 0);
	int sleep2 = 2000 + rand() % 8000;
	Sleep(sleep2);
	iResult = shutdown(ConnectSocket, SD_RECEIVE);
	if (iResult == SOCKET_ERROR)
	{
		printf("shutdown receive failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}
	printf("close socket\n");
	//cleanup
	closesocket(ConnectSocket);
	WSACleanup();
	system("pause");*/
	WSAPOLLFD fdarray = { 0 };
	WSADATA wsaData;
	SOCKET csock = INVALID_SOCKET;
	//SOCKADDR_STORAGE addrLoopback = { 0 };
	SOCKADDR_IN client;
	INT ret = 0;
	ULONG uNonBlockingMode = 1;
	CHAR buf[MAX_PATH] = { 0 };
	__try
	{
		int  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			printf("WSAStartup failed with error: %d\n", iResult);
			return 1;
		}
		if (INVALID_SOCKET == (csock = socket(AF_INET, SOCK_STREAM, 0)))
		{
			ERR("socket");
			__leave;
		}
		if (SOCKET_ERROR == ioctlsocket(csock, FIONBIO, &uNonBlockingMode))
		{
			ERR("FIONBIO");
			__leave;
		}
		srand(time(0));
		int max_length = 1 + rand() % 20;
	
		char *sendbuf = new char[max_length];
		for (int i(0); i < max_length; i++)
			sendbuf[i] = 32 + rand()%125;
		/*addrLoopback.ss_family = AF_INET;
		addrLoopback.__ss_align = ADDR_ANY;
		INETADDR_SETLOOPBACK((SOCKADDR*)&addrLoopback);
		SS_PORT((SOCKADDR*)&addrLoopback) = htons(DEFAULT_PORT);*/
		client.sin_family = AF_INET;
		client.sin_addr.s_addr = INADDR_ANY;
		client.sin_port = htons(DEFAULT_PORT);
		if (SOCKET_ERROR == connect(csock, (SOCKADDR*)&client, sizeof(client)))
		{
			if (WSAEWOULDBLOCK != WSAGetLastError())
			{
				ERR("connect");
				__leave;
			}
		}
		//Call WSAPoll for writeability on connecting socket
		fdarray.fd = csock;
		fdarray.events = POLLWRNORM;
		if (SOCKET_ERROR == (ret = WSAPoll(&fdarray, 1, DEFAULT_WAIT)))
		{
			ERR("WSAPoll");
			__leave;
		}
		if (ret)
		{
			if (fdarray.revents & POLLWRNORM)
			{
				printf("ConnectThread: Established connection\n");
				//Send data
				cout << sendbuf << endl;
				if (SOCKET_ERROR == (ret = send(csock, sendbuf, sizeof(sendbuf), 0)))
				{
					ERR("send");
					__leave;
				}
				else
					printf("ConnectThread: sent %d bytes\n", ret);
				iResult = shutdown(csock, SD_SEND);
				if (iResult == SOCKET_ERROR) {
					printf("shutdown send failed with error: %d\n", WSAGetLastError());
					closesocket(csock);
					WSACleanup();
					__leave;
				}
			}
		}
		//Call WSAPoll for readability on connected socket
		fdarray.events = POLLRDNORM;
		if (SOCKET_ERROR == (ret = WSAPoll(&fdarray, 1, DEFAULT_WAIT)))
		{
			ERR("WSAPoll");
			__leave;
		}
		if (ret)
		{
			if (fdarray.revents & POLLRDNORM)
			{	
				if (SOCKET_ERROR == (ret = recv(csock, buf, sizeof(buf), 0)))
				{
					ERR("recv");
					__leave;
				}
				else
					printf("ConnectThread: recvd %d bytes\n", ret);
			}
		}
	}
	__finally
	{
		CLOSESOCK(csock);
	}
	return 0;
}
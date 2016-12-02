#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "5223"
#define DEFAULT_BUFLEN 512
char recvbuf[DEFAULT_BUFLEN];
int iSendResult;
int recvbuflen = DEFAULT_BUFLEN;

int main()
{
	WSADATA wsadata;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsadata);
	if (iResult)
		printf("WSAStartup failed: %d\n", iResult);
	struct addrinfo *pResult = NULL, *ptr = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &pResult);
	if (iResult)
	{
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(pResult->ai_family, pResult->ai_socktype, pResult->ai_protocol);
	if (ListenSocket == INVALID_SOCKET)
	{
		printf("socket() failed: %d/n", WSAGetLastError());
		freeaddrinfo(pResult);
		WSACleanup();
	}
	iResult = bind(ListenSocket, pResult->ai_addr, (int)pResult->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error : %d\n", WSAGetLastError());
		freeaddrinfo(pResult);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	else freeaddrinfo(pResult);
	
	if (listen(ListenSocket, SOMAXCONN)==SOCKET_ERROR)
	{
		printf("Listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	SOCKET ClientSocket = INVALID_SOCKET;
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET)
	{
		printf("accept failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	do
	{
		iResult = recv(ClientSocket, recvbuf, DEFAULT_BUFLEN, 0);
		if(iResult > 0)
		{
			printf("Bytes recieved: %d\n", iResult);
			iSendResult = send(ClientSocket, recvbuf, iResult, 0);
			if (iSendResult == SOCKET_ERROR)
			{
				printf("send failed: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}else if (iResult == 0) printf("Connection closing...\n");
		else
		{
			printf("recv failed: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
	} while (iResult > 0);
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}
	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}
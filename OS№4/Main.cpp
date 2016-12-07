#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <stdio.h>
#include <cstdlib>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "5223"
//#define DEFAULT_BUFLEN 512
//#define CLOSESOCK(s) \
//        if(INVALID_SOCKET != s) {closesocket(s); s = INVALID_SOCKET;}
//#define ERR(e) \
//        printf("%s:%s failed: %d [%s@%ld]\n",__FUNCTION__,e,WSAGetLastError(),__FILE__,__LINE__)
//#define DEFAULT_WAIT    30000
//#define WS_VER          0x0202
//char recvbuf[DEFAULT_BUFLEN];
//int iSendResult;
//int recvbuflen = DEFAULT_BUFLEN;
//HANDLE hCloseSignal = NULL;
//
//int main()
//{
	/*WSADATA wsadata;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsadata);
	if (iResult)
		printf("WSAStartup failed: %d\n", iResult);
	struct addrinfo *pResult = NULL, *ptr = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET6;
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

	return 0;*/
#define PORT 5223
#define DATA_BUFSIZE 8192

typedef struct _SOCKET_INFORMATION {
	OVERLAPPED Overlapped;
	SOCKET Socket;
	CHAR Buffer[DATA_BUFSIZE];
	WSABUF DataBuf;
	DWORD BytesSEND;
	DWORD BytesRECV;
} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;

void CALLBACK WorkerRoutine(DWORD Error, DWORD BytesTransferred, LPWSAOVERLAPPED Overlapped, DWORD InFlags);
DWORD WINAPI WorkerThread(LPVOID lpParameter);

SOCKET AcceptSocket;

int main(int argc, char **argv)
{
	WSADATA wsaData;
	SOCKET ListenSocket;
	SOCKADDR_IN InternetAddr;
	INT Ret;
	HANDLE ThreadHandle;
	DWORD ThreadId;
	PCSTR ip = "127.0.0.1";
	WSAEVENT AcceptEvent;

	if ((Ret = WSAStartup((2, 2), &wsaData)) != 0)
	{
		printf("WSAStartup() failed with error %d\n", Ret);
		WSACleanup();
		return 1;
	}
	else
		printf("WSAStartup() is OK!\n");

	/*struct addrinfo *pResult = NULL, *ptr = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = WSA_FLAG_OVERLAPPED;
	int iResult = getaddrinfo(NULL , DEFAULT_PORT, &hints, &pResult);
	if (iResult)
	{
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}*/

	if ((ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
		printf("Failed to get a socket %d\n", WSAGetLastError());
		return 1;
	}
	else
		printf("WSASocket() is pretty fine!\n");

	InternetAddr.sin_family = AF_INET;
	InternetAddr.sin_addr.s_addr = INADDR_ANY;
	//InternetAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	InternetAddr.sin_port = htons(PORT);

	if (bind(ListenSocket, (SOCKADDR*)&InternetAddr, sizeof(InternetAddr)) == SOCKET_ERROR)
	{
		printf("bind() failed with error %d\n", WSAGetLastError());
		return 1;
	}
	else
		printf("bind() is OK!\n");

	if (listen(ListenSocket, 5))
	{
		printf("listen() failed with error %d\n", WSAGetLastError());
		return 1;
	}
	else
		printf("listen() is OK!\n");

	if ((AcceptEvent = WSACreateEvent()) == WSA_INVALID_EVENT)
	{
		printf("WSACreateEvent() failed with error %d\n", WSAGetLastError());
		return 1;
	}
	else
		printf("WSACreateEvent() is OK!\n");

	// Create a worker thread to service completed I/O requests
	if ((ThreadHandle = CreateThread(NULL, 0, WorkerThread, (LPVOID)AcceptEvent, 0, &ThreadId)) == NULL)
	{
		printf("CreateThread() failed with error %d\n", GetLastError());
		return 1;
	}
	else
		printf("CreateThread() should be fine!\n");

	while (TRUE)
	{
		/*system("pause");*/
		AcceptSocket = accept(ListenSocket, NULL, NULL);

		if (WSASetEvent(AcceptEvent) == FALSE)
		{
			printf("WSASetEvent() failed with error %d\n", WSAGetLastError());
			return 1;
		}
		else
			printf("WSASetEvent() should be working!\n");
	}
	return 0;
}

DWORD WINAPI WorkerThread(LPVOID lpParameter)
{
	DWORD Flags;
	LPSOCKET_INFORMATION SocketInfo;
	WSAEVENT EventArray[1];
	DWORD Index;
	DWORD RecvBytes;

	// Save the accept event in the event array
	EventArray[0] = (WSAEVENT)lpParameter;

	while (TRUE)
	{
		// Wait for accept() to signal an event and also process WorkerRoutine() returns
		while (TRUE)
		{
			Index = WSAWaitForMultipleEvents(1, EventArray, FALSE, WSA_INFINITE, TRUE);
			if (Index == WSA_WAIT_FAILED)
			{
				printf("WSAWaitForMultipleEvents() failed with error %d\n", WSAGetLastError());
				return FALSE;
			}
			else
				printf("WSAWaitForMultipleEvents() should be OK!\n");

			if (Index != WAIT_IO_COMPLETION)
			{
				// An accept() call event is ready - break the wait loop
				break;
			}
		}

		WSAResetEvent(EventArray[Index - WSA_WAIT_EVENT_0]);
		// Create a socket information structure to associate with the accepted socket
		if ((SocketInfo = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
		{
			printf("GlobalAlloc() failed with error %d\n", GetLastError());
			return FALSE;
		}
		else
			printf("GlobalAlloc() for SOCKET_INFORMATION is OK!\n");

		// Fill in the details of our accepted socket
		SocketInfo->Socket = AcceptSocket;
		ZeroMemory(&(SocketInfo->Overlapped), sizeof(WSAOVERLAPPED));
		SocketInfo->BytesSEND = 0;
		SocketInfo->BytesRECV = 0;
		SocketInfo->DataBuf.len = DATA_BUFSIZE;
		SocketInfo->DataBuf.buf = SocketInfo->Buffer;

		Flags = 0;
		if (WSARecv(SocketInfo->Socket, &(SocketInfo->DataBuf), 1, &RecvBytes, &Flags,
			&(SocketInfo->Overlapped), WorkerRoutine) == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				printf("WSARecv() failed with error %d\n", WSAGetLastError());
				return FALSE;
			}
		}
		else
			printf("WSARecv() is OK!\n");

		printf("Socket %d got connected...\n", AcceptSocket);
	}
	return TRUE;
}


void CALLBACK WorkerRoutine(DWORD Error, DWORD BytesTransferred, LPWSAOVERLAPPED Overlapped, DWORD InFlags)
{
	DWORD SendBytes, RecvBytes;
	DWORD Flags;

	// Reference the WSAOVERLAPPED structure as a SOCKET_INFORMATION structure
	LPSOCKET_INFORMATION SI = (LPSOCKET_INFORMATION)Overlapped;

	if (Error != 0)
	{
		printf("I/O operation failed with error %d\n", Error);
	}

	if (BytesTransferred == 0)
	{
		printf("Closing socket %d\n\n", SI->Socket);
	}

	if (Error != 0 || BytesTransferred == 0)
	{
		printf("trying close socket %d\n", SI->Socket);
		closesocket(SI->Socket);
		if (SI->Socket == INVALID_SOCKET) {
			printf("Socket Free");
			GlobalFree(SI);
		}
		return;
	}

	// Check to see if the BytesRECV field equals zero. If this is so, then
	// this means a WSARecv call just completed so update the BytesRECV field
	// with the BytesTransferred value from the completed WSARecv() call
	if (SI->BytesRECV == 0)
	{
		SI->BytesRECV = BytesTransferred;
		std::cout << SI->BytesRECV << std::endl;
		SI->BytesSEND = 0;
	}
	else
	{
		SI->BytesSEND += BytesTransferred;
		std::cout << SI->BytesSEND << std::endl;
	}

	if (SI->BytesRECV > SI->BytesSEND)
	{
		// Post another WSASend() request.
		// Since WSASend() is not guaranteed to send all of the bytes requested,
		// continue posting WSASend() calls until all received bytes are sent
		ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));
		SI->DataBuf.buf = SI->Buffer + SI->BytesSEND;
		SI->DataBuf.len = SI->BytesRECV - SI->BytesSEND;

		if (WSASend(SI->Socket, &(SI->DataBuf), 1, &SendBytes, 0, &(SI->Overlapped), WorkerRoutine) == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				printf("WSASend() failed with error %d\n", WSAGetLastError());
				return;
			}
		}
		else
			printf("WSASend() is OK!\n");
	}
	else
	{
		SI->BytesRECV = 0;

		// Now that there are no more bytes to send post another WSARecv() request
		Flags = 0;
		ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));
		SI->DataBuf.len = DATA_BUFSIZE;
		SI->DataBuf.buf = SI->Buffer;

		if (WSARecv(SI->Socket, &(SI->DataBuf), 1, &RecvBytes, &Flags, &(SI->Overlapped), WorkerRoutine) == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				printf("WSARecv() failed with error %d\n", WSAGetLastError());
				return;
			}
		}
		else
			printf("WSARecv() is fine!\n");
	}
}
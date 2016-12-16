#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <String>

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "5223"
#define PORT 5223
#define DATA_BUFSIZE 8192

using namespace std;

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
BOOL WINAPI ConsoleHandler(DWORD);

SOCKET AcceptSocket;
SOCKET ListenSocket;
HANDLE hTempFile;
SOCKADDR_IN ClientAddr = { 0 };
LPSTR fileName = "Temp.txt";
std::vector<std::string> server_buffer;
int main(int argc, char **argv)
{
	WSADATA wsaData;
	SOCKADDR_IN InternetAddr;
	SOCKADDR_IN ClientAddr = {1};
	INT Ret;
	HANDLE ThreadHandle;
	DWORD ThreadId;
	WSAEVENT AcceptEvent;
	//server_buffer.reserve(MAX_PATH);
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE))
	{
		printf("Unable to install handler!\n");
		return EXIT_FAILURE;
	}
	if ((Ret = WSAStartup((2, 2), &wsaData)) != 0)
	{
		printf("WSAStartup() failed with error %d\n", Ret);
		WSACleanup();
		return 1;
	}
	else;
		//printf("WSAStartup() is OK!\n");

	if ((ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
		printf("Failed to get a socket %d\n", WSAGetLastError());
		return 1;
	}
	else;
		printf("WSASocket() is pretty fine!\n");

	InternetAddr.sin_family = AF_INET;
	InternetAddr.sin_addr.s_addr = INADDR_ANY;
	InternetAddr.sin_port = htons(PORT);

	if (bind(ListenSocket, (SOCKADDR*)&InternetAddr, sizeof(InternetAddr)) == SOCKET_ERROR)
	{
		printf("bind() failed with error %d\n", WSAGetLastError());
		return 1;
	}
	else;
		printf("bind() is OK!\n");

	if (listen(ListenSocket, 0))
	{
		printf("listen() failed with error %d\n", WSAGetLastError());
		return 1;
	}
	else;
		printf("listen() is OK!\n");

	if ((AcceptEvent = WSACreateEvent()) == WSA_INVALID_EVENT)
	{
		printf("WSACreateEvent() failed with error %d\n", WSAGetLastError());
		return 1;
	}
	else;
		//printf("WSACreateEvent() is OK!\n");

	/*if ((ThreadHandle = CreateThread(NULL, 0, WorkerThread, (LPVOID)AcceptEvent, 0, &ThreadId)) == NULL)
	{
		printf("CreateThread() failed with error %d\n", GetLastError());
		return 1;
	}
	else
		printf("CreateThread() should be fine!\n");*/
	char buffer[15];
	std::string tempStr = "";
	int size = sizeof(SOCKADDR_IN);
	while (TRUE)
	{
		AcceptSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddr, &size);
		
		if (WSASetEvent(AcceptEvent) == FALSE)
		{
			printf("WSASetEvent() failed with error %d\n", WSAGetLastError());
			return 1;
		}
		else {
			printf("accept new client %s\n", inet_ntoa(ClientAddr.sin_addr));
			snprintf(buffer, sizeof(buffer), "accept new client %s \n", inet_ntoa(ClientAddr.sin_addr));
			tempStr = buffer;
			server_buffer.push_back(tempStr);
			if ((ThreadHandle = CreateThread(NULL, 0, WorkerThread, (LPVOID)AcceptEvent, 0, &ThreadId)) == NULL)
			{
				printf("CreateThread() failed with error %d\n", GetLastError());
				return 1;
			}
			else;
				//printf("CreateThread() should be fine!\n");
		}
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

	EventArray[0] = (WSAEVENT)lpParameter;

	while (TRUE)
	{
		while (TRUE)
		{
			Index = WSAWaitForMultipleEvents(1, EventArray, FALSE, WSA_INFINITE, TRUE);
			if (Index == WSA_WAIT_FAILED)
			{
				printf("WSAWaitForMultipleEvents() failed with error %d\n", WSAGetLastError());
				return FALSE;
			}
			else;
				//printf("WSACreateEvent() is OK!\n");

			if (Index != WAIT_IO_COMPLETION)
			{
				break;
			}
			else {
				//printf("%d idle", GetCurrentThreadId());
					//server_buffer.push_back(std::to_string(GetCurrentThreadId()) + " idle\n");
			}
		}

		WSAResetEvent(EventArray[Index - WSA_WAIT_EVENT_0]);
		if ((SocketInfo = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
		{
			printf("GlobalAlloc() failed with error %d\n", GetLastError());
			return FALSE;
		}
		else;
			//printf("GlobalAlloc() for SOCKET_INFORMATION is OK!\n");

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
		else;
			//printf("WSARecv() is OK!\n");
		//printf("Socket %d got connected...\n", AcceptSocket)
		//printf("Accept thread %d\n", (int)GetThreadId(GetCurrentThread()))
		char buffer[15];
		snprintf(buffer, sizeof(buffer), "Socket %d got connected...\n", AcceptSocket);
		std::string tempStr = buffer;
		server_buffer.push_back(tempStr);
		int i = GetThreadId(GetCurrentThread());
		snprintf(buffer, sizeof(buffer), "Accept thread %d \n", i);
		tempStr = buffer;
		server_buffer.push_back(tempStr);
	}
	return TRUE;
}

BOOL WINAPI ConsoleHandler(DWORD dwType)
{
	switch(dwType)
	{
	case CTRL_C_EVENT:
		printf("ctrl-c\n");
		closesocket(ListenSocket);
		hTempFile = CreateFile((LPTSTR)fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hTempFile == INVALID_HANDLE_VALUE)
		{
			printf("Error trying creat temp file %d", GetLastError());
		}
		int index = 0;
		do
		{
			const char* charBuffer = server_buffer[server_buffer.size() - index].c_str();
			DWORD bytesWrriten = 0;
			index++;
			if (!WriteFile(hTempFile, charBuffer, sizeof(charBuffer), &bytesWrriten, NULL))
			{
				printf("Error trying write to file %d", GetLastError());
			}
			server_buffer.pop_back();
		}
		while (server_buffer.size() != 0);
		exit(1);
		break;
	}
	return true;
}

void CALLBACK WorkerRoutine(DWORD Error, DWORD BytesTransferred, LPWSAOVERLAPPED Overlapped, DWORD InFlags)
{
	DWORD SendBytes, RecvBytes;
	DWORD Flags;
	char buffer[100];
	std::string tempStr;

	LPSOCKET_INFORMATION SI = (LPSOCKET_INFORMATION)Overlapped;
	if (Error != 0)
	{
		printf("I/O operation failed with error %d\n", Error);
	}

	if (BytesTransferred == 0)
	{
		//printf("Closing socket %d\n\n", SI->Socket)
		snprintf(buffer, sizeof(buffer), "Closing socket %d\n\n", SI->Socket);
		tempStr = buffer;
		server_buffer.push_back(tempStr);
	}

	if (Error != 0 || BytesTransferred == 0)
	{
		closesocket(SI->Socket);
		GlobalFree(SI);
		//printf("%d clinet disconected\n", (int)GetCurrentThreadId())
		snprintf(buffer, sizeof(buffer), "%d clinet disconected\n", (int)GetCurrentThreadId());
		tempStr = buffer;
		server_buffer.push_back(tempStr);
		return;
	}

	if (SI->BytesRECV == 0)
	{
		SI->BytesRECV = BytesTransferred;
		SI->BytesSEND = 0;
	}
	else
	{
		SI->BytesSEND += BytesTransferred;
	}

	if (SI->BytesRECV > SI->BytesSEND)
	{
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
		else;
			//printf("WSASend() is OK!\n");
	}
	else
	{
		SI->BytesRECV = 0;

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
		else;
			//printf("WSARecv() is fine!\n");
	}
}
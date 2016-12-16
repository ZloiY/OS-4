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
#include <iterator>
#include <algorithm>
#include <csignal>

#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "5223"
#define PORT 5223
#define DATA_BUFSIZE 8192

using namespace std;

typedef struct _SOCKET_INFORMATION
{
	OVERLAPPED Overlapped;
	SOCKET Socket;
	CHAR Buffer[DATA_BUFSIZE];
	WSABUF DataBuf;
	DWORD BytesSEND;
	DWORD BytesRECV;
} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;

void CALLBACK WorkerRoutine(DWORD Error, DWORD BytesTransferred, LPWSAOVERLAPPED Overlapped, DWORD InFlags);
DWORD WINAPI WorkerThread(LPVOID lpParameter);
BOOL WINAPI CtrlHandler(DWORD);
void SignalHandler(int param);

SOCKET AcceptSocket;
SOCKET ListenSocket;
HANDLE hTempFile;
SOCKADDR_IN ClientAddr = {0};
CRITICAL_SECTION critical_section;
TCHAR lpTempFileBuffer[MAX_PATH];
BOOL SUPERGODBOOLEAN = TRUE;
std::vector<std::string> server_buffer;

int main(int argc, char** argv)
{
	WSADATA wsaData;
	SOCKADDR_IN InternetAddr;
	SOCKADDR_IN ClientAddr = {1};
	INT Ret;
	HANDLE ThreadHandle;
	DWORD ThreadId;
	InitializeCriticalSectionAndSpinCount(&critical_section, 200);
	//server_buffer.reserve(MAX_PATH);
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
	{
		printf("Unable to install handler!\n");
		return EXIT_FAILURE;
	}
	void (*sig_handler)(int);
	sig_handler = signal(SIGINT, SignalHandler);

	if ((Ret = WSAStartup((2 , 2), &wsaData)) != 0)
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
	/*if ((AcceptEvent = WSACreateEvent()) == WSA_INVALID_EVENT)
	{
		printf("WSACreateEvent() failed with error %d\n", WSAGetLastError());
		return 1;
	}	else;
	//printf("WSACreateEvent() is OK!\n");

	/*if ((ThreadHandle = CreateThread(NULL, 0, WorkerThread, (LPVOID)AcceptEvent, 0, &ThreadId)) == NULL)
	{
		printf("CreateThread() failed with error %d\n", GetLastError());
		return 1;
	}
	else
		printf("CreateThread() should be fine!\n");*/
	char buffer[50];
	std::string tempStr = "";
	int size = sizeof(SOCKADDR_IN);
	while (SUPERGODBOOLEAN)
	{
		AcceptSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddr, &size);
		WSAEVENT AcceptEvent;
		if ((AcceptEvent = WSACreateEvent()) == WSA_INVALID_EVENT)
		{
			printf("WSACreateEvent() failed with error %d\n", WSAGetLastError());
			return 1;
		}

		if (WSASetEvent(AcceptEvent) == FALSE)
		{
			printf("WSASetEvent() failed with error %d\n", WSAGetLastError());
			return 1;
		}
		else
		{
			printf("accept new client %s\n", inet_ntoa(ClientAddr.sin_addr));
			snprintf(buffer, sizeof(buffer), "accept new client %s \n", inet_ntoa(ClientAddr.sin_addr));
			tempStr = buffer;
			EnterCriticalSection(&critical_section);
			server_buffer.push_back(tempStr);
			LeaveCriticalSection(&critical_section);
			if ((ThreadHandle = CreateThread(NULL, 0, WorkerThread, (LPVOID)AcceptEvent, 0, &ThreadId)) == NULL)
			{
				printf("CreateThread() failed with error %d\n", GetLastError());
				return 1;
			}
			else;
			//printf("CreateThread() should be fine!\n");
		}
	}
	Sleep(INFINITE);
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
	char buffer[50];
	snprintf(buffer, sizeof(buffer), "Socket %d got connected...\n", AcceptSocket);
	std::string tempStr = buffer;
	EnterCriticalSection(&critical_section);
	server_buffer.push_back(tempStr);
	LeaveCriticalSection(&critical_section);
	snprintf(buffer, sizeof(buffer), "Accept thread %d \n", GetThreadId(GetCurrentThread()));
	tempStr = buffer;
	EnterCriticalSection(&critical_section);
	server_buffer.push_back(tempStr);
	LeaveCriticalSection(&critical_section);
	DWORD result;
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
			//else printf("WSACreateEvent() is OK!\n");
			if (Index != WAIT_IO_COMPLETION)
			{
				break;
			}
			else
			{
				//printf("%d idle", GetCurrentThreadId());
				EnterCriticalSection(&critical_section);
				server_buffer.push_back(std::to_string(GetCurrentThreadId()) + " idle\n");
				LeaveCriticalSection(&critical_section);
			}
		}
		WSAResetEvent(EventArray[Index - WSA_WAIT_EVENT_0]);
		//printf("GlobalAlloc() for SOCKET_INFORMATION is OK!\n");


		Flags = 0;

		if ((SocketInfo = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
		{
			printf("GlobalAlloc() failed with error %d\n", GetLastError());
			return FALSE;
		}
		//else printf("Global Alloc success");
		SocketInfo->Socket = AcceptSocket;
		ZeroMemory(&(SocketInfo->Overlapped), sizeof(WSAOVERLAPPED));
		SocketInfo->BytesSEND = 0;
		SocketInfo->BytesRECV = 0;
		SocketInfo->DataBuf.len = DATA_BUFSIZE;
		SocketInfo->DataBuf.buf = SocketInfo->Buffer;
		//if (SocketInfo->Socket != INVALID_SOCKET)
		//{
			result = WSARecv(SocketInfo->Socket, &(SocketInfo->DataBuf), 1, &RecvBytes, &Flags,
			                 &(SocketInfo->Overlapped), WorkerRoutine);
			if (result == SOCKET_ERROR)
			{
				//printf("result = %d \n", result);
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					printf("WSARecv()1 failed with error %d\n", WSAGetLastError());
					return FALSE;
				}
				//else break;
			}
			else printf("WSARecv() is OK!\n");
		/*}
		/*else
		{
			printf("Socket is invalid %d\n", WSAGetLastError());
			return FALSE;
		}*/
	}
	//printf("Escaped!\n");
	//printf("Socket %d got connected...\n", AcceptSocket)
	//printf("Accept thread %d\n", (int)GetThreadId(GetCurrentThread()))

	return TRUE;
}

BOOL WINAPI CtrlHandler(DWORD dwType)
{
	switch (dwType)
	{
	case CTRL_C_EVENT:
		raise(SIGINT);
		break;
	}
	return true;
}

void SignalHandler(int param)
{
	printf("ctrl-c\n");
	SUPERGODBOOLEAN = FALSE;
	closesocket(ListenSocket);
	closesocket(AcceptSocket);
	if (GetTempFileName(L".", TEXT("LABA"), 0, lpTempFileBuffer) == 0)
	{
		printf("Something go wrong with GetTempFile() %d", GetLastError());
	}
	hTempFile = CreateFile((LPTSTR)lpTempFileBuffer, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hTempFile == INVALID_HANDLE_VALUE)
	{
		printf("Error trying creat temp file %d", GetLastError());
	}
	EnterCriticalSection(&critical_section);
	for (auto it = begin(server_buffer); it != end(server_buffer); it++)
	{
		DWORD bytesWrriten = 0;
		if (!WriteFile(hTempFile, (*it).c_str(), (*it).size(), &bytesWrriten, NULL))
		{
			printf("Error trying write to file %d", GetLastError());
		}
	}
	LeaveCriticalSection(&critical_section);
	/*for (int i = 0; i < server_buffer.size(); ++i)
	{
		DWORD bytesWrriten = 0;
		if (!WriteFile(hTempFile, server_buffer[i].c_str(), server_buffer.size(), &bytesWrriten, NULL))
		{
			printf("Error trying write to file %d", GetLastError());
		}
	}*/
	server_buffer.clear();
	WSACleanup();
	exit(1);
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
		snprintf(buffer, sizeof(buffer), "Closing socket %d\n", SI->Socket);
		tempStr = buffer;
		EnterCriticalSection(&critical_section);
		server_buffer.push_back(tempStr);
		LeaveCriticalSection(&critical_section);
	}

	if (Error != 0 || BytesTransferred == 0)
	{
		closesocket(SI->Socket);
		GlobalFree(SI);
		//printf("%d clinet disconected\n", (int)GetCurrentThreadId())
		snprintf(buffer, sizeof(buffer), "%d clinet disconected\n", (int)GetCurrentThreadId());
		tempStr = buffer;
		EnterCriticalSection(&critical_section);
		server_buffer.push_back(tempStr);
		LeaveCriticalSection(&critical_section);
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

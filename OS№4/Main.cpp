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
BOOL WINAPI CtrlHandler(DWORD);
void SignalHandler(int param);

SOCKET AcceptSocket;
SOCKET ListenSocket;
HANDLE hTempFile;
SOCKADDR_IN ClientAddr = { 0 };
CRITICAL_SECTION critical_section;
TCHAR lpTempFileBuffer[MAX_PATH];
std::vector<std::string> server_buffer;

int main(int argc, char** argv) {
	WSADATA wsaData;
	SOCKADDR_IN InternetAddr;
	INT Ret;
	HANDLE ThreadHandle;
	DWORD ThreadId;
	InitializeCriticalSectionAndSpinCount(&critical_section, 200);
	//server_buffer.reserve(MAX_PATH);
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE)) {
		printf("Unable to install handler!\n");
		return EXIT_FAILURE;
	}
	void(*sig_handler)(int);
	sig_handler = signal(SIGINT, SignalHandler);

	if ((Ret = WSAStartup((2, 2), &wsaData)) != 0) {
		printf("WSAStartup() failed with error %d\n", Ret);
		WSACleanup();
		return 1;
	}
	else;
	//printf("WSAStartup() is OK!\n");

	if ((ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET) {
		printf("Failed to get a socket %d\n", WSAGetLastError());
		return 1;
	}
	else;
	printf("WSASocket() is pretty fine!\n");

	InternetAddr.sin_family = AF_INET;
	InternetAddr.sin_addr.s_addr = INADDR_ANY;
	InternetAddr.sin_port = htons(PORT);

	if (bind(ListenSocket, (SOCKADDR*)&InternetAddr, sizeof(InternetAddr)) == SOCKET_ERROR) {
		printf("bind() failed with error %d\n", WSAGetLastError());
		return 1;
	}
	else;
	printf("bind() is OK!\n");

	if (listen(ListenSocket, 0)) {
		printf("listen() failed with error %d\n", WSAGetLastError());
		return 1;
	}
	else;
	printf("listen() is OK!\n");
	char buffer[50];
	std::string tempStr = "";
	int size = sizeof(SOCKADDR_IN);
	while (TRUE) {
		AcceptSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddr, &size);
		if (AcceptSocket == INVALID_SOCKET) {
			if (WSAGetLastError() != WSAEINTR) {
				printf("accept failed with error %d\n", WSAGetLastError());
				return 1;
			}
			else {
				printf("AcceptSocket was interrupted\n");
				break;
			}
		}
		WSAEVENT AcceptEvent;
		if ((AcceptEvent = WSACreateEvent()) == WSA_INVALID_EVENT) {
			printf("WSACreateEvent() failed with error %d\n", WSAGetLastError());
			return 1;
		}

		if (WSASetEvent(AcceptEvent) == FALSE) {
			printf("WSASetEvent() failed with error %d\n", WSAGetLastError());
			return 1;
		}
		else {
			printf("accept new client %s\n", inet_ntoa(ClientAddr.sin_addr));
			snprintf(buffer, sizeof(buffer), "accept new client %s \n", inet_ntoa(ClientAddr.sin_addr));
			tempStr = buffer;
			EnterCriticalSection(&critical_section);
			server_buffer.push_back(tempStr);
			LeaveCriticalSection(&critical_section);
			if ((ThreadHandle = CreateThread(NULL, 0, WorkerThread, (LPVOID)AcceptEvent, 0, &ThreadId)) == NULL) {
				printf("CreateThread() failed with error %d\n", GetLastError());
				return 1;
			}
			//else printf("CreateThread() should be fine!\n");
		}
	}
	if (GetTempFileName(L".", TEXT("LAB"), 0, lpTempFileBuffer) == 0) {
		printf("Something go wrong with GetTempFile() %d", GetLastError());
	}
	hTempFile = CreateFile((LPTSTR)lpTempFileBuffer, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hTempFile == INVALID_HANDLE_VALUE) {
		printf("Error trying creat temp file %d", GetLastError());
	}
	for (auto it = begin(server_buffer); it != end(server_buffer); it++) {
		DWORD bytesWrriten = 0;
		if (!WriteFile(hTempFile, (*it).c_str(), (*it).size(), &bytesWrriten, NULL)) {
			printf("Error trying write to file %d", GetLastError());
		}
	}
	server_buffer.clear();
	WSACleanup();
	return 0;
}

DWORD WINAPI WorkerThread(LPVOID lpParameter) {
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
	while (TRUE) {
		while (TRUE) {
			Index = WSAWaitForMultipleEvents(1, EventArray, FALSE, WSA_INFINITE, TRUE);
			if (Index == WSA_WAIT_FAILED) {
				printf("WSAWaitForMultipleEvents() failed with error %d\n", WSAGetLastError());
				return FALSE;
			}
			//else printf("WSACreateEvent() is OK!\n");
			if (Index != WAIT_IO_COMPLETION) {
				break;
			}
			else {
				//printf("%d idle", GetCurrentThreadId());
				EnterCriticalSection(&critical_section);
				server_buffer.push_back(std::to_string(GetCurrentThreadId()) + " idle\n");
				LeaveCriticalSection(&critical_section);
			}
		}
		WSAResetEvent(EventArray[Index - WSA_WAIT_EVENT_0]);
		Flags = 0;
		if ((SocketInfo = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL) {
			printf("GlobalAlloc() failed with error %d\n", GetLastError());
			return FALSE;
		}
		SocketInfo->Socket = AcceptSocket;
		ZeroMemory(&(SocketInfo->Overlapped), sizeof(WSAOVERLAPPED));
		SocketInfo->BytesSEND = 0;
		SocketInfo->BytesRECV = 0;
		SocketInfo->DataBuf.len = DATA_BUFSIZE;
		SocketInfo->DataBuf.buf = SocketInfo->Buffer;
		result = WSARecv(SocketInfo->Socket, &(SocketInfo->DataBuf), 1, &RecvBytes, &Flags,
			&(SocketInfo->Overlapped), WorkerRoutine);
		if (result == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				printf("WSARecv()1 failed with error %d\n", WSAGetLastError());
				return FALSE;
			}
		}
		else printf("WSARecv() is OK!\n");
	}
	return TRUE;
}

BOOL WINAPI CtrlHandler(DWORD dwType) {
	switch (dwType) {
	case CTRL_C_EVENT:
		raise(SIGINT);
		break;
	}
	return true;
}

void SignalHandler(int param) {
	printf("SIGINT\n");
	closesocket(ListenSocket);
	closesocket(AcceptSocket);
}

void CALLBACK WorkerRoutine(DWORD error, DWORD bytesTransferred, LPWSAOVERLAPPED overlapped, DWORD inFlags) {
	DWORD sendBytes, recvBytes;
	DWORD flags;
	char buffer[100];
	std::string tempStr;

	LPSOCKET_INFORMATION SI = (LPSOCKET_INFORMATION)overlapped;
	if (error != 0) {
		if (error == WSAECONNABORTED)
			printf("SIGINT Closing socket %d\n", SI->Socket);
		else printf("I/O operation on socket %llu failed with error %d\n", SI->Socket, error);
		return;
	}
	if (bytesTransferred == 0) {
		//printf("Closing socket %d\n\n", SI->Socket)
		snprintf(buffer, sizeof(buffer), "Closing socket %d\n", SI->Socket);
		tempStr = buffer;
		EnterCriticalSection(&critical_section);
		server_buffer.push_back(tempStr);
		LeaveCriticalSection(&critical_section);
	}

	if (error != 0 || bytesTransferred == 0) {
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

	if (SI->BytesRECV == 0) {
		SI->BytesRECV = bytesTransferred;
		SI->BytesSEND = 0;
	}
	else {
		SI->BytesSEND += bytesTransferred;
	}

	if (SI->BytesRECV > SI->BytesSEND) {
		ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));
		SI->DataBuf.buf = SI->Buffer + SI->BytesSEND;
		SI->DataBuf.len = SI->BytesRECV - SI->BytesSEND;

		if (WSASend(SI->Socket, &(SI->DataBuf), 1, &sendBytes, 0, &(SI->Overlapped), WorkerRoutine) == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				printf("WSASend() failed with error %d\n", WSAGetLastError());
				return;
			}
		}
		else;
		//printf("WSASend() is OK!\n");
	}
	else {
		SI->BytesRECV = 0;

		flags = 0;
		ZeroMemory(&(SI->Overlapped), sizeof(WSAOVERLAPPED));
		SI->DataBuf.len = DATA_BUFSIZE;
		SI->DataBuf.buf = SI->Buffer;

		if (WSARecv(SI->Socket, &(SI->DataBuf), 1, &recvBytes, &flags, &(SI->Overlapped), WorkerRoutine) == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				printf("WSARecv() failed with error %d\n", WSAGetLastError());
				return;
			}
		}
		else;
		//printf("WSARecv() is fine!\n");
	}
}

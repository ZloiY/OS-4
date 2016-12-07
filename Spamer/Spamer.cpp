// Spamer.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include "Windows.h"

using namespace std;

int main() {
	for (int i = 0; i < 20; i++) {
		STARTUPINFO suinfo;
		ZeroMemory(&suinfo, sizeof(STARTUPINFO));
		PROCESS_INFORMATION pinfo;
		if (!CreateProcess(L"Client.exe", NULL, NULL, NULL, FALSE,
		                   NULL, NULL, NULL, &suinfo, &pinfo)) {
			printf("value failed (%d)", GetLastError());
			return 1;
		}
		Sleep(300);
	}
	return 0;
}

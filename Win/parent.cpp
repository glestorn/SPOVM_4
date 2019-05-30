#define _CRT_SECURE_NO_WARNINGS
#define MAX_SECTION_SIZE 30

#include <conio.h>
#include <windows.h>
#include <iostream>

using namespace std;


PROCESS_INFORMATION initServer()
{
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	PROCESS_INFORMATION pi;
	char args[15];
	sprintf(args, "server.exe");
	if (!CreateProcess(NULL, args, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		printf("Create process failed (%d)\n", GetLastError());
	}
	return pi;
}

int main()
{
	HANDLE startPrint = CreateSemaphore(NULL, 0, 1, "PRINT:START");
	HANDLE finishPrint = CreateSemaphore(NULL, 0, 1, "PRINT:FINISH");
	HANDLE close = CreateSemaphore(NULL, 0, 1, "CLOSE");

	bool closeFlag = false;
	HANDLE map;
	LPVOID buff;
	map = CreateFileMapping(
						INVALID_HANDLE_VALUE,
						NULL,
						PAGE_READWRITE,
						0,
						MAX_SECTION_SIZE,
						"FileMap2");
	buff = MapViewOfFile(
						map,
						FILE_MAP_ALL_ACCESS,
						0,
						0,
						MAX_SECTION_SIZE);
	char unifyString[MAX_SECTION_SIZE];

	PROCESS_INFORMATION server = initServer();
	
	while (true)
	{
		if (WaitForSingleObject(startPrint, 1) == WAIT_OBJECT_0)
		{
			memset(unifyString, '\0', MAX_SECTION_SIZE);
			strncpy(unifyString, (char*)buff, MAX_SECTION_SIZE);
			std::cout << std::endl;
			for (int j = 0; j < strlen(unifyString); j++)
			{
				printf("%c", unifyString[j]);
				Sleep(75);
			}

			if (WaitForSingleObject(close, 1) == WAIT_OBJECT_0) {
				closeFlag = true;
				break;
			}

			ReleaseSemaphore(finishPrint, 1, NULL);
		}

		if (closeFlag) {
			break;
		}

		if (WaitForSingleObject(close, 1) == WAIT_OBJECT_0) {
			break;
		}
	}

	WaitForSingleObject(server.hProcess, INFINITE);

	CloseHandle(startPrint);
	CloseHandle(finishPrint);
	CloseHandle(close);
	CloseHandle(server.hProcess);
	CloseHandle(server.hThread);
	UnmapViewOfFile(buff);
	CloseHandle(map);

	cout << endl << endl << "Server terminated.";
	printf("\n");
	system("pause");
	return 0;
}

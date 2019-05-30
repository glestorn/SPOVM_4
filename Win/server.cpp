#define _CRT_SECURE_NO_WARNINGS
#define MAX_SECTION_SIZE 30

#include <conio.h>
#include <cstdlib>
#include <windows.h>

#include <string>
#include <vector>
#include <iostream>

using namespace std;

vector<HANDLE> threadInfo;
vector<HANDLE> closeMutexs;
vector<HANDLE> printMutexs;

HANDLE startPrint;
HANDLE finishPrint;
HANDLE close;

char procID[10];
int currentThread = 0;
bool mapCompleted = false;

DWORD WINAPI ClientThreadRoutine(void* arg) 
{
	char procID[10];
	int id = threadInfo.size();

	sprintf(procID, " %dclose", id);
	HANDLE close = OpenMutex(SYNCHRONIZE, FALSE, procID);

	sprintf(procID, " %dprint", id);
	HANDLE print = OpenMutex(SYNCHRONIZE, FALSE, procID);

	HANDLE map;
	map = OpenFileMapping(FILE_MAP_ALL_ACCESS,
						  FALSE,
						  "FileMap2");
	LPVOID buff;
	buff = MapViewOfFile(map,
						 FILE_MAP_ALL_ACCESS,
						 0,
						 0,
						 MAX_SECTION_SIZE);

	char empty[MAX_SECTION_SIZE];
	memset(empty, '\0', MAX_SECTION_SIZE);
	char unifyString[MAX_SECTION_SIZE];
	sprintf(unifyString, " Process %d", id);

	while (true)
	{
		if (WaitForSingleObject(print, 1) == WAIT_OBJECT_0)
		{
			CopyMemory((PVOID)buff, empty, sizeof(empty));
			CopyMemory((PVOID)buff, unifyString, strlen(unifyString));
			ReleaseMutex(print);
			mapCompleted = true;
			Sleep(5);
		}
		if (WaitForSingleObject(close, 1) == WAIT_OBJECT_0)
		{
			CloseHandle(close);
			CloseHandle(print);
			UnmapViewOfFile(buff);
			CloseHandle(map);
			return 0;
		}
	}

	return 0;
}
	

void addThread() 
{
	int tmp = threadInfo.size() + 1;
	HANDLE descr = CreateThread(
						NULL,							// default security attributes
						0,								// use default stack size  
						ClientThreadRoutine,			// thread function name
						NULL,							// argument to thread function 
						0,								// use default creation flags 
						NULL);							// doesn't return the thread identifier 
	if (!descr) {
		printf("Create Handle failed (%d)\n", GetLastError());
	}

	threadInfo.push_back(descr);

	sprintf(procID, " %dclose", threadInfo.size());
	closeMutexs.push_back(CreateMutex(NULL, TRUE, procID));

	sprintf(procID, " %dprint", threadInfo.size());
	printMutexs.push_back(CreateMutex(NULL, TRUE, procID));
}

void removeThread() {
	ReleaseMutex(closeMutexs.back());

	WaitForSingleObject(threadInfo.back(), INFINITE);

	CloseHandle(closeMutexs.back());
	CloseHandle(printMutexs.back());
	CloseHandle(threadInfo.back());

	closeMutexs.pop_back();
	printMutexs.pop_back();
	threadInfo.pop_back();
}

int main()
{
	cout << "\t'+' to create new thread;" << endl;
	cout << "\t'-' to delete last thread;" << endl;
	cout << "\t'q' to quit;" << endl << endl;

	startPrint = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "PRINT:START");
	finishPrint = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "PRINT:FINISH");
	close = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "CLOSE");

	char key = 0;
	ReleaseSemaphore(finishPrint, 1, NULL);

	while (true)
	{
		if (!threadInfo.empty() && WaitForSingleObject(finishPrint, 1) == WAIT_OBJECT_0)
		{
			if (currentThread >= threadInfo.size() - 1) 
				currentThread = -1;
			currentThread++;
			mapCompleted = false;
			ReleaseMutex(printMutexs[currentThread]);
			while (!mapCompleted);
			WaitForSingleObject(printMutexs[currentThread], INFINITE);
			ReleaseSemaphore(startPrint, 1, NULL);
		}

		if (kbhit())
		{
			key = _getch();
			if (key == '+')
			{
				addThread();
				Sleep(1);
			}

			if (key == '-' && !threadInfo.empty())
			{
				removeThread();
			}

			if (key == 'q')
			{
				while (!threadInfo.empty()) {
					removeThread();
				}
				break;
			}
		}
	}

	ReleaseSemaphore(close, 1, NULL);
	return 0;
}

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <vector>
#include <termios.h>
#include <semaphore.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>

const char* SEMAPHORE_NAME = "/my_sema";
const char* SEMAPHORE_NAME_2 = "/my_sema_2";
const char* SEMAPHORE_NAME_3 = "/my_sema_3";
const int MESS_SIZE = 30;
pid_t controllerPID;
int pipeRd;
int pipeWr;

void runController();

int main(int argc, char* argv[])
{
	int pipeIndexes[2];
    pipe(pipeIndexes);
    pipeRd = pipeIndexes[0];
    pipeWr = pipeIndexes[1];

    sem_t* print;
    if ((print = sem_open(SEMAPHORE_NAME, O_CREAT, 0777, 0)) == SEM_FAILED) {
    	std::cout << "There is an error in creatint print semaphore" << std::endl;
    	return 1;
    }

    sem_t* write;
    if ((write = sem_open(SEMAPHORE_NAME_2, O_CREAT, 0777, 0)) == SEM_FAILED) {
    	std::cout << "There is an error in creating write semaphore" << std::endl;
    	return 1;
    }

    sem_t* finishProgram;
    if ((finishProgram = sem_open(SEMAPHORE_NAME_3, O_CREAT, 0777, 0)) == SEM_FAILED) {
    	std::cout << "There is an error in creating finishSemaphore";
    	return 1;
    }

	runController();

	char outputRow[MESS_SIZE];
	
	int value = 0;

	while (true) {
		sem_wait(print);
		memset(outputRow, '\0', MESS_SIZE);
		read(pipeRd, &outputRow, MESS_SIZE);
		for (int i = 0; i < strlen(outputRow); i++) {
			std::cout << outputRow[i] << std::endl;
			usleep(100000);
		}
		std::cout << std::endl;
		sem_post(write);

		sem_getvalue(finishProgram, &value);
		if (value > 0) {
			close(pipeRd);
			break;
		}
	}
	return 0;
}


void runController()
{
	pid_t pid = fork();
	controllerPID = pid;

	if (pid < 0) {
		std::cout << "!!! Controller wasn't created !!!" << std::endl;
	}

	if (pid == 0) {
		char pipeData[10];
		sprintf(pipeData, "%d", pipeWr);
		if (execlp("./controller", pipeData, NULL) == -1) {
			std::cout << "Error in creating of controller" << std::endl;	
		}	
	}
}
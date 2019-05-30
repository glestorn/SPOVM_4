#include <iostream>
#include <vector>
#include <unistd.h>
#include <semaphore.h>
#include <termios.h>
#include <string>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <mutex>

std::vector<pthread_t> threadInfo;
std::vector<pthread_mutex_t> closeMutexs;
const char* SEMAPHORE_NAME = "/my_sema";
const char* SEMAPHORE_NAME_2 = "/my_sema_2";
const char* SEMAPHORE_NAME_3 = "/my_sema_3";
const int MESS_SIZE = 30;
int threadID;
sem_t* finishProgram;
int closeState = 0;

pthread_mutex_t mutex;
pthread_cond_t objOfSynchr;
int pipeWr;
int canWrite = 1;

void addThread(int pipeWr);
void removeThread();
int getch();
bool kbhit();
int switchMenu(char key);

static void *thread_func(void *arg)
{
	char outputRow[MESS_SIZE];
	sprintf(outputRow, "Process%d", threadID-1);
	while (true) {

		if (!pthread_mutex_trylock(&mutex)) {
			while (!canWrite) {
				pthread_cond_wait(&objOfSynchr, &mutex);
			}
			write(pipeWr, outputRow, MESS_SIZE);
			canWrite = 0;
			pthread_mutex_unlock(&mutex);
		}

		if (threadInfo.back() == pthread_self()) {
			if (!pthread_mutex_trylock(&closeMutexs.back())) {
				closeMutexs.pop_back();
				threadInfo.pop_back();
				threadID--;
				pthread_exit(NULL);
			}
		}
	}

	return (void*) true;
}

int main(int argc, char* argv[])
{
	pipeWr = atoi(argv[0]);

    sem_t* print;
    if ((print = sem_open(SEMAPHORE_NAME, 0)) == SEM_FAILED) {
    	std::cout << "There is an error in opening print semaphore" << std::endl;
    	return 1;
    }

    sem_t* write;
    if ((write = sem_open(SEMAPHORE_NAME_2, 0)) == SEM_FAILED) {
    	std::cout << "There is an error in opening write semaphore" << std::endl;
    	return 1;
    }

 	if ((finishProgram = sem_open(SEMAPHORE_NAME_3, 0)) == SEM_FAILED ) {
 		std::cout << "There is an error in opening finishSemaphore";
        return 1;
    }

	if(pthread_mutex_init(&mutex, NULL) != 0) {
    	std::cout << "Mutex create error" << std::endl;
	}

	if (pthread_cond_init(&objOfSynchr, NULL) != 0) {
		std::cout << "Synchronize object create error" << std::endl;
	}

	threadID = 1;
	char choice;

	std::cout << "Enter + to add a person" << std::endl;
	std::cout << "Enter - to remove a person" << std::endl;
	std::cout << "Enter q to exit" << std::endl;

	int switchCallback;

	while(true) {
		
		if (!threadInfo.empty() && !canWrite) {
			if (!pthread_mutex_trylock(&mutex)) {
				sem_post(print);
				sem_wait(write);
				
				canWrite = 1;
				pthread_cond_signal(&objOfSynchr);
				pthread_mutex_unlock(&mutex);
			}
		}

		if (closeState && threadInfo.empty()) {
			close(pipeWr);
			pthread_mutex_destroy(&mutex);
			pthread_cond_destroy(&objOfSynchr);
			sem_post(finishProgram);
			sem_post(print);
			return 0;
		}

		if (kbhit()) {
			choice = getch();
			
			if (choice != -1) {
				switchCallback = switchMenu((char)choice);
				usleep(1000);
			}
		}
	}
}

void addThread(int pipeWr)
{
    pthread_t tid;
    threadInfo.push_back(tid);

    pthread_mutex_t buff_mutex;
    closeMutexs.push_back(buff_mutex);
    if(pthread_mutex_init(&closeMutexs.back(), NULL) != 0) {
    	std::cout << "Mutex create error" << std::endl;
	}
	pthread_mutex_lock(&closeMutexs.back());

	if (pthread_create(&threadInfo.back(), NULL, thread_func, NULL) != 0) {
		std::cout << "There is an error with creating thread" << std::endl;
	}
}

void removeThread()
{
	pthread_mutex_unlock(&closeMutexs.back());
}

void removeNumThread(int num)
{
	pthread_mutex_unlock(&closeMutexs[num]);
}

int getch()
{
   struct termios oldattr, newattr;
   int ch;
   tcgetattr(STDIN_FILENO, &oldattr);
   newattr = oldattr;
   newattr.c_lflag &= ~(ICANON | ECHO);
   tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
   ch = getchar();
   tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
   return ch;
}

bool kbhit()
{
   termios term;
   tcgetattr(0, &term);

   termios term2 = term;
   term2.c_lflag &= ~ICANON;
   tcsetattr(0, TCSANOW, &term2);

   int byteswaiting;
   ioctl(0, FIONREAD, &byteswaiting);

   tcsetattr(0, TCSANOW, &term);

   return byteswaiting > 0;
}

int switchMenu(char key) {
	int retCode = 0;
	if (key == '+')
	{
		addThread(pipeWr);
		threadID++;
		return retCode;
	}

	if (key == '-')
	{
		if (!threadInfo.empty()) {
			removeThread();
		}
		return retCode;
	}

	if (key == 'q')
	{
		int j = threadInfo.size() - 1;
		for (int j; j >= 0; j--) {
			removeNumThread(j);
		}
				
		retCode = -1;
		closeState = 1;
		return retCode;
	}
	return retCode;
}
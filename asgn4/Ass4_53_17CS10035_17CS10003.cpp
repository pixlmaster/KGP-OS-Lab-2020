/*
Compilation Command: g++ -std=c++11 -pthread Ass4_53_17CS10035_17CS10003.cpp

Authors:
Prabhpreet Singh Sodhi 17CS10035
and
Akash Tiwari 17CS10003
*/

#include <sys/types.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <algorithm>
#include <iostream>
#include <climits>
#include <pthread.h>
#include <sys/syscall.h> 
#include <chrono> 

//#define PRODUCERS 2
//#define CONSUMERS 2
//#define WORKERS 4
#define BUFFER_CAPACITY 100
#define PRODUCTION 1000
//#define QUANTUM 1
#define MICRO_QUANTUM 1000000
#define REPORTER_QUANTUM 1000

using namespace std;
using namespace std::chrono;

int WORKERS = 0;
int PRODUCERS = 0;
int CONSUMERS = 0;

typedef struct xyz
{
	int thread_number;
	int new_status;
}INFO;

pthread_mutex_t race_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t reporter_thread, scheduler_thread;
int reporter_trigger  = 0;

int T_ID=0;
pthread_t* worker_threads;
int* status;

int BUFFER[BUFFER_CAPACITY];
int SIZE = 0;
int count_terminated = 0;

void* scheduler(void *args)
{	
	INFO* info = (INFO*)(args);
	while(count_terminated!=WORKERS) //!(count_terminated >= PRODUCERS && SIZE == 0))
	{

		for(int i=0; i<WORKERS; i++)
		{
			if(status[i]==2)
				continue;
			/*
			if(pthread_kill(worker_threads[i], 0)!=0)
			{
				if(status[i]==0 || status[i]==1)
				{
					reporter_trigger = 1;
					info->thread_number = i;
					info->new_status = 2;
					pthread_kill(reporter_thread, SIGUSR2);
					usleep(REPORTER_QUANTUM);
					count_terminated++;
				}
				continue;
			}*/
	
			reporter_trigger = 1;
			info->thread_number = i;
			info->new_status = 1;
			pthread_kill(reporter_thread, SIGUSR2);
			usleep(REPORTER_QUANTUM);
		
			pthread_kill(worker_threads[i], SIGUSR2);

			auto start = high_resolution_clock::now(); 
			while( ((int)duration_cast<microseconds>(high_resolution_clock::now() - start).count())<=MICRO_QUANTUM && pthread_kill(worker_threads[i], 0)==0 );
			//usleep(MICRO_QUANTUM);
			if(pthread_kill(worker_threads[i], 0)!=0)
			{
				reporter_trigger = 1;
				info->thread_number = i;
				info->new_status = 2;
				pthread_kill(reporter_thread, SIGUSR2);
				usleep(REPORTER_QUANTUM);
				count_terminated++;
			}
			else
			{	
				pthread_kill(worker_threads[i], SIGUSR1);

					reporter_trigger = 1;
					info->thread_number = i;
					info->new_status = 0;
					pthread_kill(reporter_thread, SIGUSR2);
					usleep(REPORTER_QUANTUM);
			}

		
			
		}
	}

}
	


void* reporter(void *args)
{
	INFO* update = (INFO* )(args);
	char* messages[] = {(char*)"Suspended", (char*)"Running", (char*)"Terminated"};
	while(1)
	{
		pthread_kill(pthread_self(), SIGUSR1);
		if(reporter_trigger == 1)
		{
			reporter_trigger = 0;
			status[update->thread_number] = update->new_status;
			printf("Thread %d: %s\tNumber of elements: %d\n", update->thread_number, messages[update->new_status], SIZE);
		}
		
		if(count_terminated == WORKERS)
			break;
	}
}


void signal_handler(int signal_number)
{
	switch(signal_number)
	{
		case SIGUSR1:
			pause();
			break;
		case SIGUSR2:
			break;
	}
}

void* producer(void *id)
{
	int thread_id = *((int *)id);

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	for(int i=0; i < PRODUCTION; i++)
	{
		
    	pthread_sigmask(SIG_BLOCK, &set, NULL);
		pthread_mutex_lock(&race_lock);
		//printf("MUTEX");
		if(SIZE < BUFFER_CAPACITY)
		{
			int insert = (rand()%10000);

			
			BUFFER[SIZE] = insert;
			SIZE++;
			//printf("NO MUTEX");
			pthread_mutex_unlock(&race_lock);
			pthread_sigmask(SIG_UNBLOCK, &set, NULL);

			printf("\tInserted %d\n", insert);
		}
		else
		{
			i--;
			//printf("NO MUTEX");
			pthread_mutex_unlock(&race_lock);
			pthread_sigmask(SIG_UNBLOCK, &set, NULL);
		}
		
	}
	
}

void* consumer(void *id)
{
	int thread_id = *((int *)id);
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);

	for(;;)
	{
		pthread_sigmask(SIG_BLOCK, &set, NULL);
		pthread_mutex_lock(&race_lock);
		//printf("MUTEX");
		if(SIZE > 0)
		{
			SIZE--;
			int extract = BUFFER[SIZE];
			//printf("NO MUTEX");
			pthread_mutex_unlock(&race_lock);
			pthread_sigmask(SIG_UNBLOCK, &set, NULL);

			printf("\tExtracted %d\n", extract);
		}
		else if(count_terminated>=PRODUCERS)
		{
			//printf("NO MUTEX");
			pthread_mutex_unlock(&race_lock);
			pthread_sigmask(SIG_UNBLOCK, &set, NULL);
			break;
		}
		else
		{
			pthread_mutex_unlock(&race_lock);
			pthread_sigmask(SIG_UNBLOCK, &set, NULL);
		}
		
	}
	
}	

void* producer_test(void *)
{
	int thread_id = ++T_ID;
	
		printf("Producer %d\n", thread_id);
}

void* consumer_test(void *)
{
	int thread_id = ++T_ID;

		printf("Consumer %d\n", thread_id);
}


int main()
{
	printf("The defined BUFFER_CAPACITY = %d\n", BUFFER_CAPACITY);
	printf("Enter the number of workers: ");
	scanf("%d", &WORKERS);
	worker_threads = (pthread_t *)(malloc(sizeof(pthread_t)*WORKERS));
	status = (int *)(malloc(sizeof(int)*WORKERS));

	for(int i = 0 ; i < WORKERS; i++)
	{
		status[i] = 0;
	}

	struct sigaction action;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	action.sa_handler = signal_handler;
	sigaction(SIGUSR1, &action, NULL);
	sigaction(SIGUSR2, &action, NULL);

	srand(time(NULL));
	int i =0;
	for(i=0; i<WORKERS; i++)
	{
		if((rand()%2)==0)
		{ 
			PRODUCERS++;
			int* temp = (int *)(malloc(sizeof(int)));
			*temp = i;
			printf("Thread %d: Producer\n", i);
			pthread_create(&(worker_threads[i]), NULL, producer, (void *)temp);
			pthread_kill(worker_threads[i], SIGUSR1);
		}
		else
		{
			CONSUMERS++;
			int* temp = (int *)(malloc(sizeof(int)));
			*temp = i;
			printf("Thread %d: Consumer\n", i);
			pthread_create(&(worker_threads[i]), NULL, consumer, (void *)temp);
			pthread_kill(worker_threads[i], SIGUSR1);
		}
	}
	printf("\n");
	printf("NUMBER OF PRODUCERS GENERATED: %d\n", PRODUCERS);
	printf("NUMBER OF CONSUMERS GENERATED: %d\n\n", CONSUMERS);

	if(PRODUCERS==0)
	{
		printf("NO PRODUCERS WERE GENERATED! SYSTEM WILL EXIT!\n");
		return 0;
	}
	if(CONSUMERS==0)
	{
		printf("NO CONSUMERS WERE GENERATED! SYSTEM WILL EXIT!\n");
		return 0;
	}

	INFO* info = (INFO *)malloc(sizeof(INFO));
	pthread_create(&reporter_thread, NULL, reporter, (void* )info);	
	pthread_create(&scheduler_thread, NULL, scheduler, (void *)info);

	pthread_join(scheduler_thread, NULL);
	pthread_kill(reporter_thread, SIGUSR2);
	pthread_join(reporter_thread, NULL);


	return 0;
	
}


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

#define PRODUCER 0
#define CONSUMER 1
#define MAIN 2
#define CAPACITY 2
#define getParent(x) ((x-1)/2)
#define getLeft(x) (2*x + 1)
#define getRight(x) (2*x + 2)

typedef struct job_struct
{
	long long int pid;
	int p_number;
	int priority;
	int compute_time;
	int job_id;

}job;

typedef struct heap_struct
{
	job job_list[CAPACITY];
	int n;

}heap;

typedef struct shmseg_struct 
{
	int jobs_created;
	int jobs_completed;
	long long int race_lock;
	heap p_queue;

}shmseg;

// Heap functions
int insert(heap* H, job J)
{
	if(H->n==CAPACITY)
	{
		return -1;
	}

	H->job_list[H->n] = J;

	int x = H->n;
	while(H->job_list[x].priority > H->job_list[getParent(x)].priority)
	{
		job temp = H->job_list[getParent(x)];
		H->job_list[getParent(x)]=H->job_list[x];
		H->job_list[x] = temp;

		x = getParent(x);
	}

	H->n = H->n + 1;

	return 0;
}

int remove(heap* H, job* top)
{
	top->pid = -1;
	top->p_number = -1;
	top->priority = -1;
	top->compute_time = -1;
	top->job_id = -1;
	if(H->n==0)
		return -1;

	*(top) = H->job_list[0];
	H->n = H->n - 1;
	H->job_list[0] = H->job_list[H->n];

	int j=0;
    while(1)
    {
        int left = getLeft(j);
        int right = getRight(j);
        
        int largest = j;
        
        if(left < H->n && H->job_list[left].priority > H->job_list[largest].priority)
            largest = left;
        if(right < H->n && H->job_list[right].priority > H->job_list[largest].priority)
            largest = right;
            
        if(largest == j)
        {
            break;
        }
        else
        {
            job temp = H->job_list[j];
            H->job_list[j] = H->job_list[largest];
            H->job_list[largest] = temp;
            
            j =  largest;
        }
    }

	return 0;
}

void printHeap(heap H)
{
	printf("***HEAP***\n");
	printf("Capacity = %d/%d\nOrder = ", H.n, CAPACITY);
	int i;
	for(i=0; i<H.n; i++)
	{
		printf(" %d ", H.job_list[i].priority);
	}
	printf("\n------------\n");
}

void printProducerJob(job J, int x)
{
	char *message;
	if(x==PRODUCER)
	{
		message = (char *)"Insertion successfull!";
	}
	else if (x==CONSUMER)
	{
		message = (char *)"Deletion successfull!";
	}

	printf("Producer Number: %d\nProducer PID: %lld\nJob ID: %d\nCompute Time: %d\nPriority: %d\n%s\n\n", J.p_number, J.pid, J.job_id, J.compute_time, J.priority, message);
}

/* HEAP TESTING 
int main()
{
	job A = {1,1,1,1,1};
	job B = {2,2,2,2,2};
	job C = {3,3,3,3,3};
	job D = {4,4,4,4,4};
	job E = {5,5,5,5,5};

	shmseg *s = (shmseg *)malloc(sizeof(shmseg));
	insert(&s->p_queue, B);
	insert(&s->p_queue, A);
	printHeap(s->p_queue);

	job X;
	remove(&s->p_queue, &X);
	remove(&s->p_queue, &X);
	printHeap(s->p_queue);
	printf("%d", X.priority);
}
*/

/* */
int main()
{
	int i;

	int NP, NC, NJ;
	scanf("%d %d %d", &NP, &NC, &NJ);

	int process_type= MAIN;
	int number = -1;

	key_t key = ftok("/dev/random", 'b');
	int shmid = shmget(key, sizeof(shmseg), IPC_CREAT |0666);
	if(shmid < 0)
	{
		printf("Error in creating shared memory\n");
		return -1;
	}
	else
	{
		printf("Shared memory created successfully\n");
	}
	shmseg *shared_mem_pointer = (shmseg *)shmat(shmid, NULL, 0);
	shared_mem_pointer->jobs_created = 0;
	shared_mem_pointer->jobs_completed = 0;
	shared_mem_pointer->p_queue.n = 0;
	shared_mem_pointer->race_lock = 0;

	for(i=0; i<NP; i++)
	{
		int pid = fork();
		if(pid==0)
		{
			number = i + 1;
			process_type = PRODUCER;
			break;
		}
		else 
		{
			process_type = MAIN;
		}	
	}

	if(process_type == MAIN)
	{
		for(i=0; i < NC; i++)
		{
			int pid = fork();
			if(pid==0)
			{
				number = i + 1;
				process_type = CONSUMER;
				break;
			}
			else 
			{
				process_type = MAIN;
			}
		}	
	}

	/*CREATION OF PRODUCERS, CONSUMERS AND MAIN PROCESS OVER */
	srand(time(NULL)+(100*number));

	if(process_type == PRODUCER)
	{
		while(shared_mem_pointer->jobs_created < NJ)
		{
			if((shared_mem_pointer->jobs_created - shared_mem_pointer-> jobs_completed)>= CAPACITY)
				continue;

			if(shared_mem_pointer->race_lock==0)
			{
				shared_mem_pointer->race_lock = (long long int)(getpid());
				usleep((int)(rand()%2000000));
			}
			else if( shared_mem_pointer->race_lock == (long long int)(getpid()) )
			{
				
				if((shared_mem_pointer->jobs_created - shared_mem_pointer-> jobs_completed)>= CAPACITY)
				{
					shared_mem_pointer->race_lock = 0;
				}	
				else
				{
					usleep((int)(rand()%1000000));

					job new_job;
					new_job.pid =(long long int)getpid();
					new_job.p_number = number;
					new_job.priority = ( ((int)rand())%10 )+1;
					new_job.compute_time = ( ((int)rand())%4 ) + 1;
					new_job.job_id = ( ((int)rand())%100000 ) + 1;

					/* UNCOMMENT IT TO LIMIT THE NUMBER OF CREATED JOBS
					if(shared_mem_pointer->jobs_created >= NJ)
					{
						shared_mem_pointer->mutex=1;
						break;
					}
					*/
					shared_mem_pointer->jobs_created = (shared_mem_pointer->jobs_created) + 1;		
					insert(&shared_mem_pointer->p_queue, new_job);
					printProducerJob(new_job, process_type);

					printf("Created: %d Completed: %d\n\n", shared_mem_pointer->jobs_created, shared_mem_pointer->jobs_completed);

					//printf("UNLOCKED!\n\n");
					shared_mem_pointer->race_lock = 0;

				}
			}
			else
			{

			}

			
			//printHeap(shared_mem_pointer->p_queue);
			
		}
	}
	else if (process_type == CONSUMER)
	{
		while(shared_mem_pointer->jobs_completed < NJ)
		{
			if((shared_mem_pointer->jobs_created - shared_mem_pointer-> jobs_completed)<= 0)
				continue;

			if(shared_mem_pointer->race_lock==0)
			{
				shared_mem_pointer->race_lock = (long long int)(getpid());
				usleep((int)(rand()%2000000));
			}
			else if( shared_mem_pointer->race_lock == (long long int)(getpid()) )
			{
				if((shared_mem_pointer->jobs_created - shared_mem_pointer-> jobs_completed)<= 0)
				{
					shared_mem_pointer->race_lock = 0;
				}
				else
				{
					usleep((int)(rand()%1000000));
								
					job* extracted;
					extracted = (job *)(malloc(sizeof(job)));
					shared_mem_pointer->jobs_completed = (shared_mem_pointer->jobs_completed) + 1;
					remove(&shared_mem_pointer->p_queue, extracted);
					printf("Consumer Number: %d\nConsumer PID: %lld\n", number, (long long int)getpid());
					printProducerJob(*extracted, process_type);
		
					printf("Created: %d Completed: %d\n\n", shared_mem_pointer->jobs_created, shared_mem_pointer->jobs_completed);

					shared_mem_pointer->race_lock = 0;
					sleep(extracted->compute_time);
				}
			}
			else
			{
	
			}
			

		}
	}
	else if (process_type == MAIN)
	{
		struct timeval start, end;
		gettimeofday(&start, NULL);
		while(shared_mem_pointer->jobs_completed < NJ || shared_mem_pointer->jobs_created < NJ);
		gettimeofday(&end, NULL); 
		double seconds = (end.tv_sec - start.tv_sec) + ((end.tv_usec - start.tv_usec)*0.000001); 
  
		shmctl(shmid, IPC_RMID, NULL);
		shmdt((void*)shared_mem_pointer);
		usleep(500000);
		printf("\nTime taken (in seconds): %lf\n", seconds);
		kill(0,SIGTERM);
	}
	else
	{
		printf("Invalid/Corrupt Process");
	}
	
}

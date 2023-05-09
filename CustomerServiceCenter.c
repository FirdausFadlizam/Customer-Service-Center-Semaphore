//Name: Muhammad Firdaus Fadlizam
//Program Name: Customer Service Centre 
//Description: This program uses semaphore to coordinate the customer and assistant movement. 
#include <unistd.h>         
#include <stdio.h>           
#include <pthread.h>         
#include <semaphore.h>  
#include <stdlib.h>  
#include <time.h>

//define num of customer, assistant, and awaiting chairs. 
#define MAX_CUS 12
#define MAX_ASSISTANT 2
#define MAX_CHAIRS 4

//semaphore 
sem_t assistant_semaphore;
sem_t customer_semaphore;
sem_t mutex;


int free_chairs = MAX_CHAIRS;

//hold the index of the array associated with the customer
int cus_chair_pos[MAX_CHAIRS];

//let the assistant know who to serve next
int serveMeNext = 0;
int sitHereNext = 0;
static int pid = -1;

struct Customer {
	int id;
	int arrival_time;
	int service_time;
	double time;
};

struct Customer customer[MAX_CUS];

void* customer_entry(void* index) {
	
	struct timespec start;
	int myPos = *(int*)index;
	int mySeat, swapID;
	struct Customer customer_info;
	//get time
	clock_gettime(CLOCK_MONOTONIC, &start);
	//wait for access
	sem_wait(&mutex);
	//pass to the local struct
	customer_info.id = customer[myPos].id;
	customer_info.arrival_time = customer[myPos].arrival_time;
	customer_info.service_time = customer[myPos].service_time;
	customer[myPos].time = start.tv_sec;

	++pid;
	printf("Time %2d Customer %2d \tarrives\n",customer_info.arrival_time, customer_info.id);
	//if awaiting chairs if full
	if (free_chairs > 0) {
		--free_chairs;
		sitHereNext = (++sitHereNext) % MAX_CHAIRS;
		mySeat = sitHereNext;
		cus_chair_pos[mySeat] = pid;
		//release access
		sem_post(&mutex);
		//wake up assistant
		sem_post(&assistant_semaphore);
		//wait for assistant
		sem_wait(&customer_semaphore);
		//wait for access
		sem_wait(&mutex);
		swapID = cus_chair_pos[mySeat];
		//increase number of awaiting chairs
		free_chairs++;
		//release access
		sem_post(&mutex);
		//sleep
		sleep(customer_info.service_time);
	
	}
	
	else {
		sem_post(&mutex);
		printf("Time %2d Customer %2d \t\t\t\tleaves\n", customer_info.arrival_time, customer_info.id);
	}

	//exit
	pthread_exit(0);

}

void* assistant_ready(void* tmp) {

	
	int index = *(int*)tmp;
	int myNext, C;
	//local struct variable
	struct Customer customer_info;
	struct timespec finish;
	int total;

	while (1) {

			//sleep
			sem_wait(&assistant_semaphore);
			//wait for access
			sem_wait(&mutex);
			//find which to serve
			serveMeNext = ++serveMeNext % MAX_CHAIRS;
		    
			myNext = serveMeNext;
			C = cus_chair_pos[myNext];
			//pass to local struct
			customer_info.id = customer[C].id;
			customer_info.arrival_time = customer[C].arrival_time;
			customer_info.service_time = customer[C].service_time;
			customer_info.time = customer[C].time;
			//release access
			sem_post(&mutex);
			//take customer
			sem_post(&customer_semaphore);
			//check how long customer waits
			clock_gettime(CLOCK_MONOTONIC, &finish);
			total = (int)(finish.tv_sec - customer_info.time);
			printf("Time %2d Customer %2d \t\tstarts\n", customer_info.arrival_time + total, customer_info.id);
			//serve customer
			sleep(customer_info.service_time);
			printf("Time %2d Customer %2d \t\t\tdone\n", customer_info.arrival_time + total + customer_info.service_time, customer_info.id);
			
		
		
	}
}

int main() {

	//initialize semaphore
	sem_init(&assistant_semaphore, 0, 0);
	sem_init(&customer_semaphore, 0, 0);
	sem_init(&mutex, 0, 1);

	
	int arrivalTime[] = { 3,7,8,9,11,12,14,16,19,22,34,39 };
	int serviceTime[] = { 15,10,8,5,12,4,8,14,7,2,9,3 };

	//declare thread
	pthread_t assistant_thread[MAX_ASSISTANT];
	pthread_t customer_thread[MAX_CUS];

	//create assistant thread
	for (int i = 0; i < MAX_ASSISTANT; i++) {
		pthread_create(&assistant_thread[i], 0, assistant_ready, (void*)&i);
	}

	//create customer thread
	for (int i = 0; i < MAX_CUS; i++) {
		customer[i].id = i + 1;
		customer[i].arrival_time = arrivalTime[i];
		customer[i].service_time = serviceTime[i];
		if (i != 0)
			usleep((arrivalTime[i] - arrivalTime[i - 1]) * 1000000); //interarrival time between customer
		else
			usleep(arrivalTime[i]);

		int j = i;
		pthread_create(&customer_thread[i], 0, customer_entry, (void*)&j);
	}

	//wait for customer thread to finish before exit
	for (int i = 0; i < MAX_CUS; i++) {
	
		pthread_join(customer_thread[i], 0);
	}

	exit(EXIT_SUCCESS);
	
}
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>


#define SHMKEY ((key_t) 8941)


int size;

//---------------------------
// Structure for storing 
// array information, start
// and end indexes
//---------------------------
typedef struct ArrayInfo{
	int low;
	int high;
} arrInf;

typedef struct{
	int sharedArray[500000];
} shared_mem;

shared_mem *sharedMemArray;

//--------------------------------------------------------------------
// Name:    Merge function
// Purpose: Merges a singular array with a low and high (end indexes)
//				Stores sorted merge into a a larger sorted array
//--------------------------------------------------------------------
void merge (int low, int high){

	//-------------------------------
	// indexes needed for merge sort
	//-------------------------------
	int mid = (low+high)/2; 					
	int left = low;
	int right = mid + 1;
	
	int temp[high - low + 1];
	int i = 0;
	int curr = 0;
	
	//-------
	// merge
	//-------
	while(left <= mid && right <= high){
		if (sharedMemArray->sharedArray[left] > sharedMemArray->sharedArray[right])
			temp[curr++] = sharedMemArray->sharedArray[right++];
		else
			temp[curr++] = sharedMemArray->sharedArray[left++];
	}
	
	while(left <= mid)
		temp[curr++] = sharedMemArray->sharedArray[left++];
	
	while(right <= high)
		temp[curr++] = sharedMemArray->sharedArray[right++];
		
	for (i = 0; i < (high - low + 1); ++i)
		sharedMemArray->sharedArray[low + i] = temp[i];
}

void mergeShm (int low, int high){
	//-------------------------------
	// indexes needed for merge sort
	//-------------------------------
	int mid = (low+high)/2; 					
	int left = low;
	int right = mid + 1;
	
	int temp[high - low + 1];
	int i = 0;
	int curr = 0;
	
	//-------
	// merge
	//-------
	while(left <= mid && right <= high){
		if (sharedMemArray -> sharedArray[left] > sharedMemArray -> sharedArray[right])
			temp[curr++] = sharedMemArray -> sharedArray[right++];
		else
			temp[curr++] = sharedMemArray -> sharedArray[left++];
	}
	
	while(left <= mid)
		temp[curr++] = sharedMemArray -> sharedArray[left++];
	
	while(right <= high)
		temp[curr++] = sharedMemArray -> sharedArray[right++];
		
	for (i = 0; i < (high - low + 1); ++i){
		sharedMemArray -> sharedArray[i + low] = temp[i];
		//printf("%d\n", sharedMemArray->sharedArray[i + low]);
		}
}

//----------------------------------------------
// pointers to void allow
// for dynamic type casting
// in case you're a dumb fuck who is wondering
// wtf I'm doing
//----------------------------------------------
void * mergesortThreaded(void *a){
	arrInf *inputArr = (arrInf *)a;
	int mid = (inputArr -> low + inputArr -> high) / 2;
	
	//----------------------
	// Split array in half
	// Set two threads to 
	// work on each half
	//----------------------
	arrInf arrInfo[2];
	pthread_t thread[2];
	arrInfo[0].low = inputArr -> low;
	arrInfo[0].high = mid;
	
	arrInfo[1].low = mid + 1;
	arrInfo[1].high = inputArr -> high;
	
	if (inputArr -> low >= inputArr -> high)
		return 0;
	
	//-------------------
	// Create the threads
	//-------------------
	int thr1 = pthread_create(&thread[0], NULL, mergesortThreaded, &arrInfo[0]);
	if (thr1 > 0){
		pid_t tid1 = syscall(__NR_gettid);
		printf("Error in thread %d\n", tid1);
		printf("OS has run out of threads to allocate.\n");
		printf("Please try less items.\n\n");
		exit(0);
	}
	int thr2 = pthread_create(&thread[1], NULL, mergesortThreaded, &arrInfo[1]);
	if (thr2 > 0){
		pid_t tid2 = syscall(__NR_gettid);
		printf("Error in thread %d\n", tid2);
		printf("OS has run out of threads to allocate.\n");
		printf("Please try less items.\n\n");
		exit(0);
	}
	//---------------------------
	// Wait for threads to finish
	//---------------------------
	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);
	
	merge(inputArr -> low, inputArr -> high);
	
	pthread_exit(NULL);
}

void mergesortProcesses(void *a){
	arrInf *inputArr = (arrInf *)a;
	int mid = (inputArr -> low + inputArr -> high) / 2;
	//----------------------
	// Split array in half
	// Set two threads to 
	// work on each half
	//----------------------
	arrInf arrInfo[2];
	arrInfo[0].low = inputArr -> low;
	arrInfo[0].high = mid;
	
	arrInfo[1].low = mid + 1;
	arrInfo[1].high = inputArr -> high;
	
	if (inputArr -> low >= inputArr -> high)
		return;
	
	//----------------------------
	// Create the process calls
	//----------------------------
	pid_t pid1;
	pid_t pid2;
	if ((pid1 = fork()) == 0){
		mergesortProcesses(&arrInfo[0]);
	}
	else if ((pid1 != 0) && (pid2 = fork()) == 0){
		mergesortProcesses(&arrInfo[1]);
	}
	else if ((pid1 != 0) && (pid2 != 0)){
		wait(NULL);
		mergeShm(inputArr -> low, inputArr -> high);
	}
}

void * mergesortRecursive(void *a){
	arrInf *inputArr = (arrInf *)a;
	int mid = (inputArr -> low + inputArr -> high) / 2;
	
	//----------------------
	// Split array in half
	// Set two threads to 
	// work on each half
	//----------------------
	arrInf arrInfo[2];
	arrInfo[0].low = inputArr -> low;
	arrInfo[0].high = mid;
	
	arrInfo[1].low = mid + 1;
	arrInfo[1].high = inputArr -> high;
	
	if (inputArr -> low >= inputArr -> high)
		return 0;
	
	//----------------------------
	// Create the recursion calls
	//----------------------------
	mergesortRecursive(&arrInfo[0]);
	mergesortRecursive(&arrInfo[1]);

	merge(inputArr -> low, inputArr -> high);
	return 0;
}

int main(){
	int shmid;

	int numOfElem;
	printf("\nPlease enter the amount of items you wish to sort: ");
	scanf("%d", &numOfElem);
	printf("\n");
	size = sizeof(int) * numOfElem;
	if((shmid = shmget(SHMKEY, sizeof(int) * 100000, IPC_CREAT | 0666)) < 0){
		perror ("shmget");
		exit(1);
	}
	int i = 0;
	int j = 0;

	time_t t;
	srand((unsigned) time(&t));
	
	if ((sharedMemArray = (shared_mem *) shmat (shmid, NULL, 0)) == (shared_mem *) -1){
      perror ("shmat");
      exit (0);
   }
   
   for (i = 0; i < numOfElem; ++i){
   	sharedMemArray -> sharedArray[i] = rand() % 100000;
   }
	
	clock_t start;
	clock_t end;
	//----------------------------------
	// Threaded merge sort analysis
	//----------------------------------
	arrInf ai;
	ai.low = 0;
	ai.high = numOfElem - 1;
	pthread_t thread;
	if (numOfElem < 1001){
	printf("Testing threaded mergesort...\n");
	start = clock();
	pthread_create(&thread, NULL, mergesortThreaded, &ai);
	pthread_join(thread, NULL);
	end = clock() - start;
	end = end * 1000 / CLOCKS_PER_SEC;

		
	printf("\nTime taken: %d seconds, %d milliseconds\n\n", end/1000, end%1000);
	}
	
	for (i = 0; i < numOfElem; ++i){
   	sharedMemArray -> sharedArray[i] = rand() % 100000;
   }
	printf("Testing processes mergesort...\n");
	start = clock();
	pid_t pid = fork();
	int ID = 0;
	if (pid == 0)
	mergesortProcesses(&ai);
	
	else{
	wait(NULL);
	end = clock() - start;
	end = end * 100 / CLOCKS_PER_SEC;
	printf("\nTime taken: %d seconds, %d milliseconds\n\n", end/1000, end%1000);
	if ((shmctl (shmid, IPC_RMID, (struct shmid_ds *) 0)) == -1)
	{
	   perror ("shmctl");
	   exit (-1);
   }
	//----------------------------------
	// Recursive merge sort analysis
	//----------------------------------
	for (i = 0; i < numOfElem; ++i){
   	sharedMemArray -> sharedArray[i] = rand() % 100000;
   }
	printf("Testing recursive mergesort...");
	
	start = clock();
	mergesortRecursive(&ai);
	end = clock() - start;
	end = end * 100 / CLOCKS_PER_SEC;
	printf("\nTime taken: %d seconds, %d milliseconds\n\n", end/1000, end%1000);
	
	
	
	//----------------------
	// Bubble sort analysis
	//----------------------
	for (i = 0; i < numOfElem; ++i){
   	sharedMemArray -> sharedArray[i] = rand() % 100000;
   }
	printf("Testing bubble sort...");
	start = clock();
	for (i = 0; i < numOfElem; ++i){
		for (j = 0; j < numOfElem - i - 1; ++j){
			if (sharedMemArray->sharedArray [j] > sharedMemArray->sharedArray[j + 1]){
				int swap = sharedMemArray->sharedArray[j];
				sharedMemArray->sharedArray[j] = sharedMemArray->sharedArray[j + 1];
				sharedMemArray->sharedArray[j + 1] = swap;
			}
		}
	}
	end = clock() - start;
	end = end * 1000 / CLOCKS_PER_SEC;
	printf("\nTime taken: %d seconds, %d milliseconds\n", end/1000, end%1000);
	
	}
	return 0;
}

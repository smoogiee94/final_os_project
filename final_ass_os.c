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
#include <sys/resource.h>
#include <semaphore.h>
#define SHMKEY ((key_t) 8941)

int threads = 0;
//---------------------------
// Structure for storing 
// array information, start
// and end indexes and 
// initialization of struct
//---------------------------
typedef struct ArrayInfo{
	int low;
	int high;
} arrInf;

typedef struct{
	int sharedArray[1000000000];
	sem_t arrSem;
} shared_mem;
shared_mem *sharedMemArray;




//--------------------------------------------------------------------
// Name:    Merge function
// Purpose: Merges a singular array with a low and high (end indexes)
//				Stores sorted merge into a a larger sorted array
//--------------------------------------------------------------------
void merge(int low, int high){

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
		
	for (i = 0; i < (high - low + 1); ++i){
		sharedMemArray->sharedArray[low + i] = temp[i];
	}
}




//--------------------------------------------
// Name: mergesortRecursive
// Purpose:  Sorts items using the recursive
//				 mergesort approach.
//--------------------------------------------
void * mergesortRecursive(void *a){

	arrInf *inputArr = (arrInf *)a;
	int mid = (inputArr -> low + inputArr -> high) / 2;	
	//----------------------
	// Split array in half
	//----------------------
	arrInf arrInfo[2];
	arrInfo[0].low = inputArr -> low;
	arrInfo[0].high = mid;
	
	arrInfo[1].low = mid + 1;
	arrInfo[1].high = inputArr -> high;
	
	if (inputArr -> low >= inputArr -> high)
		return 0;
	
	//--------------------------------------
	// Create the recursion calls O(n log n)
	//--------------------------------------
	mergesortRecursive(&arrInfo[0]);
	mergesortRecursive(&arrInfo[1]);

	//---------------------
	// Merge array O(n)
	//---------------------
	merge(inputArr -> low, inputArr -> high);
	return 0;
}



//----------------------------------------------
// Name: mergesortThreaded
// Purpose: uses parallelism to sort items
// 			each recursive call is implemented
//				on a seperate kernel thread.
//				If there are less than 10,000 items,
//				switches to recursive approach
//				to decrease overhead
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
	
	if ((inputArr -> high - inputArr -> low + 1) <= 10000 || threads >= 8){
		mergesortRecursive(a);
	}
	else{
		//-------------------
		// Create the threads
		//-------------------
		pthread_attr_t attr;
		pthread_attr_init (&attr);
		pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
		
		int thr1 = pthread_create(&thread[0], &attr, mergesortThreaded, &arrInfo[0]);
		if (thr1 > 0){
			pid_t tid1 = syscall(__NR_gettid);
			printf("Error in thread %d\n", tid1);
			printf("OS has run out of threads to allocate.\n");
			printf("Please try less items.\n\n");
			exit(0);
		}
		int thr2 = pthread_create(&thread[1], &attr, mergesortThreaded, &arrInfo[1]);
		if (thr2 > 0){
			pid_t tid2 = syscall(__NR_gettid);
			printf("Error in thread %d\n", tid2);
			printf("OS has run out of threads to allocate.\n");
			printf("Please try less items.\n\n");
			exit(0);
		}
		
		threads += 2;
		//---------------------------
		// Wait for threads to finish
		//---------------------------
		pthread_join(thread[0], NULL);
		pthread_join(thread[1], NULL);
		
		merge(inputArr -> low, inputArr -> high);
	}
}




//---------------------------------------------
// Name: mergesortProcesses
// Purpose: Completes mergesort by continuously
//			   forking children. 
//---------------------------------------------
void mergesortProcesses(void *a){
	arrInf *inputArr = (arrInf *)a;
	int mid = (inputArr -> low + inputArr -> high) / 2;
	
	//----------------------
	// Split array in half
	//----------------------
	arrInf arrInfo[2];
	arrInfo[0].low = inputArr -> low;
	arrInfo[0].high = mid;
	
	arrInfo[1].low = mid + 1;
	arrInfo[1].high = inputArr -> high;
	
	if (inputArr -> low >= inputArr -> high)
		return;
		
	if (inputArr -> high - inputArr -> low > 5000000)
		printf("Too many items to sort using processes.\n");
	if ((inputArr -> high - inputArr -> low + 1) <= 10000)
		mergesortRecursive(a);
		
	else{
	//----------------------------
	// Create the process calls
	//----------------------------
	pid_t pid1;
	pid_t pid2;
	pid1 = fork();
	if (pid1 < 0){
		exit(-1);
	}
	else if (pid1 == 0){
		mergesortProcesses(&arrInfo[0]);
		exit(0);
	}
	
	else{
		pid2 = fork();
		if (pid2 < 0){
			exit(-1);
		}
		else if (pid2 == 0){
			mergesortProcesses(&arrInfo[1]);
			exit(0);
		}
	}

	int status;
	waitpid(pid1, &status, 0);
	waitpid(pid2, &status, 0);
	merge(inputArr -> low, inputArr -> high);
	}

}

void isSorted(int n){
	int i = 0;
	for (i = 0; i < n - 1; ++i){
		if (sharedMemArray -> sharedArray[i] > sharedMemArray -> sharedArray[i + 1]){
			printf("Array not sorted\n");
			return;
		}
	}
	printf("Array is sorted\n");
	return;
}

int main(){
	int shmid;
	int numOfElem;
	int option;
	time_t t;
	srand((unsigned) time(&t));
	int i = 0;
	int j = 0;
	
	struct timeval st, et;
	
	printf("\nPlease enter the amount of items you wish to sort: ");
	scanf("%d", &numOfElem);
	printf("\n");
	
	arrInf ai;
	ai.low = 0;
	ai.high = numOfElem - 1;
	
	printf("1: Recursive Merge Sort\n");
	printf("2: Threaded Merge Sort\n");
	printf("3: Multi-Process Merge Sort\n");
	printf("4: Bubble sort\n");
	printf("Please enter the number of the algorithm you wish to use: ");
	scanf("%d", &option);
	printf("\n");
	
	//Creating shared memory segment for
	//threads and processes
	if((shmid = shmget(SHMKEY, ((sizeof(int) * 1000000000) + (sizeof(sem_t))), IPC_CREAT | 0666)) < 0){
		perror ("shmget");
		exit(1);
	}
	
	if ((sharedMemArray = (shared_mem *) shmat (shmid, NULL, 0)) == (shared_mem *) -1){
      perror ("shmat");
      exit (0);
   }
   sem_init(&(sharedMemArray->arrSem), 1, 1);
   
   //Randomize input values
   for (i = 0; i < numOfElem; ++i){
   	sharedMemArray -> sharedArray[i] = rand() % 100000;
   	//printf("%d\n", sharedMemArray -> sharedArray[i]);
   }
	
	//initialize timers
	clock_t start;
	clock_t end;
	
	if (option == 1){
		isSorted(numOfElem);
		//----------------------------------
		// Recursive merge sort analysis
		//----------------------------------
			printf("Testing recursive mergesort...\n");
			start = clock();
			gettimeofday(&st, NULL);
			mergesortRecursive(&ai);
			gettimeofday(&et, NULL);
			isSorted(numOfElem);
			printf("\nTime taken: %lu seconds, %lu milliseconds\n\n", (et.tv_sec - st.tv_sec), (et.tv_usec - st.tv_usec) * 0.001);
		//----------------------------------
		// Recursive merge sort analysis
		// complete
		//----------------------------------
	}
	
	if (option == 2){
		isSorted(numOfElem);
		//----------------------------------
		// Threaded merge sort analysis
		//----------------------------------
		pthread_t thread;
		pthread_attr_t attr; //default attributes
		pthread_attr_init (&attr);
		pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
		printf("Testing threaded mergesort...\n");
		gettimeofday(&st, NULL);
		mergesortThreaded(&ai);
		gettimeofday(&et, NULL);
		isSorted(numOfElem);
		printf("\nTime taken: %lu seconds, %lu milliseconds\n\n", (et.tv_sec - st.tv_sec), (et.tv_usec - st.tv_usec) * 0.001);

		
		//-----------------------------------
		// Threaded merge sort analysis
		// complete
		//-----------------------------------
	}

	pid_t pid = 1;
	if (option == 3){
	   //-----------------------------------
   	// Processes mergesort analysis
  	   //-----------------------------------
		printf("Testing processes mergesort...\n");
		pid = fork();
		int ID = 0;
		if (pid == 0)
		mergesortProcesses(&ai);
		
		else{
			isSorted(numOfElem);
			int status;
			gettimeofday(&st, NULL);
			waitpid(pid, &status, 0);
			gettimeofday(&et, NULL);
			isSorted(numOfElem);
			printf("\nTime taken: %lu seconds, %lu milliseconds\n\n", (et.tv_sec - st.tv_sec), (et.tv_usec - st.tv_usec) * 0.001);
		}		
		//--------------------------------------
		// Processes mergesort analysis
		// complete
		//--------------------------------------
	}
   
	if (option == 4){
		isSorted(numOfElem);
		//----------------------
		// Bubble sort analysis
		//----------------------
		printf("Testing bubble sort...\n");
		gettimeofday(&st, NULL);
		for (i = 0; i < numOfElem; ++i){
			for (j = 0; j < numOfElem - i - 1; ++j){
				if (sharedMemArray->sharedArray [j] > sharedMemArray->sharedArray[j + 1]){
					int swap = sharedMemArray->sharedArray[j];
					sharedMemArray->sharedArray[j] = sharedMemArray->sharedArray[j + 1];
					sharedMemArray->sharedArray[j + 1] = swap;
				}
			}
		}
		gettimeofday(&et, NULL);
		isSorted(numOfElem);
		printf("\nTime taken: %lu seconds, %lu milliseconds\n\n", (et.tv_sec - st.tv_sec), (et.tv_usec - st.tv_usec) * 0.001);
		//----------------------------
		// Bubble sort analysis
		// complete
		//----------------------------
		isSorted(numOfElem);
	}
		
	
	if (pid != 0){
		//freeing shared memory and exiting
		if ((shmctl (shmid, IPC_RMID, (struct shmid_ds *) 0)) == -1)
		{
	  	 perror ("shmctl");
	  	 exit (-1);
	  	}		
	}

	return 0;
}

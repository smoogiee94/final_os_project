#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


int target[] = {9, 2, 501, 293, 934, 237, 274, 83724, 391, 8493, 193, 9204, 2183, 192, 88184, 10294, 928, 17457,1049124,822};
typedef struct ArrayInfo{
	int low;
	int high;
} arrInf;

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
		if (target[left] > target[right])
			temp[curr++] = target[right++];
		else
			temp[curr++] = target[left++];
	}
	
	while(left <= mid)
		temp[curr++] = target[left++];
	
	while(right <= high)
		temp[curr++] = target[right++];
		
	for (i = 0; i < (high - low + 1); ++i)
		target[low + i] = temp[i];
}


//----------------------------------------------
// pointers to void allow
// for dynamic type casting
// in case you're a dumb fuck who is wondering
// wtf I'm doing
//----------------------------------------------
void * mergesort(void *a){
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
	int thr1 = pthread_create(&thread[0], NULL, mergesort, &arrInfo[0]);
	if (thr1 > 0)
		printf("Error creating thread 1\n");
	int thr2 = pthread_create(&thread[1], NULL, mergesort, &arrInfo[1]);
	if (thr2 > 0)
		printf("Error creating thread2\n");
	
	//---------------------------
	// Wait for threads to finish
	//---------------------------
	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);
	
	merge(inputArr -> low, inputArr -> high);
	
	pthread_exit(NULL);
}

int main(){
	arrInf ai;
	ai.low = 0;
	ai.high = sizeof(target) / sizeof(target[0]) - 1;
	pthread_t thread;
	
	clock_t start = clock();
	clock_t end;
	pthread_create(&thread, NULL, mergesort, &ai);
	pthread_join(thread, NULL);
	end = clock() - start;
	end = end * 1000 / CLOCKS_PER_SEC;
	int i;
	for (i = 0; i < ai.high + 1; ++i)
		printf("%d ", target[i]);
		
	printf("\nTime taken: %d seconds, %d milliseconds\n", end/1000, end%1000);
	return 0;
}

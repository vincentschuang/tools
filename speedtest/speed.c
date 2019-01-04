#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RETRY 			  5000000000

int gCount=0;

int main(int argc, char const *argv[])
{

	clock_t t;
	double i=0;
	int lCount=0;
	double time_taken = 0;	

	t = clock();
	for(i=0;i<RETRY;i++){
		gCount++;
	}

	t = clock() -t;
	//time_taken = ((double)t)/CLOCKS_PER_SEC;

	printf("Execution time = %ld\n", t);
	
	

	t = clock();
	for(i=0;i<RETRY;i++){
		lCount++;
	}

	t = clock() -t;

	printf("Execution time = %ld\n", t);



	int * ptr = &gCount;
	t = clock();
	for(i=0;i<RETRY;i++){
		*ptr++;
	}

	t = clock() -t;

	printf("Execution time = %ld\n", t);




	return 0;
}
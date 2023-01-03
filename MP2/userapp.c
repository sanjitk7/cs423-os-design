#include "userapp.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
long int factorial(int n) {
    if (n>=1)
        return n*factorial(n-1);
    else
        return 1;
}

int main(int argc, char *argv[])
{
	struct timeval t0, t1, st, ed;
	long long elapsed;

	int pid = getpid();
	unsigned long period = atoi(argv[1]);
	int process_time = atoi(argv[2]);
	FILE *fptr1,*fptr2,*fptr3;
	
	FILE *fptr = fopen("/proc/mp2/status", "w");
	if (fptr == NULL)
	    {
		printf("Could not open file");
		return 0;
	    }
	    
	
    	// register 
    	fprintf(fptr,"R, %u, %lu, %u",pid,period, process_time);
    	fclose(fptr);
    	
    	// reading
    	
    	
    	
    	gettimeofday(&t0, 0);
    	// call yield
    	fptr1 = fopen("/proc/mp2/status", "w");
    	fprintf(fptr1,"Y, %u",pid);
    	fclose(fptr1);
    	
    	for(int i=0;i<5;i++){
    	
    		gettimeofday(&st, 0);
    		
	    	// factorial calculations for 2 secs - each job
		time_t s = time(NULL);
	    	time_t c = s;
	    	long int ans;
	    	while(c - s < 2) { 
	    	ans = factorial(10);
		c = time(NULL);
    		}
    		
    		// job ends
    		gettimeofday(&ed, 0);
    		elapsed = (ed.tv_sec-t0.tv_sec)*1000000LL + st.tv_usec-t0.tv_usec;
    		printf("elapsed: %llu\n", elapsed);
    		
    		// call yield
    		fptr2 = fopen("/proc/mp2/status", "w");
    		fprintf(fptr2,"Y, %u",pid);
    		fclose(fptr2);
    		
    	}
    	
    	fptr3 = fopen("/proc/mp2/status", "w");
	if (fptr3 == NULL)
	    {
		printf("Could not open file");
		return 0;
	    }
    	fprintf(fptr3,"D, %u",pid);
    	fclose(fptr3);
    	
    	
    	return 0;
}


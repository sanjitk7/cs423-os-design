#include "userapp.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

long int factorial(int n) {
    if (n>=1)
        return n*factorial(n-1);
    else
        return 1;
}

int main(int argc, char* argv[])
{
	
	// writing to proc file
	int pid = getpid();
	FILE *fptr = fopen("/proc/mp1/status", "w");
	if (fptr == NULL)
	    {
		printf("Could not open file");
		return 0;
	    }
	   
	 fprintf(fptr,"%d", pid);
	 fclose(fptr);
	
	// factorial calculations for 15 secs
	time_t s = time(NULL);
    	time_t c = s;
    	long int ans;
    	while(c - s < 15) { 
    	ans = factorial(10);
        c = time(NULL);
    }
	
	
  
    
    
	return 0;
}

/* CREATED */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

main (int argc, char *argv[]) {
	int i = 0, j = 0, pid, max_loop = 595000000;

	pid = getpid ();

	while(1){
		i++;
		if(i == max_loop){
			printf("PID:%d Loop finished\n", pid);
			i = 0;
			j++;
		}
		if (j == 10) {
			printf("PID:%d Program finished\n", pid);			
			break;
		}
	}
	return 0;
}

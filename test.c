#include <stdio.h>

main (int argc, char *argv[]) {
	int i = 0;
	int j = 0;

	while(1){
		i++;
		if(i == 1000000000){
			printf("Loop finished\n");
			j++;
		}
		if (j == 1000000000) {
			printf("Program finished\n");			
			break;
		}
	}
	return 0;
}

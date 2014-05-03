/* CREATED */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#define CHUNK 1024

main (int argc, char *argv[]) {
	long checksum = 0;
	char temp;
	FILE *in, *out;
	char buf[CHUNK];
	int i,j;

	in = fopen("in", "r");
	out = fopen("out", "w");
	if (in == NULL || out == NULL) {
		printf("File Error\n");
		return 1;
	}

	for(j = 0; j < 10000000; j++) {
		while(!feof(in)){
			fread(&buf, CHUNK, 1, in);
			for(i=0; i<CHUNK ; ++i){
				checksum += buf[i];
				fwrite(&buf[i], 1, 1, out);
			}	
		}
	}
	fprintf(stdout, "%ld\n", checksum);
    fclose(in);
	fclose(out);

	return 0;
}

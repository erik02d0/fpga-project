#include <stdlib.h>    // EXIT_SUCCESS
#include <stdio.h>     // printf, sscanf
#include <time.h>      // clock_t, clock()

// Compile and run with e.g.
//     gcc fp-cpu.c && ./a.out {1..300000}

int main(int argc, char * argv[]) {
	unsigned int input_size = argc - 1;
	float * input_floats = calloc(input_size, sizeof(float));
	float val;
	for (unsigned int i = 0; i < input_size; i++) {
		int matches = sscanf(argv[i+1], "%f", &val);
		if (matches != 1) {
			fprintf(stderr, "Error reading argument %d.\n", i);
			return EXIT_FAILURE;
		}
		input_floats[i] = val;
	}

	clock_t start, end;
	double time;

	start = clock();

	float sum = input_floats[0];
	for (int i = 1; i < input_size; i++) {
		sum += input_floats[i];
	}

	end = clock();
	time = (double)(end-start)/CLOCKS_PER_SEC;

	printf("Sum: %f\nTime: %lf\n", sum, time);
	
	return EXIT_SUCCESS;
}

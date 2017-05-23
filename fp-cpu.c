#include <stdlib.h>    // EXIT_SUCCESS
#include <stdio.h>     // printf, sscanf
#include <time.h>      // clock_t, clock()

// Compile and run with e.g.
//     gcc fp-cpu.c && (for i in {1..1000000}; do echo $i; done) | ./a.out

int main(int argc, char * argv[]) {
	unsigned int array_size = 1;
	unsigned int element_count = 0;
	float * input_floats = malloc(sizeof(float[array_size]));
	float val;
	int matches;
	while (matches != EOF) {
		matches = scanf("%f", &val);
		if (matches == 1) {
			element_count++;
			if (element_count > array_size) {
				array_size *= 2;
				input_floats = realloc(input_floats, sizeof(float[array_size]));
			}
			input_floats[element_count-1] = val;
		} else if (EOF) {
			break;
		} else {  // Not a float or read error
			fprintf(stderr, "Error reading input value %d.", element_count+1);
			return EXIT_FAILURE;
		}
	}

	clock_t start, end;
	double time;

	start = clock();

	float sum = input_floats[0];
	for (int i = 0; i < element_count; i++) {
		sum += input_floats[i];
	}

	end = clock();
	time = (double)(end-start)/CLOCKS_PER_SEC;

	printf("Sum: %f\nTime: %lf\n", sum, time);
	
	return EXIT_SUCCESS;
}

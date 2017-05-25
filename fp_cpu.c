#include <stdlib.h>    // getenv, EXIT_SUCCESS
#include <stdio.h>     // printf, sscanf
#include <string.h>    // strcmp
#include <time.h>      // clock_t, clock()

// Compile and run with e.g.
//     gcc fp_cpu.c && (for i in {1..1000000}; do echo $i; done) | ./a.out

unsigned int float_to_uint32(float fval) {
	// Tricking the compiler into type casting
	// the raw data instead of converting.
	return *(unsigned int *)&fval;
}

void uint32_to_binary_string(unsigned int ival, char * buffer) {
	char bit;
	for (int i = 0; i < 32; i++) {
		bit = (ival >> (31 - i)) & 1;
		buffer[i] = '0' + bit;
	}
}

int main(int argc, char * argv[]) {
	char print_array = 0;
	char * print_array_env = getenv("print_array");
	if (print_array_env != NULL && strcmp(print_array_env, "1") == 0) {
		print_array = 1;
	}

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
			if (print_array) {
				unsigned int ival = float_to_uint32(val);
				char bitstr [32];
				uint32_to_binary_string(ival, bitstr);
				printf("A[%d] = 32'b%.32s;  // %f\n", element_count-1, bitstr, val);
			}
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
	for (int i = 1; i < element_count; i++) {
		sum += input_floats[i];
	}

	end = clock();
	time = (double)(end-start)/CLOCKS_PER_SEC;

	printf("Sum: %f\nTime: %lf\n", sum, time);
	
	return EXIT_SUCCESS;
}

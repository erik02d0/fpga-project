#include <stdlib.h>  // EXIT_SUCCESS, EXIT_FAILURE
#include <stdio.h>   // printf, sscanf

unsigned int float_to_uint32(float fval) {
	// Tricking the compiler into type casting
	// the raw data instead of converting.
	return *(unsigned int *)&fval;
}

int main(int argc, char * argv[]) {
	if (sizeof(unsigned int) != 4) {
		fprintf(stderr, "sizeof(unsigned int) != 4\n");
		return EXIT_FAILURE;
	}

	if (argc != 3) {
		fprintf(stderr, "Specify two floating point numbers as input arguments.\n");
		return EXIT_FAILURE;
	}

	float fval_a, fval_b;
	sscanf(argv[1], "%f", &fval_a);
	sscanf(argv[2], "%f", &fval_b);

	float fval_r = fval_a + fval_b;

	unsigned int ival_a = float_to_uint32(fval_a);
	unsigned int ival_b = float_to_uint32(fval_b);
	unsigned int ival_r = float_to_uint32(fval_r);

	printf(" %08x   %f\n+%08x  +%f\n=%08x  =%f\n", ival_a, fval_a, ival_b, fval_b, ival_r, fval_r);
	
	return EXIT_SUCCESS;
}

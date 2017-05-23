#include <stdlib.h>  // EXIT_SUCCESS, EXIT_FAILURE
#include <stdio.h>   // printf, sscanf

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

	char bitstr_a [32];  uint32_to_binary_string(ival_a, bitstr_a);
	char bitstr_b [32];  uint32_to_binary_string(ival_b, bitstr_b);
	char bitstr_r [32];  uint32_to_binary_string(ival_r, bitstr_r);

	printf(
		"T_A[] = 32'b%.32s;  // % f (%08x)\n"
		"T_B[] = 32'b%.32s;  // % f (%08x)\n"
		"T_R[] = 32'b%.32s;  // % f (%08x)\n",
		bitstr_a, fval_a, ival_a, bitstr_b, fval_b, ival_b, bitstr_r, fval_r, ival_r);
	
	return EXIT_SUCCESS;
}

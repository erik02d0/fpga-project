#include <inttypes.h>  // PRI* macros for *print*
#include <stdlib.h>    // EXIT_SUCCESS
#include <stdio.h>     // printf, sscanf
#include <stdint.h>    // uint32_t

uint32_t float_to_uint32(float fval) {
	// Tricking the compiler into type casting
	// the raw data instead of converting.
	return *(uint32_t *)&fval;
}

float uint32_to_float(uint32_t ival) {
	return *(float *)&ival;
}

void print_float_components(float fval) {
	uint32_t ival = float_to_uint32(fval);
	char     sign = ival >> 31;
	uint8_t  exp  = (ival >> 23) & 0xff;  // 0xff == 8 1-bits
	uint32_t frac = ival & 0x7fffff;  // 0x7fffff == 23 1-bits

	printf("%f(%08"PRIx32"): s=%hhx e=%"PRIu8"(%"PRId8")(%08"PRIx8") f=%"PRId32"(%06"PRIx32")\n",
		fval, ival, sign, exp, exp-127, exp, frac, frac);
}

unsigned int fp32add(uint32_t a, uint32_t b) {
	char     sign_a = a >> 31;
	uint8_t  exp_a  = (a >> 23) & 0xff;  // 0xff == 8 1-bits
	uint32_t frac_a = a & 0x7fffff;  // 0x7fffff == 23 1-bits

	char     sign_b = a >> 31;
	uint8_t  exp_b  = (a >> 23) & 0xff;  // 0xff == 8 1-bits
	uint32_t frac_b = a & 0x7fffff;  // 0x7fffff == 23 1-bits

	return a; // TODO: Add a and b.
}

int main(int argc, char * argv[]) {
	int input_size = argc - 1;
	uint32_t * input_ints = calloc(input_size, sizeof(uint32_t));
	for (unsigned int i = 0; i < input_size; i++) {
		float fval;
		int matches = sscanf(argv[i+1], "%f", &fval);
		if (matches != 1) {
			fprintf(stderr, "Error reading argument %d.\n", i);
			return EXIT_FAILURE;
		}
		print_float_components(fval);
		uint32_t ival = float_to_uint32(fval);
		input_ints[i] = ival;
	}

	// TODO: Start timer.
	uint32_t isum = input_ints[0];
	for (int i = 1; i < input_size; i++) {
		isum = fp32add(isum, input_ints[i]);
	}
	// TODO: Stop timer.

	float fsum = uint32_to_float(isum);
	printf("Sum: %f (%08"PRIx32")\n", fsum, isum);
	
	return EXIT_SUCCESS;
}

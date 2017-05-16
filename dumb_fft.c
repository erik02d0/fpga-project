#include <stdio.h>
#include <math.h>     /* cos, sin, M_PI */
#include <time.h>     /* clock_t, clock() */

#define TWOPI (2*M_PI)

void print_complex(float re, float im) {
  printf("Re: %10f,\tIm: %10f\n", re, im);
}

/* Not used */
void fft(int N, float *src, float *dst);

int main(int argc, char **argv) {

  int N = 16;
  if (argc > 1) {
    N = atoi(argv[1]);
    if (N%2 != 0) {
      printf("Number of inputs must be even. Exiting...\n");
      return -1;
    }
  }

  /* Mock input */
  int input[N];
  int i;
  input[0] = 1;
  for (i=1; i<N; i++) {
    input[i] = 2*input[i-1];
  }

  float E_re, E_im, O_re, O_im;
  float tw_re, tw_im;
  float re_output[N], im_output[N];
  float tmp_re, tmp_im;
  int N_2 = N/2;
  int k,m;
  clock_t start, end;

  /* Begin FFT calculation */
  start = clock();
  for (k=0; k<N_2; k++) {

    /* Calculate E_k, O_k */
    E_re = 0, E_im = 0, O_re = 0, O_im = 0;
    for (m=0; m<N_2; m++) {
      E_re += input[2*m]*cos(TWOPI*m*k/N_2); /* precompute the arg? */
      E_im -= input[2*m]*sin(TWOPI*m*k/N_2);

      O_re += input[2*m+1]*cos(TWOPI*m*k/N_2);
      O_im -= input[2*m+1]*sin(TWOPI*m*k/N_2);
    }

    /* Calculate the "twiddle factor" */
    tw_re = cos(TWOPI*k/N);
    tw_im = -sin(TWOPI*k/N);

    tmp_re = tw_re*O_re - tw_im*O_im;
    tmp_im = tw_re*O_im + tw_im*O_re;

    /* Calculate result */
    re_output[k] = E_re + tmp_re;
    im_output[k] = E_im + tmp_im;
    re_output[k+N_2] = E_re - tmp_re;
    im_output[k+N_2] = E_im - tmp_im;
  }
  end = clock();

  printf("Results:\n");
  for (k=0; k<N; k++)
    print_complex(re_output[k], im_output[k]);

  printf("\nTime: %lf\n", (double)(end-start)/CLOCKS_PER_SEC);

  return 0;
}

/*
 * Precalculate some constants, load values,
 * perform computations, store results.
 */
void fft(int N, float *src, float *dst) {

}

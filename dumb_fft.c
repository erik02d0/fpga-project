#include <stdio.h>
#include <math.h>     /* cos, sin, M_PI */
#include <time.h>     /* clock_t, clock() */

void fft(int N, int *src, float *re_dst, float *im_dst);

void naive_fft(int N, int *src, float *re_dst, float *im_dst);

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
  for (i=1; i<N; i++)
    input[i] = 2*input[i-1];

  float output[2*N];	/* Eventually the result should be in a single array */
  float im_sol[N];		/* Store imaginary parts here for now */
  output[0] = 0, output[N/2] = 0;
  
  /* Time etc */
  clock_t start, end;
  float time, ref_time, re_err, im_err;
  float cmp_re_out[N], cmp_im_out[N];

  /* Reference calculation */
  start = clock();
  naive_fft(N, input, cmp_re_out, cmp_im_out);
  end = clock();
  ref_time = (float)(end-start)/CLOCKS_PER_SEC;

  /* FFT calculation */
  start = clock();
  fft(N, input, output, im_sol);
  end = clock();
  time = (float)(end-start)/CLOCKS_PER_SEC;

  /* Print results */
  printf("\nResults: (errors in parentheses)\n\n");
  for (i=0; i<N; i++) {
  	re_err = output[i]-cmp_re_out[i];
  	im_err = im_sol[i]-cmp_im_out[i];
    printf("Re: %16f (%9f)\tIm: %16f (%9f)\n",
    	output[i], re_err, im_sol[i], im_err);
  }
  printf("\nTime, ref time: %lf, %lf (%.1f times faster)\n\n",
  	time, ref_time, ref_time/time);

  return 0;
}

/*
 * Precalculate some constants, load values,
 * perform computations, store results.
 */
void fft(int N, int *src, float *re_dst, float *im_dst) {

  const float TWOPI = 2*M_PI;	/* This is "hardware" */
  const int N_2 = N/2;
  float E_re, E_im, O_re, O_im;
  float tw_re, tw_im;
  float tmp_re, tmp_im;
  int k, m;

  float ftmp;

  /* The special cases k=0, k=N/2 */
  for (m=0; m<N-1; m+=2) {
    re_dst[0] += src[m]+src[m+1];
    re_dst[N_2] += src[m]-src[m+1];
  }
  im_dst[0] = 0, im_dst[N_2] = 0;

  /* Remaining cases */
  for (k=1; k<N_2; k++) {

    ftmp = TWOPI*k/N;
    E_re = 0, E_im = 0, O_re = 0, O_im = 0;

    for (m=0; m<N_2; m++) {
      E_re += src[2*m]*cos(2*m*ftmp);
      E_im -= src[2*m]*sin(2*m*ftmp);

      O_re += src[2*m+1]*cos(2*m*ftmp);
      O_im -= src[2*m+1]*sin(2*m*ftmp);
    }

    tw_re = cos(ftmp);
    tw_im = -sin(ftmp);

    tmp_re = tw_re*O_re - tw_im*O_im;
    tmp_im = tw_re*O_im + tw_im*O_re;

    re_dst[k] = E_re + tmp_re;
    im_dst[k] = E_im + tmp_im;
    re_dst[k+N_2] = E_re - tmp_re;
    im_dst[k+N_2] = E_im - tmp_im;
  }
}

void naive_fft(int N, int *src, float *re_dst, float *im_dst) {
	int k,m;
	for (k=0; k<N; k++) {
		re_dst[k] = 0;
		im_dst[k] = 0;
		for (m=0; m<N; m++) {
			re_dst[k] += src[m]*cos(2*M_PI*k*m/N);
			im_dst[k] -= src[m]*sin(2*M_PI*k*m/N);
		}
	}
}
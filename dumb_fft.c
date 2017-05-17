#include <stdio.h>
#include <math.h>     /* cos, sin */
#include <time.h>     /* clock_t, clock() */

void fft(int N, int *src, double *re_dst, double *im_dst);

void naive_fft(int N, int *src, double *re_dst, double *im_dst);

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

  double output[2*N];	/* Eventually the result should be in a single array */
  double im_sol[N];		/* Store imaginary parts here for now */
  output[0] = 0, output[N/2] = 0;
  
  /* Time etc */
  clock_t start, end;
  double time, ref_time, re_err, im_err;
  double cmp_re_out[N], cmp_im_out[N];

  /* Reference calculation */
  start = clock();
  naive_fft(N, input, cmp_re_out, cmp_im_out);
  end = clock();
  ref_time = (double)(end-start)/CLOCKS_PER_SEC;

  /* FFT calculation */
  start = clock();
  fft(N, input, output, im_sol);
  end = clock();
  time = (double)(end-start)/CLOCKS_PER_SEC;

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

/* Cooley-Tukey FFT accepting even-length
 * sequence of integers. */
void fft(int N, int *src, double *re_dst, double *im_dst) {

  /* Constant */
  const double TWOPI = 2*3.14159265358979323846;
  
  /* Registers */
	int k, m;
	double E_re, E_im, O_re, O_im;
  double tw_re, tw_im;	/* twiddle factor */
  double tmp_re, tmp_im;
  double c_curr_outer, c_curr_inner, c_tmp;
  double s_curr_outer, s_curr_inner, s_tmp;
  double c_init = 1; /* cos(0*4pi/N) */
  double s_init = 0; /* sin(0*4pi/N) */
  double tw_re_init = 1;	/* cos(0*2pi/N) */
  double tw_im_init = 0;	/* sin(0*2pi/N) */
  
  /* Input-dependent constants */
  const int N_2 = N/2;
  const double TWOPI_N = TWOPI/N;
  const double c2pi_N = cos(TWOPI_N);
  const double s2pi_N = sin(TWOPI_N);
	const double c4pi_N = c2pi_N*c2pi_N - s2pi_N*s2pi_N;
	const double s4pi_N = 2*c2pi_N*s2pi_N;
	
  /* The special cases k=0, k=N/2 */
  for (m=0; m<N-1; m+=2) {
    re_dst[0] += src[m]+src[m+1];
    re_dst[N_2] += src[m]-src[m+1];
  }
  im_dst[0] = 0, im_dst[N_2] = 0;

  /* Remaining cases */
  for (k=1; k<N_2; k++) {

    /* Update outer and inner */
    c_curr_outer = c_init*c4pi_N - s_init*s4pi_N;
    s_curr_outer = s_init*c4pi_N + c_init*s4pi_N;
    c_curr_inner = c_curr_outer, s_curr_inner = s_curr_outer;
    c_init = c_curr_outer, s_init = s_curr_outer;
		
    /* Initialize E,O summands */
		E_re = src[0], E_im = 0, O_re = src[1], O_im = 0;

    for (m=1; m<N_2; m++) {

    	/* Calculate E,O summands */
      E_re += src[2*m]*c_curr_inner;
			E_im -= src[2*m]*s_curr_inner;
			O_re += src[2*m+1]*c_curr_inner;
			O_im -= src[2*m+1]*s_curr_inner;

			/* Update inner */
			c_tmp = c_curr_inner, s_tmp = s_curr_inner;
			c_curr_inner = c_tmp*c_curr_outer - s_tmp*s_curr_outer;
			s_curr_inner = s_tmp*c_curr_outer + c_tmp*s_curr_outer;
    }
    
		/* Update twiddle factor */
    tw_re = tw_re_init*c2pi_N - (-tw_im_init)*s2pi_N;
    tw_im = -((-tw_im_init)*c2pi_N + tw_re_init*s2pi_N);
    tw_re_init = tw_re, tw_im_init = tw_im;
    
    /* Multiply O by twiddle factor */
    tmp_re = tw_re*O_re - tw_im*O_im;
    tmp_im = tw_re*O_im + tw_im*O_re;

    /* Calculate final result */
    re_dst[k] = E_re + tmp_re;
    im_dst[k] = E_im + tmp_im;
    re_dst[k+N_2] = E_re - tmp_re;
    im_dst[k+N_2] = E_im - tmp_im;
  }
}

void naive_fft(int N, int *src, double *re_dst, double *im_dst) {
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
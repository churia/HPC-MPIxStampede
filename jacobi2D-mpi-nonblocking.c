/* MPI-parallel Jacobi smoothing to solve -u''=f
 * Global vector has N unknowns, each processor works with its
 * part, which has lN = N/p unknowns.
 * Author: Georg Stadler
 */
#include <stdio.h>
#include <math.h>
#include <mpi.h>
#include "util.h"
#include <string.h>
#define epsilon 1e-5

/* compuate global residual, assuming ghost values are updated */
double compute_residual(double *lu, int M, double hsq)
{
  int i;
  double tmp, gres = 0.0, lres = 0.0;

  for (i = M + 1; i <= M * (M-1) - 2; i++){
    if(i % M != 0 && i % M != M-1 ){//pass border points!
      tmp = ((4.0 * lu[i] - lu[i-1] - lu[i+1]- lu[i-M] - lu[i+M]) / hsq - 1);
      lres += tmp * tmp;
    }
  }
  /* use allreduce for convenience; a reduce would also be sufficient */
  MPI_Allreduce(&lres, &gres, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  return sqrt(gres);
}


int main(int argc, char * argv[])
{
  int mpirank, i, j,t, p, sp, N, lN, M, iter, max_iters;
  MPI_Status status, status1, status2, status3;
  MPI_Request request_out1, request_in1;
  MPI_Request request_out2, request_in2;
  MPI_Request request_out3, request_in3;
  MPI_Request request_out4, request_in4;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpirank);
  MPI_Comm_size(MPI_COMM_WORLD, &p);

  /* get name of host running MPI process */
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  int name_len;
  MPI_Get_processor_name(processor_name, &name_len);
  printf("Rank %d/%d running on %s.\n", mpirank, p, processor_name);

  sscanf(argv[1], "%d", &N);
  sscanf(argv[2], "%d", &max_iters);

  /* compute number of unknowns handled by each process */
  double logp = log(p)/log(4);
  sp = (int) sqrt(p);
  if ((logp-(int)logp !=0 || N % sp != 0 ) && mpirank == 0 ) {
    printf("N: %d, p: %d\n", N, p);
    printf("Exiting. log4(p) must be an integer and N must be multiple of (2^j)\n");
    MPI_Abort(MPI_COMM_WORLD, 0);
  }
  /* timing */
  MPI_Barrier(MPI_COMM_WORLD);
  timestamp_type time1, time2;
  get_timestamp(&time1);
  lN = N / sp;
  M = lN + 2;

  /* Allocation of vectors, including left/upper and right/lower ghost points */
  double * lu    = (double *) calloc(sizeof(double), M * M);
  double * lunew = (double *) calloc(sizeof(double), M * M);
  double * lutemp;
  double * leftsend = (double *) calloc(sizeof(double), lN);
  double * leftrecv = (double *) calloc(sizeof(double), lN);
  double * rightsend = (double *) calloc(sizeof(double), lN);
  double * rightrecv = (double *) calloc(sizeof(double), lN);

  double h = 1.0 / (N + 1);
  double hsq = h * h;
  double gres, gres0;

  /* initial residual */
  gres0 = compute_residual(lu, M, hsq);
  gres = gres0;
  if (mpirank == 0)
    printf("Iter 0 Residual %g\n", gres0);

  for (iter = 1; iter <= max_iters && gres > epsilon*gres0; iter++) {

    /* communicate ghost values */
    if (mpirank < p - sp) {
      /* If not the up bdry processes, send/recv bdry values to the up */
      for (j = 1; j <= lN; j++){
        t = (M-2) * M + j;
        lunew[t]  = 0.25 * (hsq + lu[t-M] + lu[t-1] + lu[t+1] + lu[t+M]);
      }
      MPI_Isend(&(lunew[(M-2)*M+1]), lN, MPI_DOUBLE, mpirank+sp, 121, MPI_COMM_WORLD, &request_out1);
      MPI_Irecv(&(lunew[(M-1)*M+1]), lN, MPI_DOUBLE, mpirank+sp, 122, MPI_COMM_WORLD, &request_in1);
    }
    if (mpirank >= sp) {
      /* If not the bottom bdry processes, send/recv bdry values to the below */
      for (j = 1; j <= lN; j++){
        t = M + 1 + j;
        lunew[t]  = 0.25 * (hsq + lu[t-M] + lu[t-1] + lu[t+1] + lu[t+M]);
      }
      MPI_Isend(&(lunew[M+1]), lN, MPI_DOUBLE, mpirank-sp, 122, MPI_COMM_WORLD, &request_out2);
      MPI_Irecv(&(lunew[1]), lN, MPI_DOUBLE, mpirank-sp, 121, MPI_COMM_WORLD, &request_in2);
    }
    if (mpirank % sp !=0){
      /* If not the left bdry processes, send/recv bdry values to the left */
      for (j = 1; j <= lN; j++){
        t = j * M + 1;
        lunew[t]  = 0.25 * (hsq + lu[t-M] + lu[t-1] + lu[t+1] + lu[t+M]);
        leftsend[j-1] = lunew[j*M+1];
      }
      MPI_Isend(leftsend, lN, MPI_DOUBLE, mpirank-1, 123, MPI_COMM_WORLD, &request_out3);
      MPI_Irecv(leftrecv, lN, MPI_DOUBLE, mpirank-1, 124, MPI_COMM_WORLD, &request_in3);
    }
    if (mpirank % sp != sp - 1){
       /* If not the right bdry processes, send/recv bdry values to the right */
      for (j = 1; j <= lN; j++){
        t = j * M + lN;
        lunew[t]  = 0.25 * (hsq + lu[t-M] + lu[t-1] + lu[t+1] + lu[t+M]);
        rightsend[j-1] = lunew[t];
      }
      MPI_Isend(rightsend, lN, MPI_DOUBLE, mpirank+1, 124, MPI_COMM_WORLD, &request_out4);
      MPI_Irecv(rightrecv, lN, MPI_DOUBLE, mpirank+1, 123, MPI_COMM_WORLD, &request_in4);
    }

    /* Jacobi step for local points */
    for (i = 2 * M + 2; i < M * (M-2) - 2; i++){
      if(lN > 2 && i % M != 0 && i % M != M-1 && i % M != 1 && i % M != M-2) //pass border points!
        lunew[i]  = 0.25 * (hsq + lu[i-M] + lu[i-1] + lu[i+1] + lu[i+M]);
    }

    /* check if Isend/Irecv are done */
    if (mpirank < p - sp) {
      MPI_Wait(&request_out1, &status);
      MPI_Wait(&request_in1, &status);
    }
    if (mpirank >= sp) {
      MPI_Wait(&request_out2, &status);
      MPI_Wait(&request_in2, &status);
    }
    if (mpirank % sp !=0){
      MPI_Wait(&request_out3, &status);
      MPI_Wait(&request_in3, &status);
      for (j = 1; j <= lN; j++){
        lunew[j*M] = leftrecv[j-1];
      }
    }
    if (mpirank % sp != sp - 1){
      MPI_Wait(&request_out4, &status);
      MPI_Wait(&request_in4, &status);
      for (j = 1; j <= lN; j++){
        lunew[j*M+lN+1] = rightrecv[j-1];
      }
    }
    /* copy newu to u using pointer flipping */
    lutemp = lu; lu = lunew; lunew = lutemp;
    if (0 == (iter % 10)) {
      gres = compute_residual(lu, M, hsq);
      if (0 == mpirank) {
	printf("Iter %d: Residual: %g\n", iter, gres);
      }
    }
  }

  /* Clean up */
  free(lu);
  free(lunew);
  free(leftsend);
  free(leftrecv);
  free(rightsend);
  free(rightrecv);

  /* timing */
  MPI_Barrier(MPI_COMM_WORLD);
  get_timestamp(&time2);
  double elapsed = timestamp_diff_in_seconds(time1,time2);
  if (0 == mpirank) {
    printf("Time elapsed is %f seconds.\n", elapsed);
  }
  MPI_Finalize();
  return 0;
}

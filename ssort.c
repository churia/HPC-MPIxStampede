/* Parallel sample sort
 */
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>
#include <stdlib.h>
#include "util.h"

static int compare(const void *a, const void *b)
{
  int *da = (int *)a;
  int *db = (int *)b;

  if (*da > *db)
    return 1;
  else if (*da < *db)
    return -1;
  else
    return 0;
}

int main( int argc, char *argv[])
{
  int rank,p,root=0;
  long i, j, N, s,rsize;
  int *vec, *sample, *rbuf, *split;
  int *scounts, *rcounts, *sdispls, *rdispls, *rray;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &p);

  /* get name of host running MPI process */
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  int name_len;
  MPI_Get_processor_name(processor_name, &name_len);
  printf("Rank %d/%d running on %s.\n", rank, p, processor_name);

  /* timing */
  MPI_Barrier(MPI_COMM_WORLD);
  timestamp_type time1, time2;
  get_timestamp(&time1);

  /* Number of random numbers per processor (this should be increased
   * for actual tests or could be passed in through the command line */
  sscanf(argv[1], "%ld", &N);
  s = N/p;
  if(s*p<N) //sample 1 more for scaling
    s+=1;
  vec = calloc(N, sizeof(int));
  /* seed random number generator differently on every core */
  srand((unsigned int) (rank + 393919));

  /* fill vector with random integers */
  for (i = 0; i < N; ++i) {
    vec[i] = (int)rand();
  }
  printf("rank: %d, first entry: %d\n", rank, vec[0]);
  //for (i = 0; i < N; ++i) {
  //printf("rank: %d, %d\n", rank, vec[i]);
  //}
  /* sort locally */
  qsort(vec, N, sizeof(int), compare);

  /* randomly sample s entries from vector or select local splitters,
   * i.e., every N/P-th entry of the sorted vector */
  sample = (int *) calloc(s, sizeof(int));
  for (i = 1; i <= s; i++){
    sample[i-1] = vec[i*N/(s+1)];
  }

  /* every processor communicates the selected entries
   * to the root processor; use for instance an MPI_Gather */
  if (rank == root){
    rbuf = (int *) calloc(s*p, sizeof(int));
  }
  
  MPI_Gather(sample, s, MPI_INT, rbuf, s, MPI_INT, root, MPI_COMM_WORLD);

  MPI_Barrier(MPI_COMM_WORLD);
  /* root processor does a sort, determinates splitters that
   * split the data into P buckets of approximately the same size */
  split = (int *) calloc(p-1, sizeof(int));
  if (rank == root){
    printf("finish sampling.. length of sampled vector: %ld\n",s*p);
    //for (i = 0; i< s*p; i++)
    //  printf("%d ", rbuf[i]);
    //printf("\n");
    qsort(rbuf, s*p, sizeof(int), compare);
    //for (i = 0; i< s*p; i++)
    //  printf("%d ", rbuf[i]);
    //printf("\n");
    for(i = 1; i < p; i++){
      split[i-1] = rbuf[i*s];
    }
    printf("bcast splitters..\n");
    //for(i=0;i<p-1;i++)
    //  printf("%d ",split[i]);
    //printf("\n");
  }

  /* root process broadcasts splitters */
  MPI_Bcast(split, p-1, MPI_INT, root, MPI_COMM_WORLD);

  /* every processor uses the obtained splitters to decide
   * which integers need to be sent to which other processor (local bins) */
  scounts = (int *) calloc(p, sizeof(int));
  rcounts = (int *) calloc(p, sizeof(int));
  sdispls = (int *) calloc(p, sizeof(int));
  rdispls = (int *) calloc(p, sizeof(int));

  /*decide the number of data to send to each processor based on splitters*/
  j = 0;
  for(i = 0; i < N; i++){
    if(vec[i] <= split[j])
      scounts[j]++;
    else{
      while(vec[i] > split[j]&&j<p-1)
	j++;
      scounts[j]++;
    }
  }
  //for(i = 0; i < N; i++){
  //   printf("rank %d: send %d to %d\n", rank,scounts[i],i);
  //}
  /* tell the other processors how much data is coming */
  MPI_Alltoall(scounts, 1, MPI_INT, rcounts, 1, MPI_INT, MPI_COMM_WORLD);
  /* calculate displacements and the size of the arrays */
  sdispls[0] = 0;
  for(i = 1; i < p; i++){
    sdispls[i] = scounts[i-1] + sdispls[i-1];
  }
  rdispls[0] = 0;
  for(i = 1; i < p; i++){
    rdispls[i] = rcounts[i-1] + rdispls[i-1];
  }
  
  rsize = 0;
  for(i = 0; i < p; i++){
    rsize += rcounts[i];
  }
  
  rray = (int *) calloc(rsize, sizeof(int));
  MPI_Alltoallv(vec, scounts,sdispls,MPI_INT,
                rray,rcounts,rdispls,MPI_INT, MPI_COMM_WORLD);

  /* do a local sort */
  int *temp = vec; vec = rray; rray = temp;
  qsort(vec, rsize, sizeof(int), compare);

  /* every processor writes its result to a file */
  FILE* fd = NULL;
  char filename[256];
  snprintf(filename, 256, "output%03d.txt", rank);
  fd = fopen(filename,"w+");

  if(NULL == fd)
  {
    printf("Error opening file \n");
    return 1;
  }

  for(i = 0; i < rsize; ++i)
    fprintf(fd, "%d ", vec[i]);
  
  fprintf(fd,"\n");
  fclose(fd);

  /*clean*/
  free(vec);
  free(sample);
  free(split);
  free(scounts);free(rcounts); free(sdispls); free(rdispls); 
  free(rray);
  if (rank == root){
    free(rbuf);
  }

  /* timing */
  MPI_Barrier(MPI_COMM_WORLD);
  get_timestamp(&time2);
  double elapsed = timestamp_diff_in_seconds(time1,time2);
  if (0 == rank) {
    printf("Time elapsed is %f seconds.\n", elapsed);
  }

  MPI_Finalize();
  return 0;
}

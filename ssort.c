/* Parallel sample sort
 */
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>
#include <stdlib.h>


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
  int rank,root=0;
  int i, j, N;
  int *vec, *sample, *rbuf, *split;
  int *scounts, *rcounts, *sdispls, *rdispls, *rray;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &p);

  /* Number of random numbers per processor (this should be increased
   * for actual tests or could be passed in through the command line */
  sscanf(argv[1], "%d", &N);

  vec = calloc(N, sizeof(int));
  /* seed random number generator differently on every core */
  srand((unsigned int) (rank + 393919));

  /* fill vector with random integers */
  for (i = 0; i < N; ++i) {
    vec[i] = rand();
  }
  printf("rank: %d, first entry: %d\n", rank, vec[0]);

  /* sort locally */
  qsort(vec, N, sizeof(int), compare);

  /* randomly sample s entries from vector or select local splitters,
   * i.e., every N/P-th entry of the sorted vector */
  sample = (int *) calloc(N/p, sizeof(int));
  for (i = 0; i < N/p; i++){
    sample[i] = vec[i*p];
  }

  /* every processor communicates the selected entries
   * to the root processor; use for instance an MPI_Gather */
  if (rank == root){
    rbuf = (int *) calloc(N, sizeof(int));
  }
  MPI_Gather(sample, N/p, MPI_INT, rbuf, N/p, MPI_INT, root, MPI_COMM_WORLD);

  /* root processor does a sort, determinates splitters that
   * split the data into P buckets of approximately the same size */
  if (rank == root){
    qsort(rbuf, N, sizeof(int), compare);
    split = (int *) calloc(p-1, sizeof(int));
    for(i = 1; i < p; i++){
      split[i-1] = rbuf[i*p];
    }
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
    if(vec[i] < split[j])
      scounts[j]++;
    else
      scounts[++j]++;
  }

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
  
  int rsize = 0;
  for(i = 0; i < p; i++){
    rsize += rcounts[i];
  }
  
  rray = (int *) calloc(rsize, sizeof(int));
  MPI_Alltoallv(vec, scounts,sdisp,MPI_INT,
                rray,rcounts,rdisp,MPI_INT, MPI_COMM_WORLD);

  /* do a local sort */
  int *temp = vec; vec = rray; rray = vec;
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

  for(i = 0; i < N; ++i)
    fprintf(fd, "%d ", vec[n]);
  
  fprintf(fd,"\n");
  fclose(fd);

  /*finalize*/
  free(vec);
  free(sample);
  free(split);
  free(scounts);free(rcounts); free(sdispls); free(rdispls); 
  free(rray);
  if (rank == root){
    free(rbuf);
  }
  MPI_Finalize();
  return 0;
}

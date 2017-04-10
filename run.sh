#!/bin/bash

module load intel-2016
make
mpirun -np 4 ./mpi_solved1
mpirun -np 4 ./mpi_solved2
mpirun -np 4 ./mpi_solved3
mpirun -np 4 ./mpi_solved4
mpirun -np 4 ./mpi_solved5
mpirun -np 4 ./mpi_solved6
mpirun -np 4 ./mpi_solved7
make clean

module load mpich-x86_64
make
mpirun -np 4 ./mpi_solved1
mpirun -np 4 ./mpi_solved2
mpirun -np 4 ./mpi_solved3
mpirun -np 4 ./mpi_solved4
mpirun -np 4 ./mpi_solved5
mpirun -np 4 ./mpi_solved6
mpirun -np 4 ./mpi_solved7
make clean

module load openmpi-x86_64
make
mpirun -np 4 ./mpi_solved1
mpirun -np 4 ./mpi_solved2
mpirun -np 4 ./mpi_solved3
mpirun -np 4 ./mpi_solved4
mpirun -np 4 ./mpi_solved5
mpirun -np 4 ./mpi_solved6
mpirun -np 4 ./mpi_solved7
make clean

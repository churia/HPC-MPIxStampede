CC = mpicc
FLAGS = -O3 -Wall
TARGET = mpi_solved1 mpi_solved2 mpi_solved3 mpi_solved4 mpi_solved5 mpi_solved6 mpi_solved7

all: $(TARGET)

%: %.c
	$(CC) $(FLAGS) $< -o $@

clean:
	rm $(TARGET)

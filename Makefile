CC = mpicc
FLAGS = -O3 -Wall -lm
TARGET = mpi_solved1 mpi_solved2 mpi_solved3 mpi_solved4 mpi_solved5 mpi_solved6 mpi_solved7 jacobi2D-mpi ssort jacobi2D-mpi-nonblocking report.pdf

all: $(TARGET)

%: %.c
	$(CC) $(FLAGS) $< -o $@

report.pdf:
	pdflatex report.tex

clean:
	rm $(TARGET)

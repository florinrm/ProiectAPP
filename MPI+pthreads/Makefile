build: main
main: main.c
	mpicc -o main main.c -lm -Wall -lpthread
serial: main
	mpirun -np 1 main imagini.in
distrib: main
	mpirun -np 4 main imagini.in
clean:
	rm -f main

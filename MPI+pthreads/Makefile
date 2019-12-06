build: main
main: main.c
	mpicc -o main main.c -lm -Wall -lpthread
run: main
	mpirun -np 8 main in/lenna_color.pnm blur.pnm 8
clean:
	rm -f main

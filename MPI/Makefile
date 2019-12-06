build: main
main: main.c
	mpicc -o main main.c -lm -Wall
run: main
	mpirun -np 8 main in/lenna_color.pnm blur.pnm
clean:
	rm -f main

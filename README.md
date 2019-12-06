# Proiect APP - Blurring Photos

## How to build
In every folder there is a Makefile. For compiling, type `make` or `make build`.

## How to run
### MPI
Example: `mpirun -np <no_processes> main in/lenna_color.pnm blurred.pnm`

### pthreads
Example: `./main in/lenna_color.pnm blur.pnm <no_threads>`

### OpenMP
Example: 
`./main in/lenna_color.pnm blur.pnm <no_threads>`

### MPI + OpenMP | MPI + pthreads
Example: 
`mpirun -np <no_processes> main in/lenna_color.pnm blurred.pnm <no_threads>`

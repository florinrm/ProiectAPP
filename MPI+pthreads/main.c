#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <pthread.h>

#define COLOR 6
#define GRAYSCALE 5

float blurMatrix[3][3] = {{1.f / 16, 2.f / 16, 1.f / 16}, 
                        {2.f / 16, 4.f / 16, 2.f / 16}, 
                        {1.f / 16, 2.f / 16, 1.f / 16}};

// typedef enum filter {BLUR, SHARPEN, MEAN, EMBOSS, SMOOTH} filter;

typedef struct {
    unsigned char red, green, blue;
} rgb; 

typedef struct {
    unsigned char gray;
} gray; // grayscale image

typedef struct {
    int width, height, max_size;
    int type; // image type
    rgb **color_image; // 5 type
    gray **gray_image; // 6 type
} image;

typedef struct {
    int first;
    int second;
    int third;
} tuple;

image input, output;
const int no_threads = 8;

void readInput(const char * fileName) {
    
    FILE *file = fopen(fileName, "rb");
    if (file == NULL)
        return;

    image *buff; // reference used for creating the original image

    char buffRead[2];
    fscanf(file, "%c %c", &buffRead[0], &buffRead[1]);
    input.type = buffRead[1] - '0'; // from char to int
    fscanf(file, "%d %d\n%d\n", &input.width, &input.height, &input.max_size);

    if (input.type == COLOR) {
        input.color_image = (rgb**) malloc (input.height * sizeof(rgb*));
        for (int i = 0; i < input.height; ++i)
            input.color_image[i] = (rgb *) malloc (input.width * sizeof(rgb));
        for (int i = 0; i < input.height; ++i)
            fread(input.color_image[i], sizeof(rgb), input.width, file);

    } else if (input.type == GRAYSCALE) {
        input.gray_image = (gray**) malloc (input.height * sizeof(gray*));
        for (int i = 0; i < input.height; ++i)
            input.gray_image[i] = (gray *) malloc (input.width * sizeof(gray));
        for (int i = 0; i < input.height; ++i)
            fread(input.gray_image[i], sizeof(gray), input.width, file);
    }

    fclose(file);
}

void writeData(const char * fileName) {

    FILE *file = fopen(fileName, "wb");
    if (file == NULL)
        return;
    fprintf(file, "P%d\n%d %d\n%d\n", output.type, output.width, output.height, output.max_size);
    if (output.type == COLOR) {
        for (int i = 0; i < output.height; ++i) { 
            
            for (int j = 0; j < output.width; ++j) {
                fwrite(&output.color_image[i][j].red, sizeof(unsigned char), 1, file);
                fwrite(&output.color_image[i][j].green, sizeof(unsigned char), 1, file);
                fwrite(&output.color_image[i][j].blue, sizeof(unsigned char), 1, file);
            }
        }
    } else if (output.type == GRAYSCALE) {
        for (int i = 0; i < output.height; ++i)
            for (int j = 0; j < output.width; ++j)
                fwrite(&output.gray_image[i][j], sizeof(unsigned char), 1, file);
    }

    fclose(file);

    if (output.type == COLOR) {
        for (int i = 0; i < output.height; ++i)
            free(output.color_image[i]);
        free(output.color_image);
    } else if (output.type == GRAYSCALE) {
        for (int i = 0; i < output.height; ++i)
            free(output.gray_image[i]);
        free(output.gray_image);
    }
}

int min (int a, int b) {
    return a <= b ? a : b;
}

void* threadFunction (void* var) {
    tuple p = *(tuple*) var;
    int rank = p.first, proc = p.second, id = p.third;

    int start = id * ceil((double) output.width / (double) no_threads);
	int end = min(((id + 1) * ceil((double) output.width / (double) no_threads)), output.width);

    for (int i = rank; i < output.height; i += proc) {
        for (int j = start; j < end; ++j) {
            if (input.type == COLOR) {                
                if (i == 0 || i == output.height - 1 || j == 0 || j == output.width - 1) {
                    output.color_image[i][j].red = input.color_image[i][j].red;
                    output.color_image[i][j].green = input.color_image[i][j].green;
                    output.color_image[i][j].blue = input.color_image[i][j].blue;
                } else {
                    float red = 0, green = 0, blue = 0;
                    for (int x = i - 1; x <= i + 1; ++x) {
                        for (int y = j - 1; y <= j + 1; ++y) {
                            red += input.color_image[x][y].red * blurMatrix[x - (i - 1)][y - (j - 1)];
                            green += input.color_image[x][y].green * blurMatrix[x - (i - 1)][y - (j - 1)];
                            blue += input.color_image[x][y].blue * blurMatrix[x - (i - 1)][y - (j - 1)];
                        }
                    }
                    output.color_image[i][j].red = (u_char) red;
                    output.color_image[i][j].green = (u_char) green;
                    output.color_image[i][j].blue = (u_char) blue;
            
                }

            } else if (input.type == GRAYSCALE) {
                if (i == 0 || i == output.height - 1 || j == 0 || j == output.width - 1) {
                    output.gray_image[i][j].gray = input.gray_image[i][j].gray;
                } else {
                    float gray = 0;
                    for (int x = i - 1; x <= i + 1; ++x) {
                        for (int y = j - 1; y <= j + 1; ++y) {
                            gray += input.gray_image[x][y].gray * blurMatrix[x - (i - 1)][y - (j - 1)];
                        }
                    }
                    output.gray_image[i][j].gray = (u_char) gray;
                }
            }
        }
    }
}

void applyFilter (int rank, int proc) {
    output.type = input.type;
    output.height = input.height;
    output.width = input.width;
    output.max_size = input.max_size;

    if (output.type == COLOR) {
        output.color_image = (rgb **) malloc (output.height * sizeof(rgb *));
        for (int i = 0; i < output.height; ++i)
            output.color_image[i] = (rgb *) malloc (output.width * sizeof(rgb));
    } else if (output.type == GRAYSCALE) {
        output.gray_image = (gray **) malloc (output.height * sizeof(gray *));
        for (int i = 0; i < output.height; ++i)
            output.gray_image[i] = (gray *) malloc (output.width * sizeof(gray));
    }

    tuple tid[no_threads];
    pthread_t threads[no_threads];

    for (int i = 0; i < no_threads; i++) {
        tid[i].first = rank;
        tid[i].second = proc;
        tid[i].third = i;
    }

    for (int i = 0; i < no_threads; ++i) {
		pthread_create(&(threads[i]), NULL, threadFunction, &(tid[i]));
	}

	for (int i = 0; i < no_threads; ++i) {
		pthread_join(threads[i], NULL);
	}

}

void imageProcessing (int rank, int processes) {
    applyFilter(rank, processes);  
    if (rank != 0) {
        for (int j = rank; j < output.height; j += processes) {
            if (input.type == COLOR) {
                MPI_Send(output.color_image[j], output.width * 3, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            } else {
                MPI_Send(output.gray_image[j], output.width, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }
        }
    } else {
        for (int x = 1; x < processes; ++x) {
            for (int j = x; j < output.height; j += processes) {
                if (input.type == COLOR) {
                    MPI_Recv(output.color_image[j], output.width * 3, MPI_CHAR, x, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                } else {
                    MPI_Recv(output.gray_image[j], output.width, MPI_CHAR, x, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }
        }
    }

    if (rank == 0) {
        for (int x = 1; x < processes; ++x) {
            for (int j = 0; j < output.height; ++j) {
                if (input.type == COLOR) {
                    MPI_Send(output.color_image[j], output.width * 3, MPI_CHAR, x, 0, MPI_COMM_WORLD);
                } else {
                    MPI_Send(output.gray_image[j], output.width, MPI_CHAR, x, 0, MPI_COMM_WORLD);
                }
            }
        }
    } else {
        for (int j = 0; j < output.height; ++j) {
            if (input.type == COLOR) {
                MPI_Recv(output.color_image[j], output.width * 3, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else {
                MPI_Recv(output.gray_image[j], output.width, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
    }
}

int main (int argc, char **argv) {
    int rank, processes;
    MPI_Init(&argc,&argv);
    // id-ul taskului curent
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // nr de taskuri
    MPI_Comm_size(MPI_COMM_WORLD, &processes);
    readInput(argv[1]); 
    
    imageProcessing(rank, processes);            
    input = output;
    
    MPI_Finalize();
    writeData(argv[2]);
    return 0;
}
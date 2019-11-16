#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

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

void readInput(const char * fileName, image *img) {
    
    FILE *input = fopen(fileName, "rb");
    if (input == NULL)
        return;

    image *buff; // reference used for creating the original image

    buff = (image*) malloc (sizeof(image));
    char buffRead[2];
    fscanf(input, "%c %c", &buffRead[0], &buffRead[1]);
    buff->type = buffRead[1] - '0'; // from char to int
    fscanf(input, "%d %d\n%d\n", &buff->width, &buff->height, &buff->max_size);

    if (buff->type == COLOR) {
        buff->color_image = (rgb**) malloc (buff->height * sizeof(rgb*));
        for (int i = 0; i < buff->height; ++i)
            buff->color_image[i] = (rgb *) malloc (buff->width * sizeof(rgb));
        for (int i = 0; i < buff->height; ++i)
            fread(buff->color_image[i], sizeof(rgb), buff->width, input);

    } else if (buff->type == GRAYSCALE) {
        buff->gray_image = (gray**) malloc (buff->height * sizeof(gray*));
        for (int i = 0; i < buff->height; ++i)
            buff->gray_image[i] = (gray *) malloc (buff->width * sizeof(gray));
        for (int i = 0; i < buff->height; ++i)
            fread(buff->gray_image[i], sizeof(gray), buff->width, input);
    }

    *img = *buff;
    free(buff);

    fclose(input);
}

void writeData(const char * fileName, image *img) {

    FILE *output = fopen(fileName, "wb");
    if (output == NULL)
        return;
    fprintf(output, "P%d\n%d %d\n%d\n", img->type, img->width, img->height, img->max_size);
    if (img->type == COLOR) {
        for (int i = 0; i < img->height; ++i) { 
            
            for (int j = 0; j < img->width; ++j) {
                fwrite(&img->color_image[i][j].red, sizeof(unsigned char), 1, output);
                fwrite(&img->color_image[i][j].green, sizeof(unsigned char), 1, output);
                fwrite(&img->color_image[i][j].blue, sizeof(unsigned char), 1, output);
            }
        }
    } else if (img->type == GRAYSCALE) {
        for (int i = 0; i < img->height; ++i)
            for (int j = 0; j < img->width; ++j)
                fwrite(&img->gray_image[i][j], sizeof(unsigned char), 1, output);
    }

    fclose(output);

    if (img->type == COLOR) {
        for (int i = 0; i < img->height; ++i)
            free(img->color_image[i]);
        free(img->color_image);
    } else if (img->type == GRAYSCALE) {
        for (int i = 0; i < img->height; ++i)
            free(img->gray_image[i]);
        free(img->gray_image);
    }
}

void applyFilter (image *in, image *buff, int rank, int proc, float filter[3][3]) {
    image *output = (image *) malloc (sizeof(image));
    output->type = in->type;
    output->height = in->height;
    output->width = in->width;
    output->max_size = in->max_size;

    if (output->type == COLOR) {
        output->color_image = (rgb **) malloc (output->height * sizeof(rgb *));
        for (int i = 0; i < output->height; ++i)
            output->color_image[i] = (rgb *) malloc (output->width * sizeof(rgb));
    } else if (output->type == GRAYSCALE) {
        output->gray_image = (gray **) malloc (output->height * sizeof(gray *));
        for (int i = 0; i < output->height; ++i)
            output->gray_image[i] = (gray *) malloc (output->width * sizeof(gray));
    }

    for (int i = rank; i < output->height; i += proc) {
        for (int j = 0; j < output->width; ++j) {
            if (in->type == COLOR) {                
                if (i == 0 || i == output->height - 1 || j == 0 || j == output->width - 1) {
                    output->color_image[i][j].red = in->color_image[i][j].red;
                    output->color_image[i][j].green = in->color_image[i][j].green;
                    output->color_image[i][j].blue = in->color_image[i][j].blue;
                } else {
                    float red = 0, green = 0, blue = 0;
                    for (int x = i - 1; x <= i + 1; ++x) {
                        for (int y = j - 1; y <= j + 1; ++y) {
                            red += in->color_image[x][y].red * filter[x - (i - 1)][y - (j - 1)];
                            green += in->color_image[x][y].green * filter[x - (i - 1)][y - (j - 1)];
                            blue += in->color_image[x][y].blue * filter[x - (i - 1)][y - (j - 1)];
                        }
                    }
                    output->color_image[i][j].red = (u_char) red;
                    output->color_image[i][j].green = (u_char) green;
                    output->color_image[i][j].blue = (u_char) blue;
            
                }

            } else if (in->type == GRAYSCALE) {
                if (i == 0 || i == output->height - 1 || j == 0 || j == output->width - 1) {
                    output->gray_image[i][j].gray = in->gray_image[i][j].gray;
                } else {
                    float gray = 0;
                    for (int x = i - 1; x <= i + 1; ++x) {
                        for (int y = j - 1; y <= j + 1; ++y) {
                            gray += in->gray_image[x][y].gray * filter[x - (i - 1)][y - (j - 1)];
                        }
                    }
                    output->gray_image[i][j].gray = (u_char) gray;
                }
            }
        }
    }
    *buff = *output;
    free(output);
}

void imageProcessing (image* input, image* output, int rank, int processes, float filter[3][3]) {
    applyFilter(input, output, rank, processes, filter);  
    if (rank != 0) {
        for (int j = rank; j < output->height; j += processes) {
            if (input->type == COLOR) {
                MPI_Send(output->color_image[j], output->width * 3, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            } else {
                MPI_Send(output->gray_image[j], output->width, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }
        }
    } else {
        for (int x = 1; x < processes; ++x) {
            for (int j = x; j < output->height; j += processes) {
                if (input->type == COLOR) {
                    MPI_Recv(output->color_image[j], output->width * 3, MPI_CHAR, x, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                } else {
                    MPI_Recv(output->gray_image[j], output->width, MPI_CHAR, x, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }
        }
    }

    if (rank == 0) {
        for (int x = 1; x < processes; ++x) {
            for (int j = 0; j < output->height; ++j) {
                if (input->type == COLOR) {
                    MPI_Send(output->color_image[j], output->width * 3, MPI_CHAR, x, 0, MPI_COMM_WORLD);
                } else {
                    MPI_Send(output->gray_image[j], output->width, MPI_CHAR, x, 0, MPI_COMM_WORLD);
                }
            }
        }
    } else {
        for (int j = 0; j < output->height; ++j) {
            if (input->type == COLOR) {
                MPI_Recv(output->color_image[j], output->width * 3, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else {
                MPI_Recv(output->gray_image[j], output->width, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
    }
}

int main (int argc, char **argv) {
    int rank, processes;
    image input, output;
    MPI_Init(&argc,&argv);
    // id-ul taskului curent
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // nr de taskuri
    MPI_Comm_size(MPI_COMM_WORLD, &processes);
    readInput(argv[1], &input); 
    
    imageProcessing(&input, &output, rank, processes, blurMatrix);            
    input = output;
    
    MPI_Finalize();
    writeData(argv[2], &output);
    return 0;
}
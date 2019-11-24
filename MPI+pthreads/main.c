#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#define COLOR 6
#define GRAYSCALE 5
const int noThreads = 8;

float blurMatrix[3][3] = {{1.f / 16, 2.f / 16, 1.f / 16}, 
                        {2.f / 16, 4.f / 16, 2.f / 16}, 
                        {1.f / 16, 2.f / 16, 1.f / 16}};

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

image in;
image out;

void readInput(const char * fileName) {
    
    FILE *input = fopen(fileName, "rb");
    if (input == NULL)
        return;

    char buffRead[2];
    fscanf(input, "%c %c", &buffRead[0], &buffRead[1]);
    in.type = buffRead[1] - '0'; // from char to int
    fscanf(input, "%d %d\n%d\n", &in.width, &in.height, &in.max_size);

    if (in.type == COLOR) {
        in.color_image = (rgb**) malloc (in.height * sizeof(rgb*));
        for (int i = 0; i < in.height; ++i)
            in.color_image[i] = (rgb *) malloc (in.width * sizeof(rgb));
        for (int i = 0; i < in.height; ++i)
            fread(in.color_image[i], sizeof(rgb), in.width, input);

    } else if (in.type == GRAYSCALE) {
        in.gray_image = (gray**) malloc (in.height * sizeof(gray*));
        for (int i = 0; i < in.height; ++i)
            in.gray_image[i] = (gray *) malloc (in.width * sizeof(gray));
        for (int i = 0; i < in.height; ++i)
            fread(in.gray_image[i], sizeof(gray), in.width, input);
    }

    fclose(input);
}

void writeData(const char * fileName) {

    FILE *output = fopen(fileName, "wb");
    if (output == NULL)
        return;
    fprintf(output, "P%d\n%d %d\n%d\n", out.type, out.width, out.height, out.max_size);
    if (out.type == COLOR) {
        for (int i = 0; i < out.height; ++i) { 
            
            for (int j = 0; j < out.width; ++j) {
                fwrite(&out.color_image[i][j].red, sizeof(unsigned char), 1, output);
                fwrite(&out.color_image[i][j].green, sizeof(unsigned char), 1, output);
                fwrite(&out.color_image[i][j].blue, sizeof(unsigned char), 1, output);
            }
        }
    } else if (out.type == GRAYSCALE) {
        for (int i = 0; i < out.height; ++i)
            for (int j = 0; j < out.width; ++j)
                fwrite(&out.gray_image[i][j], sizeof(unsigned char), 1, output);
    }

    fclose(output);

    if (out.type == COLOR) {
        for (int i = 0; i < out.height; ++i)
            free(out.color_image[i]);
        free(out.color_image);
    } else if (out.type == GRAYSCALE) {
        for (int i = 0; i < out.height; ++i)
            free(out.gray_image[i]);
        free(out.gray_image);
    }
}

void createOutput() {
    out.type = in.type;
    out.height = in.height;
    out.width = in.width;
    out.max_size = in.max_size;

    if (out.type == COLOR) {
        out.color_image = (rgb **) malloc (out.height * sizeof(rgb *));
        for (int i = 0; i < out.height; ++i)
            out.color_image[i] = (rgb *) malloc (out.width * sizeof(rgb));
    } else if (out.type == GRAYSCALE) {
        out.gray_image = (gray **) malloc (out.height * sizeof(gray *));
        for (int i = 0; i < out.height; ++i)
            out.gray_image[i] = (gray *) malloc (out.width * sizeof(gray));
    }
}

int min(int a, int b) {
    return a <= b ? a : b;
}

void* threadFunction(void* var) {
    int id = *(int *) var;

    int start = id * ceil((double) out.height / (double) noThreads);
	int end = min(((id + 1) * ceil((double) out.height / (double) noThreads)), out.height);

    for (int i = start; i < end; ++i) {
        for (int j = 0; j < out.width; ++j) {
            if (in.type == COLOR) {                
                if (i == 0 || i == out.height - 1 || j == 0 || j == out.width - 1) {
                    out.color_image[i][j].red = in.color_image[i][j].red;
                    out.color_image[i][j].green = in.color_image[i][j].green;
                    out.color_image[i][j].blue = in.color_image[i][j].blue;
                } else {
                    float red = 0, green = 0, blue = 0;
                    for (int x = i - 1; x <= i + 1; ++x) {
                        for (int y = j - 1; y <= j + 1; ++y) {
                            red += in.color_image[x][y].red * blurMatrix[x - (i - 1)][y - (j - 1)];
                            green += in.color_image[x][y].green * blurMatrix[x - (i - 1)][y - (j - 1)];
                            blue += in.color_image[x][y].blue * blurMatrix[x - (i - 1)][y - (j - 1)];
                        }
                    }
                    out.color_image[i][j].red = (u_char) red;
                    out.color_image[i][j].green = (u_char) green;
                    out.color_image[i][j].blue = (u_char) blue;
            
                }

            } else if (in.type == GRAYSCALE) {
                if (i == 0 || i == out.height - 1 || j == 0 || j == out.width - 1) {
                    out.gray_image[i][j].gray = in.gray_image[i][j].gray;
                } else {
                    float gray = 0;
                    for (int x = i - 1; x <= i + 1; ++x) {
                        for (int y = j - 1; y <= j + 1; ++y) {
                            gray += in.gray_image[x][y].gray * blurMatrix[x - (i - 1)][y - (j - 1)];
                        }
                    }
                    out.gray_image[i][j].gray = (u_char) gray;
                }
            }
        }
    }

    return NULL;
}

int main (int argc, char **argv) {

    readInput(argv[1]); 
    createOutput();

    pthread_t threads[noThreads];
    int tid[noThreads];

    for (int i = 0; i < noThreads; ++i) {
        tid[i] = i;
    }

    for (int i = 0; i < noThreads; i++) {
        pthread_create(&threads[i], NULL, threadFunction, &tid[i]);
    }

    for (int i = 0; i < noThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    writeData(argv[2]);
    return 0;
}
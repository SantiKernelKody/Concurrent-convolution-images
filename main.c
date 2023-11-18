#include <stdio.h>
#include <stdlib.h>
#include "pgm.h"
int ReadPGM(char *file_name, byte **ppImg, int *pnWidth, int *pnHeight);
void WritePGM(char *file_name, byte *pImg, int nWidth, int nHeight);
int FrameConv3x3(byte *pInp, byte *pOut, int nW, int nH, int conv[9], int denom);
void sharp_matrix(int coef[], int n);
void top_sobel_matrix(int coef[], int n);
void blur_matrix(int coef[], int n);

// Global definitions and types
#define MAX_THREADS 4 // Assuming we have 4 CPUs for simplicity

typedef enum
{
    SOBEL,
    BLUR,
    SHARPEN
} FilterType;

typedef struct
{
    int width;
    int height;
    byte *data;
} Image;

typedef struct
{
    Image *img;
    Image *out_img;
    int *filter_matrix;
    int filter_denom;
    int start_row;
    int num_rows;
} ThreadArg;

int main(int nArg, char **ppArg)
{

    return 0;
}
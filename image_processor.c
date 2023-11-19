#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
typedef unsigned char byte;
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
    char *filename;
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
// Thread function for reading an image asynchronously
void *read_image_async(void *args)
{
    char *filename = (char *)args;
    Image *img = malloc(sizeof(Image));
    if (!img)
    {
        perror("Failed to allocate memory for image structure");
        return NULL;
    }

    byte *buffer;
    if (ReadPGM(filename, &buffer, &img->width, &img->height) != 0)
    {
        fprintf(stderr, "Failed to read image from %s\n", filename);
        free(img);
        return NULL;
    }

    img->data = buffer;
    return img;
}
// Thread function to process a segment of the image
void *process_segment(void *args)
{
    ThreadArg *thread_arg = (ThreadArg *)args;
    int start_row = thread_arg->start_row;
    int num_rows = thread_arg->num_rows;
    int nW = thread_arg->img->width;
    int conv_index;
    byte *pInpSegment = thread_arg->img->data + start_row * nW;
    byte *pOutSegment = thread_arg->out_img->data + start_row * nW;

    FrameConv3x3(pInpSegment, pOutSegment, nW, num_rows + 2, thread_arg->filter_matrix, thread_arg->filter_denom);

    return NULL;
}
// Thread function for writing an image asynchronously
void *write_image_async(void *args)
{
    Image *img = (Image *)args;
    char *filename = malloc(strlen(img->filename) + 1);
    strcpy(filename, img->filename);
    WritePGM(filename, img->data, img->width, img->height);
    free(filename);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <filter_type> <input_image.pgm> <output_image.pgm>\n", argv[0]);
        fprintf(stderr, "Filter types: sobel, blur, sharpen\n");
        return EXIT_FAILURE;
    }

    // Parse filter type from command line
    int filter_matrix[9];
    int filter_denom = 1; // Default denominator for filter normalization
    FilterType filter_type;
    if (strcmp(argv[1], "sobel") == 0)
    {
        top_sobel_matrix(filter_matrix, sizeof(filter_matrix) / sizeof(int));
        filter_type = SOBEL;
    }
    else if (strcmp(argv[1], "blur") == 0)
    {
        blur_matrix(filter_matrix, sizeof(filter_matrix) / sizeof(int));
        filter_type = BLUR;
    }
    else if (strcmp(argv[1], "sharpen") == 0)
    {
        sharp_matrix(filter_matrix, sizeof(filter_matrix) / sizeof(int));
        filter_type = SHARPEN;
    }
    else
    {
        fprintf(stderr, "Invalid filter type. Use 'sobel', 'blur', or 'sharpen'.\n");
        return EXIT_FAILURE;
    }

    // Read the image asynchronously
    pthread_t read_thread;
    Image *input_image = malloc(sizeof(Image));
    input_image->filename = strdup(argv[2]);
    pthread_create(&read_thread, NULL, read_image_async, argv[2]);
    pthread_join(read_thread, (void **)&input_image);

    // Prepare output image
    Image *output_image = malloc(sizeof(Image));
    output_image->width = input_image->width;
    output_image->height = input_image->height;
    output_image->data = malloc(output_image->width * output_image->height);
    output_image->filename = strdup(argv[3]);
    if (!output_image->data)
    {
        perror("Failed to allocate memory for output image data");
        free(input_image->data);
        free(input_image);
        free(output_image);
        free(output_image->data);
        return EXIT_FAILURE;
    }

    // Process the image using threads
    pthread_t processing_threads[MAX_THREADS];
    ThreadArg thread_args[MAX_THREADS];
    int rows_per_thread = input_image->height / MAX_THREADS;
    for (int i = 0; i < MAX_THREADS; ++i)
    {
        thread_args[i].img = input_image;
        thread_args[i].out_img = output_image;
        thread_args[i].filter_matrix = filter_matrix;
        thread_args[i].filter_denom = filter_denom;
        thread_args[i].start_row = i * rows_per_thread;
        thread_args[i].num_rows = (i == MAX_THREADS - 1) ? rows_per_thread + input_image->height % MAX_THREADS : rows_per_thread;
        // process_segment(&thread_args[i]);
        pthread_create(&processing_threads[i], NULL, process_segment, &thread_args[i]);
    }

    // Wait for all processing to complete
    for (int i = 0; i < MAX_THREADS; ++i)
    {
        pthread_join(processing_threads[i], NULL);
    }

    // Write the image asynchronously
    pthread_t write_thread;
    output_image->filename = argv[3];
    pthread_create(&write_thread, NULL, write_image_async, output_image);
    pthread_join(write_thread, NULL);

    // Cleanup
    free(input_image->data);
    free(input_image);
    // free(input_image->filename);
    free(output_image->data);
    // free(output_image->filename);
    free(output_image);

    return 0;
}
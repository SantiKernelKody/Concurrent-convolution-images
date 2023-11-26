#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
// Byte definition
typedef unsigned char byte;
// TDA structure
typedef struct ImageTask
{
    char *filename;
    char *filter;
    struct ImageTask *next;
} ImageTask;

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
// Global variables
pthread_mutex_t mutex;
ImageTask *head = NULL;
// Fucntion definitions
int ReadPGM(char *file_name, byte **ppImg, int *pnWidth, int *pnHeight);
void WritePGM(char *file_name, byte *pImg, int nWidth, int nHeight);
int FrameConv3x3(byte *pInp, byte *pOut, int nW, int nH, int conv[9], int denom);
void sharp_matrix(int coef[], int n);
void top_sobel_matrix(int coef[], int n);
void blur_matrix(int coef[], int n);
// Function to enqueue the files required by the user
void enqueue_image(char *filename, char *filter)
{
    ImageTask *newTask = malloc(sizeof(ImageTask));
    newTask->filename = strdup(filename);
    newTask->filter = strdup(filter);
    newTask->next = NULL;

    if (head == NULL)
    {
        head = newTask;
    }
    else
    {
        ImageTask *temp = head;
        while (temp->next != NULL)
        {
            temp = temp->next;
        }
        temp->next = newTask;
    }
}
// Function to enqueue the files required to keep processing the rest
ImageTask *dequeue_image()
{
    if (head == NULL)
    {
        return NULL;
    }
    ImageTask *task = head;
    head = head->next;
    return task;
}

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
        fprintf(stderr, "Failed to read image from %s\n. Please verify the filename.", filename);
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
// Function to collect all the files that the user wants to aplly filters
int user_interface_images_requested()
{
    char inputFilename[256];
    char inputFilter[256];

    while (1)
    {
        printf("Enter image filename ('quit' to exit): ");
        scanf("%s", inputFilename);
        if (strcmp(inputFilename, "quit") == 0)
            break;

        printf("Enter filter type (sobel, blur, sharpen): ");
        scanf("%s", inputFilter);
        if (strcmp(inputFilter, "sobel") != 0 && strcmp(inputFilter, "blur") != 0 && strcmp(inputFilter, "sharpen") != 0)
        {
            printf("Invalid filter type. Use 'sobel', 'blur', or 'sharpen'.\n");
            continue;
        }

        enqueue_image(inputFilename, inputFilter);
    }
    if (head == NULL)
    {
        return 1;
    }
    return 0;
}
// Getting file just name no extension
char *get_filename_no_extension(const char *filename)
{
    if (filename == NULL)
    {
        return NULL;
    }

    char *dot_position = strrchr(filename, '.');
    if (dot_position == NULL)
    {
        printf("There is no extension on the file provided please try again.");
        return NULL;
    }

    int name_length = dot_position - filename;
    char *name_without_extension = malloc(name_length + 1); // +1 for NULL char

    if (name_without_extension != NULL)
    {
        strncpy(name_without_extension, filename, name_length);
        name_without_extension[name_length] = '\0'; // Make sure it ends as NULL
    }
    else
    {
        fprintf(stderr, "Failed to allocate memory for name_without_extension\n");
    }

    return name_without_extension;
}

int main(int argc, char *argv[])
{
    printf("========  Image Processor Program  ========\n\n");
    int status = user_interface_images_requested();
    if (status == 1)
        return 0;

    long MAX_THREADS = sysconf(_SC_NPROCESSORS_ONLN) - 1;
    ImageTask *temp = dequeue_image();
    while (temp != NULL)
    {
        // Parse filter type from command line
        int filter_matrix[9];
        int filter_denom = 1; // Default denominator for filter normalization

        if (strcmp(temp->filter, "sobel") == 0)
        {
            top_sobel_matrix(filter_matrix, sizeof(filter_matrix) / sizeof(int));
        }
        else if (strcmp(temp->filter, "blur") == 0)
        {
            blur_matrix(filter_matrix, sizeof(filter_matrix) / sizeof(int));
        }
        else if (strcmp(temp->filter, "sharpen") == 0)
        {
            sharp_matrix(filter_matrix, sizeof(filter_matrix) / sizeof(int));
        }

        // Mutex init
        // pthread_mutex_init(&mutex, NULL);

        // Read the image asynchronously
        pthread_t read_thread;
        Image *input_image;
        pthread_create(&read_thread, NULL, read_image_async, temp->filename);
        pthread_join(read_thread, (void **)&input_image);
        if (input_image == NULL)
        {
            temp = dequeue_image();
            continue;
        }

        input_image->filename = strdup(temp->filename);

        // Prepare output image
        Image *output_image = malloc(sizeof(Image));
        output_image->width = input_image->width;
        output_image->height = input_image->height;
        output_image->data = malloc(output_image->width * output_image->height);
        char *file_just_name = get_filename_no_extension(temp->filename);
        if (file_just_name == NULL)
        {
            fprintf(stderr, "Failed to process the filename\n");
            return 1;
        }

        int out_filename_size = strlen(file_just_name) + strlen(temp->filter) + strlen("_result.pgm") + 2; // +2 for the underscores and null terminator
        char *output_file_name = malloc(out_filename_size);
        if (output_file_name != NULL)
        {
            // Concat
            sprintf(output_file_name, "%s_%s_result.pgm", file_just_name, temp->filter);
        }
        else
        {
            free(output_file_name);
            free(output_image->data);
            free(output_image);
            printf("Failed to allocate memory for output_file_name\n");
            fprintf(stderr, "Failed to allocate memory for output_file_name\n");
            return 1;
        }

        output_image->filename = strdup(output_file_name);
        if (!output_image->data)
        {
            perror("Failed to allocate memory for output image data");
            free(input_image->data);
            free(input_image);
            free(output_image);
            free(output_image->data);
            return 1;
        }
        // Process the image using threads
        pthread_t processing_threads[MAX_THREADS];
        ThreadArg thread_args[MAX_THREADS];
        int rows_per_thread = input_image->height / MAX_THREADS;
        clock_t start, end;
        double cpu_time_used;
        start = clock();
        cpu_set_t cpuset;
        for (int i = 0; i < MAX_THREADS; ++i)
        {
            thread_args[i].img = input_image;
            thread_args[i].out_img = output_image;
            thread_args[i].filter_matrix = filter_matrix;
            thread_args[i].filter_denom = filter_denom;
            thread_args[i].start_row = i * rows_per_thread;
            thread_args[i].num_rows = rows_per_thread;

            pthread_create(&processing_threads[i], NULL, process_segment, &thread_args[i]);

            CPU_ZERO(&cpuset);
            CPU_SET(i % MAX_THREADS, &cpuset);

            pthread_setaffinity_np(processing_threads[i], sizeof(cpu_set_t), &cpuset);
        }
        end = clock();
        cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
        printf("Convolutional process timing: %f seconds\n", cpu_time_used);
        // Wait for all processing to complete
        for (int i = 0; i < MAX_THREADS; ++i)
        {
            pthread_join(processing_threads[i], NULL);
        }
        // Mutex destroy
        // pthread_mutex_destroy(&mutex);

        // Write the image asynchronously
        pthread_t write_thread;
        pthread_create(&write_thread, NULL, write_image_async, output_image);
        pthread_join(write_thread, NULL);

        // Free memory
        free(input_image->data);
        free(input_image);
        free(output_image->data);
        free(output_image);
        free(output_file_name);

        // NEXT IMAGE TASK
        temp = dequeue_image();
    }

    return 0;
}
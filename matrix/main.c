#include "matrix.h"
#include <time.h>
#include <pthread.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/times.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdarg.h>

#define ERR_EXIT(EXPR, STRING) \
    do { if ((EXPR)) {\
        perror((STRING));\
        exit(EXIT_FAILURE);\
    }} while(0)


typedef struct {
        matrix* mx1;
        matrix* mx2;
        matrix* res;
        size_t first;
        size_t size;
        pthread_t tr_pid;
} thread_arg;

void* mat_thread(void* arg) 
{
        int i = 0;
        matrix* res = ((thread_arg*) arg)->res;
        matrix* mx1 = ((thread_arg*) arg)->mx1;
        matrix* mx2 = ((thread_arg*) arg)->mx2;

        for (i = 0; i < ((thread_arg*) arg)->size; ++i) {
                
                /*fprintf(stderr, "!! %d %d %d\n"
                                , (((thread_arg*) arg)->tr_pid) 
                                , GET_RAW(res, (((thread_arg*) arg)->first + i))
                                , GET_COL(res, (((thread_arg*) arg)->first + i))
                        );*/
                M_NUMBER(res, (((thread_arg*) arg)->first + i)) = 
                        raw_col(mx1, mx2, 
                                GET_RAW(res, (((thread_arg*) arg)->first + i)),
                                GET_COL(res, (((thread_arg*) arg)->first + i)));
        }

        pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
 
        struct timespec start_time;
        struct timespec end_time;
        size_t sec_time;
        long nsec_time;

        int n = 0;
        int opt = 0;
        int i = 0;
        char err = '\0';
    
        int fd1 = 0;
        int fd2 = 0;
    
        int out = 0;
        size_t size = 0;
    
        thread_arg* arg_arr = NULL;
    
        matrix* mx1 = NULL;
        matrix* mx2 = NULL;
        matrix* res = NULL;
    
        char* help_msg = "Usage: matmul -c [] inp1 inp2 out"
                         ": -c number of threads\n"
                         "-h this help\n";
    
        while ((opt = getopt(argc, argv, "c:h")) != -1) {
    
                switch (opt) {
    
                        case 'c':
                                sscanf(optarg, "%d%c", &n, &err);
                                ERR_EXIT((err != '\0'), help_msg);
                        break;
    
                        case 'h':
                                fprintf(stderr, help_msg);
                        break;
    
                        default:
                                fprintf(stderr, help_msg);
                                return 1;
                }
        }
    
        fd1 = open(argv[3], O_RDONLY);
        fd2 = open(argv[4], O_RDONLY);
        out = open(argv[5], O_WRONLY);
    
        mx1 = matrix_asc_read(fd1);
        mx2 = matrix_asc_read(fd2);
    
        if (mx1->cols != mx2->raws) {
                perror("Incorrect matrix size\n");
                return 0;
        }
    
        res = matrix_create(mx1->raws, mx2->cols);
    
        size = mx1->raws * mx2->cols;
    
        arg_arr = (thread_arg*) calloc (n, sizeof(thread_arg));
    
        int cur_first = 0;

        for (i = 0; i < n; ++i) {
    
                arg_arr[i].mx1 = mx1;
                arg_arr[i].mx2 = mx2;
                arg_arr[i].res = res;
                arg_arr[i].first = cur_first;
                arg_arr[i].size = size / n;
    
                cur_first += arg_arr[i].size;

        }

        arg_arr[n - 1].size += size - (arg_arr[0].size * n);

        clock_gettime(CLOCK_REALTIME, &start_time);

        for (opt = 0; opt < 5; ++opt) {

        for (i = 0; i < n; ++i) {
                fprintf(stderr, "! %d %d \n", arg_arr[i].first, arg_arr[i].size);
                pthread_create(&(arg_arr[i].tr_pid), NULL,
                                mat_thread, arg_arr + i);
        }

        for (i = 0; i < n; ++i) {
                void* ret = NULL;
                pthread_join(arg_arr[i].tr_pid, &ret);
        }
        }

        clock_gettime(CLOCK_REALTIME, &end_time);
        sec_time = (end_time.tv_sec - start_time.tv_sec); 

        if (end_time.tv_nsec > start_time.tv_nsec) {
                nsec_time = (end_time.tv_nsec - start_time.tv_nsec) / 1000000L; 
        } else {
                nsec_time = 1000L + (end_time.tv_nsec - start_time.tv_nsec) / 1000000L;
                sec_time--;
        }

        fprintf(stderr, "time usage: %lu.%ld\n", sec_time, nsec_time);

        matrix_free(mx1);
        matrix_free(mx2);
        free(arg_arr);

        matrix_asc_write(out, res);
        matrix_free(res);

        return 0;
}

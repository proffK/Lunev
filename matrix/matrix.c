#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "matrix.h"
#include <unistd.h>

long matrix_init(matrix* mx, void* mdata)
{
        mx->raws = *((size_t*) mdata);
        mdata += sizeof(size_t);

        mx->raws = *((size_t*) mdata);
        mdata += sizeof(size_t);
        
        mx->data = mdata;

        return (mx->raws * mx->cols);
}

int matrix_mul(matrix* mx1, matrix* mx2, matrix* res)
{
        if (!(mx1 && mx2 && res)) {
                return -1;
        }

        if ((mx1->cols != mx2->raws) ||
            (res->cols != mx2->cols) ||
            (res->raws != mx1->raws)) {
                return -1;
        }

        return 0;
}

matrix* matrix_create(size_t raws, size_t cols)
{
        matrix* new_mx = (matrix*) malloc (sizeof(matrix));

        if (!new_mx) return NULL;

        new_mx->data = (m_num*) calloc (raws * cols, sizeof(m_num));

        if (!(new_mx->data)) {
                free(new_mx);
                return NULL;
        }

        new_mx->raws = raws;
        new_mx->cols = cols;

        return new_mx;
}

int matrix_free(matrix* mx)
{
        free(mx->data);
        free(mx);

        return 0;
}

matrix* matrix_bin_read(int fd)
{
        int raws = 0;
        int cols = 0;
        matrix* mx = NULL;

        read(fd, &raws, sizeof(int));
        read(fd, &cols, sizeof(int));
        
        mx = matrix_create(raws, cols);

        if (!mx) return NULL;

        read(fd, mx->data, raws * cols);

        return mx;
}

int matrix_bin_write(int fd, matrix* mx)
{
        write(fd, &mx->raws, sizeof(int));
        write(fd, &mx->cols, sizeof(int));
        write(fd, mx->data, mx->raws * mx->cols);

        return 0;
}

matrix* matrix_bin_map(int fd)
{
        return NULL;
}

matrix* matrix_asc_read(int fd)
{
        int i = 0;
        int raws = 0;
        int cols = 0;
        size_t size = 0;

        FILE* inp = NULL;
        matrix* mx = NULL;
        
        inp = fdopen(fd, "r");
        
        if (!inp) return NULL;

        fscanf(inp, "%d", &raws);
        fscanf(inp, "%d", &cols);
        
        mx = matrix_create(raws, cols);
        size = raws * cols;

        if (!mx) return NULL;

        for (i = 0; i < size; ++i) {
                fscanf(inp, "%lg", &(mx->data)[i]);
        }

        return mx;
}

int matrix_asc_write(int fd, matrix* mx)
{
        int i = 0;
        size_t size = 0;

        FILE* out = NULL;
        out = fdopen(fd, "w");
        
        if (!out) return 0;

        fprintf(out, "%lu ", mx->raws);
        fprintf(out, "%lu\n", mx->cols);
        
        size = mx->raws * mx->cols;

        for (i = 0; i < size; ++i) {
                fprintf(out, "%lg ", (mx->data)[i]);
        }

        return 0;
}

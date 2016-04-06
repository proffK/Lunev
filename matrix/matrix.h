#ifndef __MATRIX__
#define __MATRIX__

#include <stdbool.h>
#include <stddef.h>

#define GET_RAW(mx, num) ((num) / ((mx)->cols))
#define GET_COL(mx, num) ((num) % ((mx)->cols))

#define MELEM(mx, iel, jel) (*((mx)->data + (mx)->cols * iel + jel))

#define M_NUMBER(mx, num) (MELEM((mx), \
                           GET_RAW((mx), (num)), GET_COL((mx), (num))))

typedef double m_num;

typedef struct {
        size_t raws;
        size_t cols;
//        bool tr_flag;
        m_num* data;
} matrix;

long matrix_init(matrix* mx, void* mdata);

int matrix_mul(matrix* mx1, matrix* mx2, matrix* res);

matrix* matrix_create(size_t raws, size_t cols);

int matrix_free(matrix* mx);

matrix* matrix_bin_read(int fd);

matrix* matrix_bin_map(int fd);

int matrix_bin_write(int fd, matrix* mx);

matrix* matrix_asc_read(int fd);

int matrix_asc_write(int fd, matrix* mx);

static inline m_num dot_prod(m_num* a, m_num* b, size_t size)
{
        m_num res = 0;
        while (size--) {
                res += *a * *b;
                ++a;
                ++b;
        }

        return res;
}

static inline m_num raw_col(matrix* mx1, matrix* mx2, int raw, int col)
{
        int i = 0;
        m_num res = 0;

        for (i = 0; i < mx1->cols; ++i) {
                res += MELEM(mx1, raw, i) * MELEM(mx2, i, col);
        }

        return res;
}

#endif

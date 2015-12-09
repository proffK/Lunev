#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <limits.h>

#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>

#define PAGE_SIZE 4096
#define PIPE_SIZE 65536

#define ERR_EXIT(EXPR, STRING) \
    if ((EXPR)) {\
        perror((STRING));\
        exit(EXIT_FAILURE);\
    }

typedef struct {

    int fd_inp;
    int fd_out;
    int rd;                 /* 1 if fd ready for read/write */
    int wr;
    ssize_t free_size;
    char* rptr;
    char* wptr;
    char* data;
    ssize_t size;

} prbuf;

typedef struct {

    int pipefd[2];

} pipefd;

prbuf* prbuf_create(int size, int fd_inp, int fd_out);

int prbuf_init(prbuf** buf_arr, int n, fd_set* rset, fd_set* wset);

int prbuf_do_ops(prbuf** buf_arr, int n);

int prbuf_delete(prbuf* buf);

int prbuf_read(prbuf* buf);

int prbuf_write(prbuf* buf);

int prbuf_dump(prbuf* buf);

int init_rset(fd_set* rset, prbuf** buf_arr, int n);

int init_wset(fd_set* wset, prbuf** buf_arr, int n);

int check_end(prbuf** buf_arr, int n);

int child(int fd_inp, int fd_out);

int main(int argc, char* argv[]) {
    
    int n = 0;
    int opt = 0;
    int i = 0;
    char err = '\0';

    char* help_msg = "Usage : -n number of created child\n"
                             "-f input file\n";

    int fd_inp = 0;
    int fd_out = STDOUT_FILENO;
    double db_buf_size = 0;
    int cur_buf_size = 0;

    prbuf** buff_arr = NULL;
    pipefd* pipefd_buf = NULL;

    fd_set rset;
    fd_set wset;

    ERR_EXIT((argc < 5), "Too few arguments\n")

    while ((opt = getopt(argc, argv, "n:f:")) != -1) {

        switch (opt) {

            case 'n':
                sscanf(optarg, "%d%c", &n, &err);
                ERR_EXIT((err != '\0'), help_msg)
                break;

            case 'f':
                fd_inp = open(optarg, O_RDONLY);
                ERR_EXIT((fd_inp == -1), "Incorrect filename")
                break;

            default:
                fprintf(stderr, help_msg);
                return 1;

        }
    }
    
    ERR_EXIT((optind > argc), "No argument after -n ")
    
    ERR_EXIT((n <= 0), "Incorrect argument")

    buff_arr = (prbuf**) calloc (n, sizeof(prbuf*));
    ERR_EXIT((!(buff_arr)), "No enough memory")

    pipefd_buf = (pipefd*) malloc ((2 * n - 1) * sizeof(pipefd));
    ERR_EXIT((!(pipefd_buf)), "No enough memory for pipe")

    for (i = 0; i < (2 * n - 1); ++i) {
        int flags = 0;
       
        ERR_EXIT((pipe((pipefd_buf + i)->pipefd) == -1)
                  , "Can't create pipe")     

        flags = fcntl((pipefd_buf + i)->pipefd[0], F_GETFD, 0);
        flags |= O_NONBLOCK;
        ERR_EXIT(fcntl((pipefd_buf + i)->pipefd[0], F_SETFL, flags), "Error fcntl")
            
        flags = fcntl((pipefd_buf + i)->pipefd[1], F_GETFD, 0);
        flags |= O_NONBLOCK;
        ERR_EXIT(fcntl((pipefd_buf + i)->pipefd[1], F_SETFL, flags), "Error fcntl")
 
    }

    db_buf_size = pow(3, n - 1) * PAGE_SIZE;
    ERR_EXIT((db_buf_size > (double) INT_MAX), "Too big number")

    cur_buf_size = (int) db_buf_size;

    for (i = 0; i < n; ++i) {
        pid_t cld_pid;
        int cld_fd_inp = 0;
        int cld_fd_out = 0;
        int par_fd_inp = 0;
        int par_fd_out = 0;

        if (i == 0) {

            cld_fd_inp = fd_inp;
            cld_fd_out = (pipefd_buf + 2 * i)->pipefd[1];
            par_fd_inp = (pipefd_buf + 2 * i)->pipefd[0];
            par_fd_out = (pipefd_buf + 2 * i + 1)->pipefd[1];

        } else if (i == n - 1) {

            par_fd_out = fd_out;
            par_fd_inp = (pipefd_buf + 2 * i)->pipefd[0];
            cld_fd_inp = (pipefd_buf + 2 * i - 1)->pipefd[0];
            cld_fd_out = (pipefd_buf + 2 * i)->pipefd[1];

        } else {

            cld_fd_inp = (pipefd_buf + 2 * i - 1)->pipefd[0];
            cld_fd_out = (pipefd_buf + 2 * i)->pipefd[1];
            par_fd_inp = (pipefd_buf + 2 * i)->pipefd[0];
            par_fd_out = (pipefd_buf + 2 * i + 1)->pipefd[1];

        }

        buff_arr[i] = prbuf_create(cur_buf_size, par_fd_inp, par_fd_out);
        ERR_EXIT((buff_arr == NULL), "No memory for parent buffer")
        prbuf_dump(buff_arr[i]);

        cld_pid = fork();

        if (cld_pid == 0) {
            int j = 0;

            for (j = 0; j < i; ++j) {
                close(buff_arr[j]->fd_inp);
                close(buff_arr[j]->fd_out);
            }

            child(cld_fd_inp, cld_fd_out);
            return 0;

        }

        cur_buf_size /= 3;
    }

    while(1) {

        init_rset(&rset, buff_arr, n);
        init_wset(&wset, buff_arr, n);

        for (i = 0; i < n; ++i) {

            prbuf_dump(buff_arr[i]);

        }

        ERR_EXIT((select(INT_MAX, &rset, &wset, NULL, NULL) == -1),
                  "Select error");

        prbuf_init(buff_arr, n, &rset, &wset);
        for (i = 0; i < n; ++i) {

            prbuf_dump(buff_arr[i]);

        }

        prbuf_do_ops(buff_arr, n);

        if (check_end(buff_arr, n) == 1) break;

    }

    for (i = 0; i < n; ++i) {

        prbuf_delete(buff_arr[i]);

    }

    free(buff_arr);
    free(pipefd_buf);
    close(fd_inp);
    
    return 0;
}

prbuf* prbuf_create(int size, int fd_inp, int fd_out) {

    prbuf* new_pb = (prbuf*) calloc (1, sizeof(prbuf));
    
    if (new_pb == NULL) {
        return NULL;
    }

    new_pb->data = (char*) calloc (size, sizeof(char));

    if (new_pb->data == NULL) {
        return NULL;
    }

    new_pb->size = size;
    new_pb->rd = 0;
    new_pb->wr = 0;
    new_pb->free_size = size;
    new_pb->rptr = new_pb->data;
    new_pb->wptr = new_pb->data;
    new_pb->fd_inp = fd_inp;
    new_pb->fd_out = fd_out;

    return new_pb;

}

int prbuf_init(prbuf** buf_arr, int n, fd_set* rset, fd_set* wset) {

    int i = 0;

    for (i = 0; i < n; ++i) {

        if (FD_ISSET(buf_arr[i]->fd_inp, rset)) {

            buf_arr[i]->rd = 1;
            fprintf(stderr, "r %d num:%d\n", buf_arr[i]->fd_inp, i);

        } else if (FD_ISSET(buf_arr[i]->fd_out, wset)) {

            buf_arr[i]->wr = 1;
            fprintf(stderr, "w %d num:%d\n", buf_arr[i]->fd_out, i);

        }

    }

}

int prbuf_do_ops(prbuf** buf_arr, int n) {

    int i = 0;

    for (i = 0; i < n; ++i) {
        do { 
        
        if (buf_arr[i]->rd == 1) {

            prbuf_read(buf_arr[i]);

        }

        if (buf_arr[i]->wr == 1) {

            prbuf_write(buf_arr[i]);

        }
        }
        while ( buf_arr[i]->wr == 1 && buf_arr[i]->rd == 1);

    }

    return 0;
}

int prbuf_delete(prbuf* buf) {

    free(buf->data);
    buf->data = NULL;
    free(buf);

    return 0;

}

int prbuf_read(prbuf* buf) {
    static int size;

    if (buf->rptr == buf->wptr && buf->free_size == 0) {

        return 0;

    }
    
    size = PAGE_SIZE;

    while ((buf->rptr != buf->wptr || buf->free_size != 0) && buf->rd != -1) {

        size = read(buf->fd_inp, buf->rptr, size);
        fprintf(stderr, "size %d\n", buf->free_size);

        if (size == -1 && errno == EAGAIN) {

            buf->rd = 0;
            return 1;

        }

        if (size < PAGE_SIZE) {
            
            close(buf->fd_inp);
            buf->rd = -1;

        }

        if (buf->rptr == buf->data + buf->size) buf->rptr = buf->data;

        buf->rptr += size;
        buf->free_size -= size;

        if (buf->free_size < PAGE_SIZE) size = 0; //buf->free_size;

        if (buf->rptr == buf->data + buf->size) buf->rptr = buf->data;
    }

    return 0;
}

int prbuf_write(prbuf* buf) {

    if (buf->rptr == buf->wptr && buf->free_size == buf->size) {

        return 0;

    }

    while (buf->rptr != buf->wptr || buf->free_size == 0) {
        int size = 0;

        if ((buf->size - buf->free_size) < PAGE_SIZE)
            size = (buf->size - buf->free_size);
        else size = PAGE_SIZE;

        size = write(buf->fd_out, buf->wptr, size);

        if (size == -1 && errno == EAGAIN) {

            buf->wr = 0;
            return 1;

        }

        //if (ret_code == 0) {
        //    
        //    close(buf->fd_inp);
        //    buf->rd = 1;
        //    return 0;

        //}

        if (buf->wptr == buf->data + buf->size) buf->wptr = buf->data;

        buf->wptr += size;
        buf->free_size += size;

        if (buf->wptr == buf->data + buf->size) buf->wptr = buf->data;
    }

    return 0;
}

int init_rset(fd_set* rset, prbuf** buf_arr, int n) {

    int i = 0;
    FD_ZERO(rset);

    for (i = 0; i < n; ++i) {

        if (buf_arr[i]->rd == 0) {

            FD_SET(buf_arr[i]->fd_inp, rset);

        }
    }

    return 0;
}


int init_wset(fd_set* wset, prbuf** buf_arr, int n) {

    int i = 0;
    FD_ZERO(wset);

    for (i = 0; i < n; ++i) {

        if (buf_arr[i]->wr == 0) {

            FD_SET(buf_arr[i]->fd_out, wset);

        }
    }

    return 0;
}

int prbuf_dump(prbuf* buf) {

    fprintf(stderr, "\ninput fd:%d\n"
                    "output fd:%d\n"
                    "read state:%d\n"
                    "write state:%d\n"
                    "free size:%d\n"
                    "data ptr:%p\n"
                    "write ptr:%p\n"
                    "read ptr:%p\n"
                    "size:%d\n", 
                    buf->fd_inp,
                    buf->fd_out,
                    buf->rd,
                    buf->wr,
                    buf->free_size,
                    buf->data,
                    buf->wptr,
                    buf->rptr,
                    buf->size);

    return 0;

}
int check_end(prbuf** buf_arr, int n) { 
    int i = 0, counter = 0;

    for (i = 0; i < n; ++i) {

        if (buf_arr[i]->rd == -1) ++counter;

    }

    if (counter == n) return 1;

    return 0;

}

int child(int fd_inp, int fd_out) {
    int flags = 0;
    char buf[PIPE_SIZE] = {};
    int cur_size = 0;

    flags = fcntl(fd_inp, F_GETFD, 0);
    flags &= ~O_NONBLOCK;
    ERR_EXIT(fcntl(fd_inp, F_SETFL, flags), "Error fcntl")
        
    flags = fcntl(fd_out, F_GETFD, 0);
    flags &= ~O_NONBLOCK;
    ERR_EXIT(fcntl(fd_out, F_SETFL, flags), "Error fcntl")

    fprintf(stderr, "child fd_inp:%d\nchild fd_out:%d\n", fd_inp, fd_out);

    while ((cur_size = read(fd_inp, buf, PAGE_SIZE)) != 0) {
        //perror("Ddd");
        //fprintf(stderr, "!!read_succes %d %d", cur_size, fd_inp);
        write(fd_out, buf, cur_size);
    //sleep(7);
    }

    close(fd_inp);
    close(fd_out);

    return 0;

}

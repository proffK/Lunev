#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PIPE_SIZE 65536

const char* main_pipe_name = "/tmp/main_pipe";

const char* wr_lock_pipe_name = "/tmp/write_lock";

const char* rd_lock_pipe_name = "/tmp/read_lock";

int receive_mode(int fd_out);

int send_mode(int fd_inp);

int main(int argc, char* argv[]) {

    if (argc == 1) return receive_mode(STDOUT_FILENO);

    int fd_inp = open(argv[1], O_RDONLY);

    if (fd_inp == -1) {

        perror("File doesn't exist\n");
        return 1;

    }

    return send_mode(fd_inp);

}

int send_mode(int fd_inp)
{
    int wr_lock_fifo = 0;
    int rd_lock_fifo = 0;
    int main_pipe = 0;
    uint8_t buf[PIPE_SIZE];
    int wr_size;

    mkfifo(rd_lock_pipe_name, 0777);
    mkfifo(wr_lock_pipe_name, 0777);
    mkfifo(main_pipe_name, 0777);

    rd_lock_fifo = open(rd_lock_pipe_name, O_WRONLY);
    wr_lock_fifo = open(wr_lock_pipe_name, O_WRONLY);

    write(wr_lock_fifo, buf, PIPE_SIZE); //entry critical region
    
    write(rd_lock_fifo, buf, PIPE_SIZE);

    main_pipe = open(main_pipe_name, O_WRONLY);

    while ((wr_size = read(fd_inp, buf, PIPE_SIZE)) != 0) {

  //      sleep(5);

        write(main_pipe, buf, wr_size);

    } //leave critical region

    close(main_pipe);
    close(wr_lock_fifo);
    close(fd_inp);

    return 0;

}

int receive_mode(fd_out)
{
    int wr_lock_fifo = 0;
    int rd_lock_fifo = 0;
    int main_pipe = 0;
    uint8_t buf[PIPE_SIZE];
    int wr_size;

    mkfifo(rd_lock_pipe_name, 0777);
    mkfifo(wr_lock_pipe_name, 0777);
    mkfifo(main_pipe_name, 0777);

    rd_lock_fifo = open(rd_lock_pipe_name, O_RDONLY);
    wr_lock_fifo = open(wr_lock_pipe_name, O_RDONLY);

    read(rd_lock_fifo, buf, PIPE_SIZE); //entry critical region
    
    main_pipe = open(main_pipe_name, O_RDONLY);

    while ((wr_size = read(main_pipe, buf, PIPE_SIZE)) != 0) {

        write(fd_out, buf, wr_size);

    }

    read(wr_lock_fifo, buf, PIPE_SIZE); //leave critical region

    close(wr_lock_fifo);
    close(rd_lock_fifo);
    close(fd_out);

    return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define PIPE_SIZE  4 * 1024

int main(int argc, char* argv[]) {
    
    int pipe_fd[2] = {};
    int inp_file = 0;
    int err_code = 0;
    pid_t temp = 0;
    char buf[PIPE_SIZE] = {};
    
    err_code = pipe(pipe_fd);

    if (err_code) {

        printf("I can't open pipe\n");
        return 1;

    }
    

    if ((temp = fork()) == 0) {
        ssize_t inp_size = 0;

        inp_file = open(argv[1], O_RDONLY | O_NONBLOCK);
        while ((inp_size = read(inp_file, buf, PIPE_SIZE)) == PIPE_SIZE) {
            
            fprintf(stderr, "%d\n", inp_size);
            write(pipe_fd[1], buf, PIPE_SIZE);
            
        }

        if (inp_size != 0 && inp_size != PIPE_SIZE) {

            write(pipe_fd[1], buf, inp_size);

        } 

        close(pipe_fd[1]);
        close(inp_file);
        fprintf(stderr, "\nERROR %d\n", inp_size);
        return 0;

    } else if (temp > 0) {
        ssize_t out_size = 0;
        close(pipe_fd[1]);

        while ((out_size = read(pipe_fd[0], buf, PIPE_SIZE)) == PIPE_SIZE) {

            write(1, buf, PIPE_SIZE);
            fprintf(stderr, "%d %d\n", out_size, pipe_fd[0]);

        }

        if (out_size != 0 && out_size != PIPE_SIZE) {
            
            write(1, buf, out_size);

        }

        close(pipe_fd[0]);
        return 0;

    }

    perror("Fork error\n");

    return 2;

}

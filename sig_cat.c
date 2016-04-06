#include "bits.h"

#include <unistd.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <signal.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

static void bit_hndl(int signo);

#define barr_size 256 * 8

int inp_bit = -1;

int main(int argc, char* argv[])
{
    pid_t ppid = 0;
    pid_t cpid = 0;
    int fd = 0;
    sigset_t mask;

    if (argc != 2) {

        perror("Incorrect arguments\n");
        exit(EXIT_FAILURE);

    }

    if ((fd = open(argv[1], O_RDONLY)) == -1) {

        perror("Incorrect filename\n");
        exit(EXIT_FAILURE);

    }

    sigfillset(&mask);

    if (sigprocmask(SIG_SETMASK, &mask, NULL)) {

        perror("Sigprocmask error\n");
        exit(EXIT_SUCCESS);

    }

    ppid = getpid();
    cpid = fork();

    if (cpid == -1) {

        perror("fork error\n");
        exit(EXIT_FAILURE);

    } else if (cpid == 0) { /* Child */

        struct sigaction sigact = {
    
            .sa_handler = bit_hndl,
            .sa_mask = mask
        };

        STATIC_BARR(barr, barr_size); 
        int cur_size = barr_size;
        int i = 0;

        prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0);

        if (ppid != getppid()) {

            perror("Parent terminated\n");
            exit(EXIT_FAILURE);

        }

        sigaction(SIGUSR1, &sigact, NULL);
        sigaction(SIGUSR2, &sigact, NULL);
        sigaction(SIGCHLD, &sigact, NULL);

        while (cur_size == barr_size) {
            
            cur_size = READ_BARR(fd, barr, cur_size);

            for (i = 0; i < cur_size; ++i) {

                int cur_b = 0;

                cur_b = GET_B(barr, i);

                if (cur_b == 1) {

                    kill(getppid(), SIGUSR1); // Enter

                } else if (cur_b == 0) {

                    kill(getppid(), SIGUSR2);

                } else {

                    perror("GET_B err\n");
                    exit(EXIT_FAILURE);

                }

                sigdelset(&mask, SIGUSR2);
                sigsuspend(&mask); // Exit

            }
        }

        return 0;

    } else { /* Parent */

        struct sigaction sigact = {
    
            .sa_handler = bit_hndl,
            .sa_mask = mask
        };

        STATIC_BARR(barr, barr_size);
        int i = 0;

        sigaction(SIGUSR1, &sigact, NULL);
        sigaction(SIGUSR2, &sigact, NULL);
        sigaction(SIGCHLD, &sigact, NULL);

        sigdelset(&mask, SIGUSR1);
        sigdelset(&mask, SIGUSR2);
        sigdelset(&mask, SIGCHLD);

        while (1) {

            for (i = 0; i < barr_size; ++i) {

                sigsuspend(&mask); // Exit

                if (inp_bit == 1) {

                    SET_B(barr, i);
                    inp_bit = -1;

                } else if (inp_bit == 0) {

                    CLR_B(barr, i);
                    inp_bit = -1;

                } else if (inp_bit == -1) {

                    WRITE_BARR(STDOUT_FILENO, barr, i);
                    close(fd);
                    return 0;

                }

                kill(cpid, SIGUSR2); // Enter

            }

            WRITE_BARR(STDOUT_FILENO, barr, i);

        }

        exit(EXIT_FAILURE);

    }

    exit(EXIT_FAILURE);

}

static void bit_hndl(int signo) {

    if (signo == SIGUSR1) {

        inp_bit = 1;

    } else if (signo == SIGUSR2) {
        
        inp_bit = 0;

    } else if (signo == SIGCHLD) {
        
        inp_bit = -1;

    }

    return;

}

        

        





                



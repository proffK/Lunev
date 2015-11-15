#include <stdio.h>
#include <getopt.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    int n = 0, opt = 0, i = 0;
    pid_t ppid = 0;
    char err = '\0';
    int err_w = 0;
    char help_msg[] = "Usage : -n create n process and print all information about\n";
    while ((opt = getopt(argc, argv, "n:")) != -1) {
        switch (opt) {
            case 'n':
                sscanf(optarg, "%d%c", &n, &err);
                if (err != '\0') {
                    fprintf(stderr, help_msg);
                    return 0;
                }
                break;
            default:
                fprintf(stderr, help_msg);
                return 1;
        }
    }
    if (optind > argc) {
        fprintf(stderr, "No argument after -n %d");
        return 2;
    }
    if (n <= 0) {
        fprintf(stderr, "Incorrect argument");
        return 3;
    }
    
    ppid = getpid();
    
    for (i = 0; i < n; ++i) {
    pid_t pid = 0;
    
        if ((pid = fork()) == 0) {
            pid = getpid();

            fprintf(stderr, "%d %d %d\n", i + 1, pid, ppid);
            return 0;

        }
        wait(&err_w);

    }
        
    return 0;
}



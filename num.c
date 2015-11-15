#include <stdio.h>
#include <getopt.h>

int main(int argc, char* argv[]) {
    int n = 0, opt = 0, i = 0;
    char err = '\0';
    char help_msg[] = "Usage : -n print all number before n \n";
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
    for (i = 1; i <= n; ++i) fprintf(stderr, "%d\n", i);
    return 0;
}



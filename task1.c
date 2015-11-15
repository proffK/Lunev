#include <unistd.h>
#include <stdio.h>

int main(int argc, char* argv[]) {

    if (argc == 1) {
        fprintf(stderr, "No argument");
    }

    execvp(argv[1], argv + 1);

    return 0;

}

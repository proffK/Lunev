#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {

    int fd1 = 0, fd2 = 0;
    struct stat st1;
    struct stat st2;

    fd1 = open(argv[1], O_RDONLY);
    fd2 = open(argv[1], O_RDONLY);

    fcntl(fd1, &st1);
    fcntl(fd2, &st2);

    print("dev_t %d %d\n", st1.dev_t, st2.dev_t);
    printf("\%d \%d : ino_t",st1.ino_t, st2.ino_t);
        printf("\%d \%d : mode_t",st1.mode_t, st2.mode_t);
        printf("\%d \%d : nlink_t",st1.nlink_t, st2.nlink_t);
        printf("\%d \%d : uid_t",st1.uid_t, st2.uid_t);
        printf("\%d \%d : gid_t",st1.gid_t, st2.gid_t);
        printf("\%d \%d : dev_t",st1.dev_t, st2.dev_t);
        printf("\%d \%d : off_t",st1.off_t, st2.off_t);

    return 0;


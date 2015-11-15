#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct {

    long mtype;
    int num;

} messg;

pid_t* pid_table = NULL;
int n = 0;
int counter = 1;

int is_child(messg* msg, int id);

int main(int argc, char* argv[]) {

    int opt = 0, i = 0, id = 0;
    char err = '\0';
    char help_msg[] = "Usage : -n print all number before n\n";
    pid_t cld_pid = 0;
    messg msg = {};
    key_t key = 0;
    pid_t my_pid = getpid();

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

    if ((key = ftok(argv[0], getpid())) == -1) {

        perror("Can't generate id of message queue");
        return 13;

    }

    if ((id = msgget(key, IPC_CREAT | /*IPC_EXCL |*/ 0666)) == -1) {

            perror("mq create error");
            return 4;

    }

    for (i = 0; i < n; ++i) {

        cld_pid = fork();

        if (cld_pid == -1) {

            perror("Too big number\n");
            msgctl(id, IPC_RMID, NULL);
            return 12;

        }

        if (cld_pid == 0) {

            is_child(&msg, id);
            return 0;

        }

        counter++;
   }

    msg.mtype = 1;

    if (msgsnd(id, &msg, sizeof(messg) - sizeof(long), 0) == -1) {

        msgctl(id, IPC_RMID, NULL);
        perror("mq send error");
        return 5;

    }

    if (msgrcv(id, &msg, sizeof(messg) - sizeof(long),  n + 1, 0) == -1) {

        msgctl(id, IPC_RMID, NULL);
        perror("mq recieve error");
        return 6;

    }

    msgctl(id, IPC_RMID, NULL);
    return 0;

}

int is_child(messg* msg, int id) {

    int i = 0;

    if (msgrcv(id, msg, sizeof(messg) - sizeof(long), counter, 0) == -1) {

        //free(pid_table);
        msgctl(id, IPC_RMID, NULL);
        fprintf(stderr, "mq recieve error %d", counter);
        return 6;

    }

    fprintf(stderr,"%d ", counter);

    msg->mtype = counter + 1;
    
    if (msgsnd(id, msg, sizeof(messg) - sizeof(long), 0) == -1) {

        //free(pid_table);
        msgctl(id, IPC_RMID, NULL);
        perror("mq send error");
        return 5;

    }
     
    return 0;

}

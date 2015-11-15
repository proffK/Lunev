#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define BUFFER_SIZE 4088
#define SEM_NUMS 5
#define SEM_PERM 0666
#define SHM_PERM 0666

#define EMPTY 0
#define FULL 1
#define MUTEX 2
#define REC 3
#define TRAN 4

#define PREPARE_OP(SEM, NUM, OP, FLAG) SEM.sem_num = (NUM);\
                                       SEM.sem_op = (OP);\
                                       SEM.sem_flg = (FLAG);

#define SEM_UP(x) PREPARE_OP(sop[0], (x), 1, SEM_UNDO)\
                  semop(semid, sop, 1);

#define SEM_DOWN(x) PREPARE_OP(sop[0], (x), -1, SEM_UNDO)\
                    semop(semid, sop, 1);

#define NUSEM_UP(x) PREPARE_OP(sop[0], (x), 1, 0)\
                    semop(semid, sop, 1);

#define NUSEM_DOWN(x) PREPARE_OP(sop[0], (x), -1, 0)\
                      semop(semid, sop, 1);

typedef struct {

    char buf[BUFFER_SIZE];
    size_t size;

} shared_buf;

int receive_mode(int fd_out, int semid, int shmid);

int send_mode(int fd_inp, int semid, int shmid);

int main(int argc, char* argv[]) {

    int shmid = 0;
    int semid = 0;
    key_t key = 0;
    int fd_inp = open(argv[1], O_RDONLY);

    if (fd_inp == -1 && argc > 1) {

        perror("File doesn't exist\n");
        return 1;

    }

    if ((key = ftok(argv[0], 1)) == -1) {

        perror("Can't gen ipc key.\n");
        return 1;

    }
    

    shmid = shmget(key, sizeof(shared_buf), IPC_CREAT | SHM_PERM);

    if (shmid == -1) {

        perror("Can't open shared memory");
        exit(EXIT_FAILURE);

    }

    semid = semget(key, SEM_NUMS, IPC_CREAT | SEM_PERM);

    if (semid == -1) {

        perror("Creat error\n");
        exit(EXIT_FAILURE);

    } 

    if (argc == 1) return receive_mode(STDOUT_FILENO, semid, shmid);

    return send_mode(fd_inp, semid, shmid);

}

int send_mode(int fd_inp, int semid, int shmid)
{
    shared_buf* shbuf = NULL;
    size_t cur_size = BUFFER_SIZE;
    struct sembuf sop[5];

    PREPARE_OP(sop[0], TRAN, 0, IPC_NOWAIT | SEM_UNDO);
    PREPARE_OP(sop[1], TRAN, 1, SEM_UNDO);
    PREPARE_OP(sop[2], EMPTY, 1, SEM_UNDO);
    //PREPARE_OP(sop[3], FULL, -1, SEM_UNDO);
    PREPARE_OP(sop[4], MUTEX, 1, SEM_UNDO);

    if (semop(semid, sop, 4) == -1) {

        if (errno != EAGAIN) {

            perror("Can't take TRAN sem\n");
            exit(EXIT_FAILURE);

        } else {

            return 0;

        }

    }
    
    shbuf = (shared_buf*) shmat (shmid, NULL, 0); 

    if (shbuf == (void*) -1) {

        perror("Can't attach to shared memory\n");
        exit(EXIT_FAILURE);

    }

    SEM_DOWN(EMPTY)  /* P(empty) */
    SEM_DOWN(MUTEX) /* P(mutex) */

    cur_size = read(fd_inp, shbuf -> buf, BUFFER_SIZE);
    shbuf -> size = cur_size;
    
    SEM_UP(MUTEX)   /* V(mutex) */
    SEM_UP(FULL)   /* V(full) */

    PREPARE_OP(sop[0], FULL, 0, 0) /* Wait reciever */
    semop(semid, sop, 1);

    while (cur_size == BUFFER_SIZE) {
        
        NUSEM_DOWN(EMPTY)  /* P(empty) */
        SEM_DOWN(MUTEX) /* P(mutex) */

        cur_size = read(fd_inp, shbuf -> buf, BUFFER_SIZE);
        shbuf -> size = cur_size;
        sleep(1);
        
        SEM_UP(MUTEX)   /* V(mutex) */
        NUSEM_UP(FULL)   /* V(full) */
    }

    if (shmdt(shbuf)) {

        perror("Can't detouch memory\n");
        exit(EXIT_FAILURE);

    }

    PREPARE_OP(sop[0], REC, 0, 0) /* Wait reciever's terminate */
    semop(semid, sop, 1);

    return 0;
}

int receive_mode(fd_out, semid, shmid)
{
    shared_buf* shbuf = NULL;
    size_t cur_size = BUFFER_SIZE;
    struct sembuf sop[3];

    shbuf = (shared_buf*) shmat (shmid, NULL, 0); 

    if (shbuf == (void*) -1) {

        perror("Can't attach to shared memory\n");
        exit(EXIT_FAILURE);

    }

    PREPARE_OP(sop[0], REC, 0, IPC_NOWAIT | SEM_UNDO)
    PREPARE_OP(sop[1], REC, 1, SEM_UNDO)
    //PREPARE_OP(sop[2], FULL, 1, SEM_UNDO);
    
    if (semop(semid, sop, 2) == -1) {

        if (errno != EAGAIN) {

            perror("Can't take REC sem\n");
            exit(EXIT_FAILURE);

        } else {

            return 0;

        }

    }

    SEM_DOWN(FULL) /* P(full) */
    SEM_DOWN(MUTEX) /* P(mutex) */

    cur_size = shbuf -> size;
    cur_size = write(fd_out, shbuf -> buf, cur_size);

    SEM_UP(MUTEX)   /* V(mutex) */
    SEM_UP(EMPTY)    /* V(empty) */

    while (cur_size == BUFFER_SIZE) {

        NUSEM_DOWN(FULL) /* P(full) */
        SEM_DOWN(MUTEX) /* P(mutex) */

        cur_size = shbuf -> size;
        cur_size = write(fd_out, shbuf -> buf, cur_size);

        SEM_UP(MUTEX)   /* V(mutex) */
        NUSEM_UP(EMPTY)    /* V(empty) */
    }

    if (semctl(semid, 0, IPC_RMID, 0)) {

        perror("Can't delete sems\n");
        exit(EXIT_FAILURE);

    }

    if (shmdt(shbuf)) {

        perror("Can't detouch memory\n");
        exit(EXIT_FAILURE);

    }

    return 0;
}

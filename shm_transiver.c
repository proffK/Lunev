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

const int MAX_TRIES = 10;

#define SEM_UP(x) sop.sem_num = x;\
                  sop.sem_op = 1;\
                  sop.sem_flg = SEM_UNDO;\
                  semop(semid, &sop, 1);

#define SEM_DOWN(x) sop.sem_num = x;\
                    sop.sem_op = -1;\
                    sop.sem_flg = SEM_UNDO;\
                    semop(semid, &sop, 1);

#define NUSEM_UP(x) sop.sem_num = x;\
                    sop.sem_op = 1;\
                    sop.sem_flg = 0;\
                    semop(semid, &sop, 1);

#define NUSEM_DOWN(x) sop.sem_num = x;\
                      sop.sem_op = -1;\
                      sop.sem_flg = 0;\
                      semop(semid, &sop, 1);

typedef struct {

    char buf[BUFFER_SIZE];
    size_t size;

} shared_buf;

union semun {

    int              val;   
    struct semid_ds *buf;  
    unsigned short  *array; 
    struct seminfo  *__buf;  
                             
};

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

    semid = semget(key, SEM_NUMS, IPC_CREAT | IPC_EXCL | SEM_PERM);

    if (semid != -1) {

        struct sembuf init_sop = {1, 0, 0};
        unsigned short initarr[SEM_NUMS] = {1, 0, 1, 1, 1};
        union semun arg;

        arg.array = initarr;

        if (semctl(semid, 0, SETALL, arg)) {

            perror("Can't init semaphores\n");
            exit(EXIT_FAILURE);

        }

        if (semop(semid, &init_sop, 1)) {

            perror("Can't do init operation\n");
            exit(EXIT_FAILURE);

        }

    } else if (errno != EEXIST) {

        perror("Creat error\n");
        exit(EXIT_FAILURE);

    } else {

        union semun arg;
        struct semid_ds ds;
        int i = 0;
        
        if ((semid = semget(key, SEM_NUMS, SEM_PERM)) == -1) {

            perror("Can't open sems\n");
            exit(EXIT_FAILURE);

        }

        arg.buf = &ds;

        for (i = 0; i < MAX_TRIES; ++i) {

            if (semctl(semid, 0, IPC_STAT, arg)) {

                perror("Can't read info about sems\n");
                exit(EXIT_FAILURE);

            }

            if (ds.sem_otime != 0) 
                break;

            sleep(1);

        }

        if (ds.sem_otime == 0) {

            perror("Existed sem non init.\n");
            semctl(semid, 0, IPC_RMID, arg);
            exit(EXIT_FAILURE);

        }

    }

    if (argc == 1) return receive_mode(STDOUT_FILENO, semid, shmid);

    return send_mode(fd_inp, semid, shmid);

}

int send_mode(int fd_inp, int semid, int shmid)
{
    shared_buf* shbuf = NULL;
    size_t cur_size = BUFFER_SIZE;
    struct sembuf sop;

    sop.sem_num = TRAN;
    sop.sem_op = -1;
    sop.sem_flg = IPC_NOWAIT | SEM_UNDO;

    if (semop(semid, &sop, 1) == -1) {

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

    while (cur_size == BUFFER_SIZE) {
        
        NUSEM_DOWN(EMPTY)  /* P(empty) */
        SEM_DOWN(MUTEX) /* P(mutex) */

        cur_size = read(fd_inp, shbuf -> buf, BUFFER_SIZE);
        shbuf -> size = cur_size;
        //sleep(1);
        
        SEM_UP(MUTEX)   /* V(mutex) */
        NUSEM_UP(FULL)   /* V(full) */
    }

    if (shmdt(shbuf)) {

        perror("Can't detouch memory\n");
        exit(EXIT_FAILURE);

    }

    SEM_UP(TRAN)

    return 0;
}

int receive_mode(fd_out, semid, shmid)
{
    shared_buf* shbuf = NULL;
    size_t cur_size = BUFFER_SIZE;
    struct sembuf sop;

    shbuf = (shared_buf*) shmat (shmid, NULL, 0); 

    if (shbuf == (void*) -1) {

        perror("Can't attach to shared memory\n");
        exit(EXIT_FAILURE);

    }

    sop.sem_num = REC;
    sop.sem_op = -1;
    sop.sem_flg = IPC_NOWAIT | SEM_UNDO;

    if (semop(semid, &sop, 1) == -1) {

        if (errno != EAGAIN) {

            perror("Can't take TRAN sem\n");
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

    SEM_UP(REC)

    return 0;
}

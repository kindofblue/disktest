#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>

typedef struct{
    int dir_num;
    int file_per_dir;
    int data_len;
    int read_count;
} TEST_SET;

TEST_SET test_set;
TEST_SET test_1k= { 1000, 10000, (1<<10), 100000};
TEST_SET test_32k= { 1000, 10000, (32<<10), 100000};
TEST_SET test_1M= { 500, 1000, (1<<20), 20000};
TEST_SET test_2M= { 200, 1000, (2<<20), 10000};
TEST_SET test_32M= { 50, 500, (32<<20), 1000};
TEST_SET test_32MQ= { 10, 10, (32<<20), 20};
TEST_SET test_64MQ= { 10, 10, (64<<20), 20};
TEST_SET test_1G= {1, 10, (1024<<20), 10};


static int stop = 0;

int create_dir () {
    int i;
    char dir_name[256];
    for (i = 0; i < test_set.dir_num; i ++) {
        sprintf(dir_name, "%d", i);
        mkdir(dir_name, 0777);
    }
    return 0;
}

int creat_files(void) {
    char file_name[256];
    char *buf;
    int i,j,dir_num,fpd,len;
    int percent;
    int fd;

    dir_num = test_set.dir_num;
    fpd = test_set.file_per_dir;
    len = test_set.data_len;
    percent = dir_num / 100;
    if (percent == 0) percent = 1;

    buf = memalign(4096,len);
    memset(buf,0x1,len);

    if (NULL == buf) {
        perror("Malloc:");
        exit(-1);
    }

    printf("Start creating files...\n");
    for (i = 0; i < dir_num; i++) {
        for (j=0; j < fpd; j++) {
            sprintf(file_name, "%d/%d", i, j);
            fd = open(file_name, O_CREAT|O_WRONLY|O_DIRECT, 0666);
            if (fd == -1) { 
                perror("file creat error.");
                exit(-1);
            }
            if (-1 == write(fd, buf, len)) {
                perror("write error:");
                exit(-1);
            }
            close(fd);
        }
        if (percent - 1 == i % percent) {
            printf("%d%% done:\n", i/percent);
            if (stop == 1) { 
                i++; 
                break;
            }
        }
    }
    sync();
    free(buf);
//    printf("Write: Total %d files done, %d MB\n", dir_num * fpd , dir_num * fpd * (len / 1024) / 1024);
    printf("Write: Total %d files done, %d MB\n", i * j , i * j * (len / 1024) / 1024);
    return 0;
}

int read_files(int nreads) {
    int i,x,y;
    unsigned int seed, rand_num, n;
    int fd;
    char *buf, fn[128];
    int dir_num,fpd,len;
    int percent;

    dir_num = test_set.dir_num;
    fpd = test_set.file_per_dir;
    len = test_set.data_len;
    buf = memalign(4096, len);
    percent = nreads/100;
    if (percent == 0) percent = 1;

    seed = time(NULL);
    srand(time(NULL));
    //rand_num = seed * 44318 + 2147483647;
    rand_num = seed * 4079 + 12923;
    //rand_num = seed + 2147483647;
    for (i = 0; i < nreads; i++) {
        //rand_num = rand_num + 2147483647;
        rand_num = rand_num * 4079 + 12923;
        //rand_num = rand();
        n = rand_num % (dir_num * fpd);
        x = n / fpd;
        y = n % fpd;

        sprintf(fn,"%d/%d",x,y);
        if (-1 == (fd = open(fn, O_RDONLY|O_DIRECT))){
            perror("file read error.");
            exit(-1);
        }
        if (-1 == read(fd, buf, len)){
            perror("Read Error:");
            exit(-1);
        }
        close(fd);

        if (percent - 1 == i % percent) {
            printf("%d%% done:\n", i/percent);
            if (stop == 1) break;
        }

    }
    free(buf);
    printf("Read: Total %d files done, %d MB\n", nreads, nreads * (len / 1024) / 1024);

    return 0;
}

int cleanup_files() {
    int i,j;
    int dir_num,fpd;
    char buf[256];
    int percent;

    dir_num = test_set.dir_num;
    fpd = test_set.file_per_dir;
    percent = dir_num / 100;
    if (percent == 0) percent = 1;

    for (i = 0; i < dir_num; i++) {
        for (j = 0; j < fpd; j ++) {
            sprintf(buf, "%d/%d", i, j);
            unlink(buf);
        }
        if (percent - 1 == i % percent) {
            printf("%d%% done:\n", i/percent);
        }
    }
    sync();
    printf("Delete: Total %d files done, %d MB\n", dir_num * fpd , dir_num * fpd * (test_set.data_len / 1024) / 1024);
    return 0;
}

void Ussage(char *name){

    printf("%s: FileSize Action\n",name);

}


void int_handler(int arg) {
    char msg[] = "CTRL-C received, wait for current percentage to finish...";
    write(1, msg, sizeof msg);
    stop = 1;
    return;

}
int main(int argc, char *argv[]) {

    char *act, *size;

    if (argc != 3){
        Ussage(argv[0]);
        exit(-1);
    }
    size = argv[1];
    act = argv[2];

    if (signal(SIGINT, int_handler) == SIG_ERR) {
        perror("signal:");
    }

    if (!strcmp(size,"32k")){
        test_set = test_32k;
    } else if (!strcmp(size,"1k")){
        test_set = test_1k;
    } else if (!strcmp(size,"1m")){
        test_set = test_1M;
    } else if (!strcmp(size,"2m")){
        test_set = test_2M;
    } else if (!strcmp(size,"32m")){
        test_set = test_32M;
    } else if (!strcmp(size,"32mq")){
        test_set = test_32MQ;
    } else if (!strcmp(size,"64mq")){
        test_set = test_64MQ;
    } else if (!strcmp(size, "1g"))  {
        test_set = test_1G;
    } else{
        Ussage(argv[0]);
        exit(-1);
    }

    if (!strcmp(act, "create")){
        create_dir();
    } else if (!strcmp(act, "write")) {
        creat_files();       
    } else if (!strcmp(act, "read")) {
        read_files(test_set.read_count);
    } else if (!strcmp(act, "cleanup")) {
        cleanup_files();
    } else{
        Ussage(argv[0]);
        exit(-1);
    }
    return 0;
}








#include <pthread.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <fnmatch.h>
#include <errno.h>
#include <signal.h>
const char* str;
int counter = 0;
int n_dir = 0;
int n_file = 0;
int n_thread;
pthread_mutex_t  lock;
pthread_cond_t notEmpty;
pthread_t* thread;


typedef  struct
{
    char path[1024];
    void* next_adress;
    void* prev_adress;
} dir_queue;


dir_queue* head_queue = NULL;
dir_queue* tail_queue = NULL;


void finish(int signum) {
    __sync_fetch_and_add(&counter, 1);
    pthread_t tid = pthread_self();
    if (signum == 2) {
        printf("Search stopped, found %d files\n", n_file);
        if (head_queue != NULL) {
            while (tail_queue->prev_adress != NULL)
            {
                tail_queue = tail_queue->prev_adress;
                free((void*)tail_queue->next_adress);
            }
        }
    }
    else {
        printf("Done searching, found %d files\n", n_file);
    }

    for (int i = 0; i < n_thread; ++i)
    {
        sleep(0.01);
        if (tid != thread[i])
        {
            pthread_mutex_unlock(&lock);
            pthread_cancel(thread[i]);


        }
    }
    free((void*)tail_queue);
    free((void*)thread);


    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&notEmpty);
    pthread_exit(NULL);
}

void add_dir_queue(const char* dir) {
    pthread_mutex_lock(&lock);
    n_dir++;
    dir_queue* arglist = (dir_queue*)malloc(sizeof(dir_queue));
    if (arglist == NULL) {
        printf("malloc failed: %s\n", strerror(errno));
        exit(1);
    }
    strcpy((arglist->path), dir);
    arglist->next_adress = (void*)head_queue;
    arglist->prev_adress = NULL;

    if (n_dir != 1) {
        head_queue->prev_adress = (void*)arglist;
    }
    else {

        tail_queue = arglist;
    }
    head_queue = arglist;
    pthread_cond_signal(&notEmpty);
    pthread_mutex_unlock(&lock);

}


int name_corresponds(const char* path, const char* str) {
    return fnmatch(str, strrchr(path, '/') + 1, 0) == 0;
}

void treat_file(const char* path) {
    struct stat file;
    stat(path, &file);
    if (name_corresponds(path, str)) {
        __sync_fetch_and_add(&n_file, 1);
        printf("%s\n", path);
    }
}

void browse(const char* path) {
    DIR* dir = opendir(path);
    struct dirent* entry;
    struct stat dir_stat;
    if (!dir) { perror(path); return; }
    while ((entry = readdir(dir)) != NULL) {
        char buff[strlen(path) + strlen(entry->d_name) + 2];
        sprintf(buff, "%s/%s", path, entry->d_name);
        stat(buff, &dir_stat);
        if (strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".") != 0) {
            if ((dir_stat.st_mode & __S_IFMT) == __S_IFDIR) {
                add_dir_queue(buff);
            }
            else
                treat_file(buff);
        }
    }
    closedir(dir);
    __sync_fetch_and_add(&counter, -1);
}

char* dequeue() {

    pthread_mutex_lock(&lock);

    while (n_dir == 0) {
        if (counter == 0) {
            raise(SIGUSR1);
        }

        pthread_cond_wait(&notEmpty, &lock);

    }
    __sync_fetch_and_add(&counter, 1);
    dir_queue* tmp = tail_queue;
    if (tail_queue->prev_adress != NULL) {
        tail_queue = tail_queue->prev_adress;
    }

    n_dir--;
    pthread_mutex_unlock(&lock);
    char* str_path = tmp->path;
    if (tmp->prev_adress != NULL) {
        free((void*)tmp->next_adress);
    }
    return str_path;
}

void* run_thread(void* t) {


    char* path;
    while ((path = dequeue()))
    {
        browse(path);
        pthread_testcancel();
    }
    return NULL;

}




void error() {
    printf("ERROR : Invalid number of arguments \n");
    exit(1);
}


int main(int argc, char** argv) {
    if (argc != 4) {
        error();
    }
    long   t;
    void* status;
    int pc;
    char* path;
    path = argv[1];
    str = argv[2];
    n_thread = atoi(argv[3]);
    thread = (pthread_t*)malloc(sizeof(pthread_t) * n_thread);

    add_dir_queue(path);

    struct sigaction func_sigint;
    struct sigaction func_siguser1;
    func_sigint.sa_handler = finish;
    func_siguser1.sa_handler = finish;
    sigaction(SIGUSR1, &func_sigint, NULL);
    sigaction(SIGINT, &func_siguser1, NULL);
    pc = pthread_mutex_init(&lock, NULL);
    if (pc)
    {
        printf("ERROR in pthread_mutex_init(): "
            "%s\n", strerror(pc));
        exit(1);
    }


    for (t = 0; t < n_thread; ++t)
    {
        pc = pthread_create(&thread[t], NULL, run_thread, (void*)t);
        if (pc)
        {
            printf("ERROR in pthread_create(): "
                "%s\n", strerror(pc));
            exit(1);
        }
    }

    for (t = 0; t < n_thread; ++t)
    {
        pc = pthread_join(thread[t], &status);
        if (pc)
        {
            printf("ERROR in pthread_join(): "
                "%s\n", strerror(pc));
            exit(1);
        }
    }
    return 0;
}





#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>

#include "buffer.h"

#define SEM_EMPTY "/sem_empty"
#define SEM_FULL "/sem_full"
#define SEM_MUTEX "/sem_mutex"

int main(int argc, char *argv[]) {
    int producer_id, num_items;
    int shm_id;
    shared_buffer_t *shared;
    sem_t *empty, *full, *mutex;
    int i;
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <id> <num_items>\n", argv[0]);
        exit(1);
    }
    
    producer_id = atoi(argv[1]);
    num_items = atoi(argv[2]);
    
    if (producer_id <= 0 || num_items <= 0) {
        fprintf(stderr, "Error: ID and num_items must be positive\n");
        exit(1);
    }
    shm_id = shmget(SHM_KEY, sizeof(shared_buffer_t), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }
    
    shared = (shared_buffer_t *)shmat(shm_id, NULL, 0);
    if (shared == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }
    
   empty = sem_open(SEM_EMPTY, O_CREAT, 0644, BUFFER_SIZE);
    if (empty == SEM_FAILED) {
        perror("sem_open empty failed");
        exit(1);
    }
    
    full = sem_open(SEM_FULL, O_CREAT, 0644, 0);
    if (full == SEM_FAILED) {
        perror("sem_open full failed");
        exit(1);
    }
    
    mutex = sem_open(SEM_MUTEX, O_CREAT, 0644, 1);
    if (mutex == SEM_FAILED) {
        perror("sem_open mutex failed");
        exit(1);
    }

    if (!shared->initialized) {
        shared->head = 0;
        shared->tail = 0;
        shared->count = 0;
        shared->initialized = 1;
        
        for (i = 0; i < BUFFER_SIZE; i++) {
            shared->buffer[i].value = 0;
            shared->buffer[i].producer_id = 0;
        }
        
        printf("Producer %d: Initialized shared memory\n", producer_id);
        fflush(stdout);
    }
    
    for (i = 0; i < num_items; i++) {
        int item_value = producer_id * 1000 + i;
        
        sem_wait(empty);
        
        sem_wait(mutex);
        
        shared->buffer[shared->head].value = item_value;
        shared->buffer[shared->head].producer_id = producer_id;
        shared->head = (shared->head + 1) % BUFFER_SIZE;
        shared->count++;
        
        printf("Producer %d: Produced value %d\n", producer_id, item_value);
        fflush(stdout);
        
	sem_post(mutex);
        
        sem_post(full);
        
        usleep(10000);
    }
    
    sem_close(empty);
    sem_close(full);
    sem_close(mutex);
    
    shmdt(shared);
    
    return 0;
}

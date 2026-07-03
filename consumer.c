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
    int consumer_id, num_items;
    int shm_id;
    shared_buffer_t *shared;
    sem_t *empty, *full, *mutex;
    int i;
    int item_value, item_producer;
    
    // PARSE COMMAND-LINE ARGUMENTS
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <id> <num_items>\n", argv[0]);
        exit(1);
    }
    
    consumer_id = atoi(argv[1]);
    num_items = atoi(argv[2]);
    
    if (consumer_id <= 0 || num_items <= 0) {
        fprintf(stderr, "Error: ID and num_items must be positive\n");
        exit(1);
    }
    
    // ACCESS SHARED MEMORY
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
    
    // Consumers open existing semaphores
    empty = sem_open(SEM_EMPTY, 0, 0644, 0);
    if (empty == SEM_FAILED) {
        perror("sem_open empty failed - make sure a producer runs first");
        exit(1);
    }
    
    full = sem_open(SEM_FULL, 0, 0644, 0);
    if (full == SEM_FAILED) {
        perror("sem_open full failed - make sure a producer runs first");
        exit(1);
    }
    
    mutex = sem_open(SEM_MUTEX, 0, 0644, 0);
    if (mutex == SEM_FAILED) {
        perror("sem_open mutex failed - make sure a producer runs first");
        exit(1);
    }
    
    for (i = 0; i < num_items; i++) {
        sem_wait(full);
        
        sem_wait(mutex);
        
        // Remove item from buffer
        item_value = shared->buffer[shared->tail].value;
        item_producer = shared->buffer[shared->tail].producer_id;
        shared->tail = (shared->tail + 1) % BUFFER_SIZE;
        shared->count--;
        
        printf("Consumer %d: Consumed value %d from Producer %d\n", consumer_id, item_value, item_producer);
        fflush(stdout);
        
        sem_post(mutex);
        
        sem_post(empty);
        
        usleep(10000); 
    }
    
    sem_close(empty);
    sem_close(full);
    sem_close(mutex);
    
    shmdt(shared);
    
    return 0;
}

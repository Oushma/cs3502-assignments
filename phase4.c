#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

// CONSTANTS
#define NUM_ACCOUNTS 5
#define NUM_THREADS 4
#define INITIAL_BALANCE 1000.0
#define TRANSFERS_PER_THREAD 10

// SHARED DATA WITH MUTEX PROTECTION
typedef struct {
    int account_id;
    double balance;
    int transaction_count;
    pthread_mutex_t lock;
} Account;

Account accounts[NUM_ACCOUNTS];

// Thread-safe deposit
void deposit(int account_id, double amount) {
    pthread_mutex_lock(&accounts[account_id].lock);
    accounts[account_id].balance += amount;
    accounts[account_id].transaction_count++;
    pthread_mutex_unlock(&accounts[account_id].lock);
}

// Thread-safe withdrawal
int withdraw(int account_id, double amount) {
    int success = 0;
    pthread_mutex_lock(&accounts[account_id].lock);
    if (accounts[account_id].balance >= amount) {
        accounts[account_id].balance -= amount;
        accounts[account_id].transaction_count++;
        success = 1;
    }
    pthread_mutex_unlock(&accounts[account_id].lock);
    return success;
}

// Thread-safe get balance
double get_balance(int account_id) {
    double balance;
    pthread_mutex_lock(&accounts[account_id].lock);
    balance = accounts[account_id].balance;
    pthread_mutex_unlock(&accounts[account_id].lock);
    return balance;
}

// DEADLOCK-FREE TRANSFER FUNCTION
// This function uses LOCK ORDERING to prevent deadlock.
void transfer(int from_id, int to_id, double amount) {
    int first_id, second_id;
    
    // Determine which account to lock first (always lower ID first)
    if (from_id < to_id) {
        first_id = from_id;
        second_id = to_id;
    } else {
        first_id = to_id;
        second_id = from_id;
    }
    
    printf("Thread %lu: Lock ordering: Account %d then Account %d\n", 
           pthread_self(), first_id, second_id);
    
    // Lock the smaller account first
    pthread_mutex_lock(&accounts[first_id].lock);
    printf("Thread %lu: Locked Account %d\n", pthread_self(), first_id);
    
    // Add a small delay 
    usleep(10000);
    
    // Lock the larger account second
    pthread_mutex_lock(&accounts[second_id].lock);
    printf("Thread %lu: Locked Account %d\n", pthread_self(), second_id);
    
    // Critical section - perform the transfer
    // Check if from account has enough balance
    double from_balance = accounts[from_id].balance;
    
    if (from_balance >= amount) {
        accounts[from_id].balance -= amount;
        accounts[to_id].balance += amount;
        accounts[from_id].transaction_count++;
        accounts[to_id].transaction_count++;
        printf("Thread %lu: Transferred $%.2f from Account %d to Account %d\n", 
               pthread_self(), amount, from_id, to_id);
    } else {
        printf("Thread %lu: Insufficient funds for transfer $%.2f from Account %d\n", 
               pthread_self(), amount, from_id);
    }
    
    // Unlock in reverse order
    pthread_mutex_unlock(&accounts[second_id].lock);
    printf("Thread %lu: Unlocked Account %d\n", pthread_self(), second_id);
    pthread_mutex_unlock(&accounts[first_id].lock);
    printf("Thread %lu: Unlocked Account %d\n", pthread_self(), first_id);
}

// THREAD FUNCTION 
void* teller_thread(void* arg) {
    int teller_id = *(int*)arg;
    unsigned int seed = time(NULL) ^ pthread_self() ^ (teller_id * 12345);
    
    printf("Teller %d: Starting\n", teller_id);
    
    // Same deadlock-prone scenario as Phase 3:
    int from_id, to_id;
    if (teller_id % 2 == 0) {
        from_id = 0;
        to_id = 1;
        printf("Teller %d: Will transfer from Account %d to Account %d\n", 
               teller_id, from_id, to_id);
    } else {
        from_id = 1;
        to_id = 0;
        printf("Teller %d: Will transfer from Account %d to Account %d\n", 
               teller_id, from_id, to_id);
    }
    
    for (int i = 0; i < TRANSFERS_PER_THREAD; i++) {
        double amount = (double)((rand_r(&seed) % 50) + 1);
        
        printf("Teller %d: Attempting transfer %d: $%.2f from Account %d to Account %d\n",
               teller_id, i+1, amount, from_id, to_id);
        
        transfer(from_id, to_id, amount);
        
        // Random delay between transfers
        usleep((rand_r(&seed) % 5000));
    }
    
    printf("Teller %d: Finished\n", teller_id);
    return NULL;
}

// ============================================
// MAIN
// ============================================
int main() {
    pthread_t threads[NUM_THREADS];
    int* thread_ids[NUM_THREADS];
    double total_expected = 0.0;
    
    printf("=== Phase 4: Deadlock Resolution (Lock Ordering) ===\n\n");
    
    // Initialize accounts and mutexes
    printf("Initializing %d accounts with $%.2f each\n", NUM_ACCOUNTS, INITIAL_BALANCE);
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        accounts[i].transaction_count = 0;
        if (pthread_mutex_init(&accounts[i].lock, NULL) != 0) {
            perror("pthread_mutex_init failed");
            exit(1);
        }
        total_expected += INITIAL_BALANCE;
    }
    printf("Total expected balance: $%.2f\n\n", total_expected);
    
    // Show deadlock resolution strategy
    printf("DEADLOCK RESOLUTION STRATEGY: Lock Ordering\n");
    printf("  Even tellers (0, 2): Transfer from Account 0 to Account 1\n");
    printf("  Odd tellers (1, 3):  Transfer from Account 1 to Account 0\n");
    printf("  FIX: Always lock the LOWER account ID first\n");
    
    // Create threads
    printf("Creating %d teller threads...\n\n", NUM_THREADS);
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = malloc(sizeof(int));
        *thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, teller_thread, thread_ids[i]) != 0) {
            perror("pthread_create failed");
            exit(1);
        }
    }
    
    // Wait for all threads to finish
    printf("Waiting for all threads to complete...\n");
    printf("(This should ALWAYS complete with lock ordering!)\n\n");
    
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("Joining Teller %d...\n", i);
        int result = pthread_join(threads[i], NULL);
        if (result == 0) {
            printf("Teller %d joined successfully.\n", i);
        } else {
            printf("Error joining Teller %d: %s\n", i, strerror(result));
        }
        free(thread_ids[i]);
    }
    
    // All threads completed!
    printf("\nAll threads completed successfully! (No deadlock!)\n");
    
    // Show results
    printf("\n=== Final Results ===\n");
    double total_actual = 0.0;
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        double balance = get_balance(i);
        int transactions;
        pthread_mutex_lock(&accounts[i].lock);
        transactions = accounts[i].transaction_count;
        pthread_mutex_unlock(&accounts[i].lock);
        printf("Account %d: $%.2f (%d transactions)\n", i, balance, transactions);
        total_actual += balance;
    }
    
    printf("\n=== Balance Verification ===\n");
    printf("Starting balance:     $%.2f\n", total_expected);
    printf("Actual ending:        $%.2f\n", total_actual);
    printf("Difference:           $%.2f\n", total_expected - total_actual);
    
    if (total_actual == total_expected) {
        printf("\nNO DEADLOCK DETECTED!\n");
        printf("   Lock ordering successfully prevented deadlock.\n");
        printf("   All threads completed and balances are consistent.\n");
    } else {
        printf("\nBALANCE MISMATCH: Some transactions may not have completed.\n");
        printf("   This should not happen with lock ordering.\n");
    }
    
    // Clean up mutexes
    printf("\nCleaning up mutexes...\n");
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        pthread_mutex_destroy(&accounts[i].lock);
    }
    
    return 0;
}

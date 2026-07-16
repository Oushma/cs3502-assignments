#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

// CONSTANTS
#define NUM_ACCOUNTS 5
#define NUM_THREADS 8
#define INITIAL_BALANCE 1000.0

// Transaction types
#define DEPOSIT 0
#define WITHDRAW 1

// SHARED DATA WITH MUTEX PROTECTION
typedef struct {
    int account_id;
    double balance;
    int transaction_count;
    pthread_mutex_t lock;
} Account;

Account accounts[NUM_ACCOUNTS];

// THREAD-SAFE FUNCTIONS
void deposit(int account_id, double amount) {
    pthread_mutex_lock(&accounts[account_id].lock);
    accounts[account_id].balance += amount;
    accounts[account_id].transaction_count++;
    pthread_mutex_unlock(&accounts[account_id].lock);
}

void withdraw(int account_id, double amount) {
    pthread_mutex_lock(&accounts[account_id].lock);
    if (accounts[account_id].balance >= amount) {
        accounts[account_id].balance -= amount;
        accounts[account_id].transaction_count++;
    }
    pthread_mutex_unlock(&accounts[account_id].lock);
}

double get_balance(int account_id) {
    double balance;
    pthread_mutex_lock(&accounts[account_id].lock);
    balance = accounts[account_id].balance;
    pthread_mutex_unlock(&accounts[account_id].lock);
    return balance;
}

// THREAD FUNCTION
void* teller_thread(void* arg) {
    int teller_id = *(int*)arg;
    unsigned int seed = time(NULL) ^ pthread_self() ^ (teller_id * 12345);
    double amount;
    int account_id;
    int action;
    int transaction_count;
    
    // CLEAN SCHEDULE - All accounts end at predictable values
    switch(teller_id) {
        case 0:  // Deposit $10 to Account 0, 50 times (+$500)
            account_id = 0; amount = 10.0; action = DEPOSIT; transaction_count = 50;
            printf("Teller %d: DEPOSIT  $%.2f to Account %d (%d times) [WITH LOCK]\n", 
                   teller_id, amount, account_id, transaction_count);
            break;
        case 1:  // Deposit $5 to Account 1, 100 times (+$500)
            account_id = 1; amount = 5.0; action = DEPOSIT; transaction_count = 100;
            printf("Teller %d: DEPOSIT  $%.2f to Account %d (%d times) [WITH LOCK]\n", 
                   teller_id, amount, account_id, transaction_count);
            break;
        case 2:  // Withdraw $10 from Account 2, 100 times (-$1000, drains to $0)
            account_id = 2; amount = 10.0; action = WITHDRAW; transaction_count = 100;
            printf("Teller %d: WITHDRAW $%.2f from Account %d (%d times) [WITH LOCK]\n", 
                   teller_id, amount, account_id, transaction_count);
            break;
        case 3:  // Withdraw $5 from Account 3, 100 times (-$500)
            account_id = 3; amount = 5.0; action = WITHDRAW; transaction_count = 100;
            printf("Teller %d: WITHDRAW $%.2f from Account %d (%d times) [WITH LOCK]\n", 
                   teller_id, amount, account_id, transaction_count);
            break;
        case 4:  // Deposit $8 to Account 4, 75 times (+$600)
            account_id = 4; amount = 8.0; action = DEPOSIT; transaction_count = 75;
            printf("Teller %d: DEPOSIT  $%.2f to Account %d (%d times) [WITH LOCK]\n", 
                   teller_id, amount, account_id, transaction_count);
            break;
        case 5:  // Withdraw $6 from Account 0, 80 times (-$480)
            account_id = 0; amount = 6.0; action = WITHDRAW; transaction_count = 80;
            printf("Teller %d: WITHDRAW $%.2f from Account %d (%d times) [WITH LOCK]\n", 
                   teller_id, amount, account_id, transaction_count);
            break;
        case 6:  // Deposit $4 to Account 1, 150 times (+$600)
            account_id = 1; amount = 4.0; action = DEPOSIT; transaction_count = 150;
            printf("Teller %d: DEPOSIT  $%.2f to Account %d (%d times) [WITH LOCK]\n", 
                   teller_id, amount, account_id, transaction_count);
            break;
        case 7:  // Deposit $2 to Account 2, 50 times (+$100)
            account_id = 2; amount = 2.0; action = DEPOSIT; transaction_count = 50;
            printf("Teller %d: DEPOSIT  $%.2f to Account %d (%d times) [WITH LOCK]\n", 
                   teller_id, amount, account_id, transaction_count);
            break;
        default:
            return NULL;
    }
    
    for (int i = 0; i < transaction_count; i++) {
        if (action == DEPOSIT) {
            deposit(account_id, amount);
        } else {
            withdraw(account_id, amount);
        }
        usleep(rand_r(&seed) % 1000);
    }
    
    printf("Teller %d: Finished (%d transactions)\n", 
           teller_id, transaction_count);
    return NULL;
}

// ============================================
// MAIN
// ============================================
int main() {
    pthread_t threads[NUM_THREADS];
    int* thread_ids[NUM_THREADS];
    double total_expected = 0.0;
    
    printf("=== Phase 2: Mutex Protection (WITH Locks) ===\n\n");
    
    // Initialize accounts with mutexes
    printf("Initializing %d accounts with $%.2f each\n", NUM_ACCOUNTS, INITIAL_BALANCE);
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        accounts[i].transaction_count = 0;
        pthread_mutex_init(&accounts[i].lock, NULL);
        total_expected += INITIAL_BALANCE;
    }
    printf("Total expected balance: $%.2f\n\n", total_expected);
    
    // Show transaction schedule
    printf("Transaction Schedule (8 tellers, WITH mutex protection):\n");
    printf("  Teller 0: DEPOSIT  $10.00 to Account 0 (50 times) -> +$500\n");
    printf("  Teller 1: DEPOSIT  $5.00  to Account 1 (100 times) -> +$500\n");
    printf("  Teller 2: WITHDRAW $10.00 from Account 2 (100 times) -> -$1000 (drains to $0)\n");
    printf("  Teller 3: WITHDRAW $5.00  from Account 3 (100 times) -> -$500\n");
    printf("  Teller 4: DEPOSIT  $8.00  to Account 4 (75 times) -> +$600\n");
    printf("  Teller 5: WITHDRAW $6.00  from Account 0 (80 times) -> -$480\n");
    printf("  Teller 6: DEPOSIT  $4.00  to Account 1 (150 times) -> +$600\n");
    printf("  Teller 7: DEPOSIT  $2.00  to Account 2 (50 times) -> +$100\n\n");
    
    // Calculate expected final balances
    double expected[5];
    expected[0] = INITIAL_BALANCE + (50 * 10.0) - (80 * 6.0);   // 1000 + 500 - 480 = 1020
    expected[1] = INITIAL_BALANCE + (100 * 5.0) + (150 * 4.0);   // 1000 + 500 + 600 = 2100
    expected[2] = INITIAL_BALANCE - (100 * 10.0) + (50 * 2.0);   // 1000 - 1000 + 100 = 100
    expected[3] = INITIAL_BALANCE - (100 * 5.0);                 // 1000 - 500 = 500
    expected[4] = INITIAL_BALANCE + (75 * 8.0);                  // 1000 + 600 = 1600
    double expected_total = expected[0] + expected[1] + expected[2] + 
                           expected[3] + expected[4];            // 1020+2100+100+500+1600 = 5320
    
    printf("Expected final balances:\n");
    printf("  Account 0: $%.2f\n", expected[0]);
    printf("  Account 1: $%.2f\n", expected[1]);
    printf("  Account 2: $%.2f (drained then partially refilled)\n", expected[2]);
    printf("  Account 3: $%.2f\n", expected[3]);
    printf("  Account 4: $%.2f\n\n", expected[4]);
    
    // Create threads
    printf("Creating %d teller threads...\n\n", NUM_THREADS);
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = malloc(sizeof(int));
        *thread_ids[i] = i;
        pthread_create(&threads[i], NULL, teller_thread, thread_ids[i]);
    }
    
    // Wait for all threads to finish
    printf("\nWaiting for all threads to complete...\n\n");
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        free(thread_ids[i]);
    }
    
    // Show results
    printf("\n=== Final Results ===\n");
    double total_actual = 0.0;
    int all_correct = 1;
    
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        double balance = get_balance(i);
        int transactions;
        pthread_mutex_lock(&accounts[i].lock);
        transactions = accounts[i].transaction_count;
        pthread_mutex_unlock(&accounts[i].lock);
        
        printf("Account %d: $%.2f (expected: $%.2f) [%d transactions] %s\n", 
               i, balance, expected[i], transactions,
               balance == expected[i] ? "CORRECT" : "INCORRECT");
        if (balance != expected[i]) all_correct = 0;
        total_actual += balance;
    }
    
    // Verification
    printf("\n=== Balance Verification ===\n");
    printf("Starting balance:     $%.2f\n", total_expected);
    printf("Expected ending:      $%.2f\n", expected_total);
    printf("Actual ending:        $%.2f\n", total_actual);
    printf("Difference:           $%.2f\n", expected_total - total_actual);
    
    if (all_correct && total_actual == expected_total) {
        printf("\nSUCCESS: No race conditions detected.\n");
        printf("   All transactions properly synchronized with mutexes.\n");
    } else {
        printf("\nERROR: Balance mismatch! Race condition detected.\n");
    }
    
    // Clean up mutexes
    printf("\nCleaning up mutexes...\n");
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        pthread_mutex_destroy(&accounts[i].lock);
    }
    
    return 0;
}

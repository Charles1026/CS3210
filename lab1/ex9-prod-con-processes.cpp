/*******************************************************************
 * ex789-prod-con-threads.cpp
 * Producer-consumer synchronisation problem in C++
 *******************************************************************/

#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define DO_LOGGING false

constexpr int PRODUCERS = 2;
constexpr int CONSUMERS = 1;

constexpr int MAX_BUFFER_LEN = 10;
constexpr int TOTAL_SHM_SIZE = MAX_BUFFER_LEN + 1 + 2;
constexpr int CONSUMER_SUM_OFFSET = 0;
constexpr int CURR_PROD_OFFSET = 1;
constexpr int CURR_CONS_OFFSET = 2;
constexpr int BUFFER_OFFSET = 3;
int (*shm)[TOTAL_SHM_SIZE] = NULL; // consumer_sum, curr_prod, curr_cons, buffer baked into 1
int SHM_KEY = 420; // consumer_sum, curr_prod, curr_cons, buffer baked into 1
/* !!!!!! KEY ASSUMPTION !!!!!! That the program will not run long enought for curr_prod or curr_cons to overflow. */
constexpr int NUM_SEMS = 3;
sem_t (*sems)[NUM_SEMS]; // shm sem , prod sem and cons sem baked into 1
constexpr int SHM_SEM_OFFSET = 0;
constexpr int PROD_SEM_OFFSET = 1;
constexpr int CONS_SEM_OFFSET = 2;

volatile bool *has_ended = NULL;

// Testing stuff
int (*todos)[2] = NULL; // to_prod, to_cons
constexpr int TO_PROD_OFFSET = 0;
constexpr int TO_CONS_OFFSET = 1;

void producer(int producer_id)
{
	while (!(*has_ended)) {
    int randInt = (rand() % 10) + 1; // random number between 1 to 10
    sem_wait(&((*sems)[PROD_SEM_OFFSET])); // Wait on buffer to not be full
    sem_wait(&((*sems)[SHM_SEM_OFFSET])); // acquire SHM Semaphore
    if (((*todos)[TO_PROD_OFFSET])-- <= 0) {
      sem_post(&((*sems)[SHM_SEM_OFFSET]));
      sem_post(&((*sems)[CONS_SEM_OFFSET]));
      break;
    }

    (*shm)[BUFFER_OFFSET + ((*shm)[CURR_PROD_OFFSET] % MAX_BUFFER_LEN)] = randInt;
    ++((*shm)[CURR_PROD_OFFSET]);
    #if DO_LOGGING
    printf("Producer %d adding %d to buffer, curr buffer size %d.\n", producer_id, randInt, (*shm)[CURR_PROD_OFFSET] - (*shm)[CURR_CONS_OFFSET]);
    #endif
    sem_post(&((*sems)[SHM_SEM_OFFSET])); // Release SHM Semaphore
    sem_post(&((*sems)[CONS_SEM_OFFSET])); // Post on cons sem as it is no longer empty
  }
  printf("Producer %d exiting.\n", producer_id);
}

void consumer(int consumer_id)
{
	while (!(*has_ended)) {
    sem_wait(&((*sems)[CONS_SEM_OFFSET])); // Wait on buffer to not be empty
    sem_wait(&((*sems)[SHM_SEM_OFFSET])); // acquire SHM Semaphore
    if (((*todos)[TO_CONS_OFFSET])-- <= 0) {
      sem_post(&((*sems)[SHM_SEM_OFFSET]));
      sem_post(&((*sems)[PROD_SEM_OFFSET]));
      break;
    }

    int consume_idx = (*shm)[CURR_CONS_OFFSET] % MAX_BUFFER_LEN;
    (*shm)[CONSUMER_SUM_OFFSET] += (*shm)[BUFFER_OFFSET + consume_idx];
    ++((*shm)[CURR_CONS_OFFSET]);
    #if DO_LOGGING
    printf("Consumer %d taking %d from buffer, curr buffer size %d. Sum now %d.\n", consumer_id, (*shm)[BUFFER_OFFSET + consume_idx], 
        (*shm)[CURR_PROD_OFFSET] - (*shm)[CURR_CONS_OFFSET], (*shm)[CONSUMER_SUM_OFFSET]);
    #endif
    sem_post(&((*sems)[SHM_SEM_OFFSET])); // Release SHM Semaphore
    sem_post(&((*sems)[PROD_SEM_OFFSET])); // Post on prod sem as it is no longer full
  }
  printf("Consumer %d exiting.\n", consumer_id);
}

void handle_sigint(int sig) {
  printf("\nCaught signal %d (SIGINT). Exiting gracefully...\n", sig);
  *has_ended = true;
}

bool shm_detach() {
  if (shmdt(shm) == -1) {
    printf("Failed to detach shared memory for prod cons.\n");
    return false;
  }

  if (shmdt(sems) == -1) {
    printf("Failed to detach shared memory for sems.\n");
    return false;
  }

  if (shmdt(todos) == -1) {
    printf("Failed to detach shared memory for todos.\n");
    return false;
  }

  return true;
}

int main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("Please enter num of iterations to run.\n");
    return -1;
  }

  // Create sem shared memory
  int sem_shm_id = shmget(SHM_KEY++, sizeof(sem_t) * NUM_SEMS, IPC_CREAT | 0666);
  if (sem_shm_id < 0) {
      printf("Failed to create sem shared memory.\n");
      exit(-1);
  }
  sems = (sem_t (*)[NUM_SEMS])shmat(sem_shm_id, NULL, 0);
  printf("SHM shared memory segment created with ID: %d\n", sem_shm_id);

  // Create SHM Sem
  if (sem_init(&((*sems)[SHM_SEM_OFFSET]), 1, 1) == -1) {  // pshared = 1 for sharing between processes
    printf("Failed to initialize shm semaphore.\n");
    exit(-1);
  }

  // Create Prod Sem
  if (sem_init(&((*sems)[PROD_SEM_OFFSET]), 1, MAX_BUFFER_LEN) == -1) {  // pshared = 1 for sharing between processes
    printf("Failed to initialize prod semaphore.\n");
    exit(-1);
  }

  // Create Cons Sem
  if (sem_init(&((*sems)[CONS_SEM_OFFSET]), 1, 0) == -1) {  // pshared = 1 for sharing between processes
    printf("Failed to initialize cons semaphore.\n");
    exit(-1);
  }

  // Create prod cons shared memory
  int shm_id = shmget(SHM_KEY++, sizeof(int) * TOTAL_SHM_SIZE, IPC_CREAT | 0666);
  if (shm_id < 0) {
      printf("Failed to create prod cons shared memory.\n");
      exit(-1);
  }
  shm = (int (*)[TOTAL_SHM_SIZE])shmat(shm_id, NULL, 0);
  printf("Prod cons shared memory segment created with ID: %d\n", shm_id);
  // Initalise values for prod cons shm
  for (int i = 0; i < TOTAL_SHM_SIZE; ++i) {
    (*shm)[i] = 0;
  }

  // Create sigint shared memory
  int sigint_shm_id = shmget(SHM_KEY++, sizeof(bool), IPC_CREAT | 0666);
  if (sigint_shm_id < 0) {
      printf("Failed to create sigint shared memory.\n");
      exit(-1);
  }
  has_ended = (volatile bool *)shmat(sigint_shm_id, NULL, 0);
  printf("Shared memory segment created with ID: %d\n", sigint_shm_id);
  *has_ended = false; // initalise value of sigint check
  signal(SIGINT, handle_sigint); // set signal handler

  
  // Create testing shared memory
  int todo_shm_id = shmget(SHM_KEY++, sizeof(int) * 2, IPC_CREAT | 0666);
  if (todo_shm_id < 0) {
      printf("Failed to create sigint shared memory.\n");
      exit(-1);
  }
  todos = (int (*)[2])shmat(todo_shm_id, NULL, 0);
  printf("Shared memory segment created with ID: %d\n", todo_shm_id);
  // initalise value of todos
  (*todos)[TO_PROD_OFFSET] = atoi(argv[1]); 
  (*todos)[TO_CONS_OFFSET] = atoi(argv[1]);

  int producer_childid[PRODUCERS];
  int consumer_childid[CONSUMERS];

  for (int p = 0; p < PRODUCERS; p++)
  {
    int child_pid = fork();
    // Child
    if (child_pid == 0) {
      producer(p);
      exit(shm_detach() * -1);
    } 
    
    // Handle Error
    if (child_pid < 0) {
      printf("Error: failed to create producer %d\n", p);
      exit(-1);
    }

    producer_childid[p] = child_pid;
    printf("Main: created producer %d with pid %d.\n", p, child_pid);
  }

  for (int c = 0; c < CONSUMERS; c++)
  {
    int child_pid = fork();
    // Child
    if (child_pid == 0) {
      consumer(c);
      exit(shm_detach() * -1);
    } 
    
    // Handle Error
    if (child_pid < 0) {
      printf("Error: failed to create consumer %d\n", c);
      exit(-1);
    }

    consumer_childid[c] = child_pid;
    printf("Main: created consumer %d with pid %d.\n", c, child_pid);
  }

  for (int i = 0; i < PRODUCERS + CONSUMERS; ++i) {
    wait(NULL);
  }
  printf("Final Consumer Sum is: %d.\n", (*shm)[CONSUMER_SUM_OFFSET]); // No need for sem here as only process left

  // int val;
  // sem_getvalue(&((*sems)[CONS_SEM_OFFSET]), &val);
  // printf("Final cons sem value is: %d.\n", val);
  // sem_getvalue(&((*sems)[PROD_SEM_OFFSET]), &val);
  // printf("Final prod sem value is: %d.\n", val);

  bool success = true;

  // Semaphore cleanup
  if (sem_destroy(&((*sems)[SHM_SEM_OFFSET])) == -1) {
    printf("Failed to destroy shm semaphore.\n");
    success = false;
  }
  printf("Destroyed shm semaphore.\n");

  if (sem_destroy(&((*sems)[PROD_SEM_OFFSET])) == -1) {
    printf("Failed to destroy prod semaphore.\n");
    success = false;
  }
  printf("Destroyed prod semaphore.\n");

  if (sem_destroy(&((*sems)[CONS_SEM_OFFSET])) == -1) {
    printf("Failed to destroy cons semaphore.\n");
    success = false;
  }
  printf("Destroyed cons semaphore.\n");

  // SHM cleanup
  success = shm_detach();

  if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
    printf("Failed to delete prod cons shared memory.\n");
    success = false;
  }
  printf("Deleted prod cons shared memory.\n");

  if (shmctl(sem_shm_id, IPC_RMID, NULL) == -1) {
    printf("Failed to delete sems shared memory.\n");
    success = false;
  }
  printf("Deleted sems shared memory.\n");

  // Cleanup sigint shm
  if (shmdt((void *)has_ended) == -1) {
    printf("Failed to detach shared memory for sigint.\n");
  }
  printf("Detached sigint shared memory.\n");

  if (shmctl(sigint_shm_id, IPC_RMID, NULL) == -1) {
    printf("Failed to delete sigint shared memory.\n");
  }
  printf("Deleted sigint shared memory.\n");

  // Cleanup todo shm
  if (shmctl(todo_shm_id, IPC_RMID, NULL) == -1) {
    printf("Failed to delete todos shared memory.\n");
  }
  printf("Deleted todos shared memory.\n");
	/*
					some tips for this exercise:

					1. you may want to handle SIGINT (ctrl-C) so that your program
									can exit cleanly (by killing all threads, or just calling
		 exit)

					1a. only one thread should handle the signal (POSIX does not define
									*which* thread gets the signal), so it's wise to mask out the
		 signal on the worker threads (producer and consumer) and let the main
		 thread handle it
	*/
}

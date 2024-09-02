/*******************************************************************
 * ex789-prod-con-threads.cpp
 * Producer-consumer synchronisation problem in C++
 *******************************************************************/

#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <signal.h>
#include <chrono>

#define DO_LOGGING false

constexpr int PRODUCERS = 2;
constexpr int CONSUMERS = 1;

constexpr int MAX_BUFFER_LEN = 10;
int consumer_sum = 0;
int producer_buffer[MAX_BUFFER_LEN];
/* !!!!!! KEY ASSUMPTION !!!!!! That the program will not run long enought for curr_prod or curr_cons to overflow. */
int curr_prod = 0; // Tracks production idx
int curr_cons = 0; // Tracks consumption idx
pthread_mutex_t shared_var_lock = PTHREAD_MUTEX_INITIALIZER; // locks both the producer buffer and consumer sum
pthread_cond_t wait_not_full_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t wait_not_empty_cond = PTHREAD_COND_INITIALIZER;

volatile bool has_ended = false;

// Testing stuff
constexpr int SAMPLES = 10;
constexpr int START_NUM_ITER = 10000;
constexpr int SAMPLE_DISTANCE = 10000;
constexpr int END_NUM_ITER = (SAMPLES * SAMPLE_DISTANCE) + START_NUM_ITER;
int to_prod = 0, to_cons = 0;

void *producer(void *threadid)
{
  int tid = *((int *)threadid);
	while ((!has_ended) && (to_prod-- > 0)) {
    int randInt = (rand() % 10) + 1; // random number between 1 to 10

    pthread_mutex_lock(&shared_var_lock); // Get Mutex
    // Wait for there to be empty space on the buffer, check on has_ended also to prevent infinite wait on termination and full buffer
    while ((curr_prod - curr_cons >= MAX_BUFFER_LEN) && !has_ended) {
      pthread_cond_wait(&wait_not_full_cond, &shared_var_lock);
    }
    producer_buffer[curr_prod % MAX_BUFFER_LEN] = randInt;
    ++curr_prod;
    #if DO_LOGGING
    printf("Producer %d adding %d to buffer, curr buffer size %d.\n", tid, randInt, curr_prod - curr_cons);
    #endif
    pthread_mutex_unlock(&shared_var_lock); // Release Mutex
    pthread_cond_broadcast(&wait_not_empty_cond); // As we added to buffer it is definitely not empty
  }
  printf("Producer %d exiting.\n", tid);
  return NULL;
}

void *consumer(void *threadid)
{
  int tid = *((int *)threadid);
	while ((!has_ended) && (to_cons-- > 0)) {
    pthread_mutex_lock(&shared_var_lock); // Get Mutex
    // Wait for there to be value on the buffer, check on has_ended also to prevent infinite wait on termination and empty buffer
    while ((curr_prod - curr_cons <= 0) && !has_ended) {
      pthread_cond_wait(&wait_not_empty_cond, &shared_var_lock);
    }
    int consume_idx = curr_cons % MAX_BUFFER_LEN;
    consumer_sum += producer_buffer[consume_idx];
    ++curr_cons;
    #if DO_LOGGING
    printf("Consumer %d taking %d from buffer, curr buffer size %d. Sum now %d.\n", tid, producer_buffer[consume_idx], curr_prod - curr_cons, consumer_sum);
    #endif
    pthread_mutex_unlock(&shared_var_lock); // Release Mutex    
    pthread_cond_broadcast(&wait_not_full_cond); // As we took from buffer it is definitely not full
  }
  printf("Consumer %d exiting.\n", tid);
  return NULL;
}

void handle_sigint(int sig) {
  printf("\nCaught signal %d (SIGINT). Exiting gracefully...\n", sig);
  has_ended = true;
}

int main(int argc, char *argv[])
{
  signal(SIGINT, handle_sigint);
  FILE *log_csv = fopen("./threads_timings.csv", "w+");
  fprintf(log_csv, "Iterations, Duration, Sum\n");
  for (int i = START_NUM_ITER; i < END_NUM_ITER; i += SAMPLE_DISTANCE) {
    to_prod = i;
    to_cons = i;
    consumer_sum = 0;
    curr_prod = 0;
    curr_cons = 0;
    auto start_time = std::chrono::system_clock::now();

    pthread_t producer_threads[PRODUCERS];
    pthread_t consumer_threads[CONSUMERS];
    int producer_threadid[PRODUCERS];
    int consumer_threadid[CONSUMERS];

    int rc;
    int t1, t2;

    sigset_t omask, mask;
    sigfillset(&mask);
    pthread_sigmask(SIG_SETMASK, &mask, &omask);

    for (t1 = 0; t1 < PRODUCERS; t1++)
    {
      int tid = t1;
      producer_threadid[tid] = tid;
      printf("Main: creating producer %d\n", tid);
      rc = pthread_create(&producer_threads[tid], NULL, producer,
                (void *)&producer_threadid[tid]);
      if (rc)
      {
        printf("Error: Return code from pthread_create() is %d\n", rc);
        has_ended = true;
        exit(-1);
      }
    }

    for (t2 = 0; t2 < CONSUMERS; t2++)
    {
      int tid = t2;
      consumer_threadid[tid] = tid;
      printf("Main: creating consumer %d\n", tid);
      rc = pthread_create(&consumer_threads[tid], NULL, consumer,
                (void *)&consumer_threadid[tid]);
      if (rc)
      {
        printf("Error: Return code from pthread_create() is %d\n", rc);
        has_ended = true;
        exit(-1);
      }
    }

    pthread_sigmask(SIG_SETMASK, &omask, NULL);
    for (int i = 0; i < PRODUCERS; ++i) {
      pthread_join(producer_threads[i], NULL);
    }
    for (int i = 0; i < CONSUMERS; ++i) {
      pthread_join(consumer_threads[i], NULL);
    }
    printf("Final Consumer Sum is: %d.\n", consumer_sum); // No need for lock here as only thread left
    fflush(stdout);

    // Log test
    auto interval = std::chrono::system_clock::now() - start_time;
    fprintf(log_csv, "%d, %ld, %d\n", i, interval.count(), consumer_sum);
    fflush(log_csv);
  }
  fclose(log_csv);
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

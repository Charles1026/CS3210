/*******************************************************************
 * ex3456-race-condition.cpp
 * Demonstrates a race condition.
 *******************************************************************/

#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <chrono>
#include <pthread.h> // include the pthread library
#include <unistd.h>

#define ADD_THREADS 4
#define SUB_THREADS 4

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond;

int global_counter = 0;
int max_value = 14;

void *add(void *threadid)
{
	long tid = *(long *)threadid;
  pthread_mutex_lock(&lock);
	global_counter++;
  std::cout << "Incr Thread " << tid << ", counter value: " << global_counter << std::endl;
  if (global_counter >= max_value) {
    max_value = -1; // Set this to -1 to make the check in the sub threads always pass
    pthread_cond_signal(&cond);
  }
  pthread_mutex_unlock(&lock);
	sleep(rand() % 2);
	printf("add thread #%ld incremented global_counter!\n", tid);
	pthread_exit(NULL); // terminate thread
}

void *sub(void *threadid)
{
	long tid = *(long *)threadid;
  pthread_mutex_lock(&lock);
  while (global_counter < max_value) {
    pthread_cond_wait(&cond, &lock);
  }
	global_counter--;
  std::cout << "Decr Thread " << tid << ", counter value: " << global_counter << std::endl;
  pthread_mutex_unlock(&lock);
	sleep(rand() % 2);
	printf("sub thread #%ld decremented global_counter! \n", tid);
	pthread_exit(NULL); // terminate thread
}

int main(int argc, char *argv[])
{
	global_counter = 10;
	pthread_t add_threads[ADD_THREADS];
	pthread_t sub_threads[SUB_THREADS];
	long add_threadid[ADD_THREADS];
	long sub_threadid[SUB_THREADS];

  pthread_cond_init(&cond, NULL);



	int rc;
	long t1, t2;
	for (t1 = 0; t1 < ADD_THREADS; t1++)
	{
		int tid = t1;
		add_threadid[tid] = tid;
		printf("main thread: creating add thread %d\n", tid);
		rc = pthread_create(&add_threads[tid], NULL, add,
							(void *)&add_threadid[tid]);
		if (rc)
		{
			printf("Return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

	for (t2 = 0; t2 < SUB_THREADS; t2++)
	{
		int tid = t2;
		sub_threadid[tid] = tid;
		printf("main thread: creating sub thread %d\n", tid);
		rc = pthread_create(&sub_threads[tid], NULL, sub,
							(void *)&sub_threadid[tid]);
		if (rc)
		{
			printf("Return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

  int retVal = 0;
  for (int i = 0; i < ADD_THREADS; ++i) {
    pthread_join(add_threads[i], (void **) NULL);
  }

  for (int i = 0; i < SUB_THREADS; ++i) {
    pthread_join(sub_threads[i], (void **) NULL);
  }

	printf("### global_counter final value = %d ###\n", global_counter);
	pthread_exit(NULL);
}

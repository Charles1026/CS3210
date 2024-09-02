#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <chrono>

constexpr int SAMPLES = 10;
constexpr int SAMPLE_REPEAT = 10;
constexpr int START_NUM_ITER = 100000;
constexpr int SAMPLE_DISTANCE = 50000;
constexpr int END_NUM_ITER = (SAMPLES * SAMPLE_DISTANCE) + START_NUM_ITER;

int main(int argc, char *argv[]) {
  // threads
  FILE *threads_log = fopen("./threads_timings.csv", "w+");
  fprintf(threads_log, "Iterations, Duration\n");
  for (int i = START_NUM_ITER; i < END_NUM_ITER; i += SAMPLE_DISTANCE) {
    long total_time = 0;
    for (int j = 0; j < SAMPLE_REPEAT; ++j) {
      auto start_time = std::chrono::system_clock::now();
      if (fork() == 0) {
        char str_number[20];
        snprintf(str_number, sizeof(str_number), "%d", i);
        execl("./ex9-prod-con-threads", "ex9-prod-con-threads",  str_number, (char *)NULL);
      }
      wait(NULL);
      total_time += (std::chrono::system_clock::now() - start_time).count();
    }
    fprintf(threads_log, "%d, %ld\n", i, long(total_time / SAMPLE_REPEAT));
  }
  fclose(threads_log);

  // processes
  FILE *processes_log = fopen("./processes_timings.csv", "w+");
  fprintf(processes_log, "Iterations, Duration\n");
  for (int i = START_NUM_ITER; i < END_NUM_ITER; i += SAMPLE_DISTANCE) {
    auto start_time = std::chrono::system_clock::now();
    if (fork() == 0) {
      char str_number[20];
      snprintf(str_number, sizeof(str_number), "%d", i);
      execl("./ex9-prod-con-processes", "ex9-prod-con-processes",  str_number, (char *)NULL);
    }
    wait(NULL);
    auto interval = std::chrono::system_clock::now() - start_time;
    fprintf(processes_log, "%d, %ld\n", i, interval.count());
  }
  fclose(processes_log);
}
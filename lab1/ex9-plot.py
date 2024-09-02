import matplotlib.pyplot as plt
import csv

NANOSECONDS_TO_MICROSECONDS = 1 / 1000000
ITERATIONS_SCALER = 1 / 100000

if __name__ == "__main__":
  threads_timings = []
  processes_timings = []
  iterations = []
  
  with open("./threads_timings.csv", "+r") as threadsFile:
    reader = csv.reader(threadsFile)
    next(reader) # read header row
    for row in reader:
      iterations.append(int(row[0]) * ITERATIONS_SCALER)
      threads_timings.append(int(row[1]) * NANOSECONDS_TO_MICROSECONDS)
      
  with open("./processes_timings.csv", "+r") as processesFile:
    reader = csv.reader(processesFile)
    next(reader) # read header row
    for idx, row in enumerate(reader):
      assert iterations[idx] == int(row[0]) * ITERATIONS_SCALER, "Iterations should be the same in threads and processes"
      processes_timings.append(int(row[1]) * NANOSECONDS_TO_MICROSECONDS)
      
  plt.plot(iterations, threads_timings, color = "blue", label = "threads")
  plt.plot(iterations, processes_timings, color = "red", label = "processes")
  plt.legend()
  plt.title("Plot of Threads and Processes Timings(ms) against Iterations(10^6)")
  plt.savefig("./comparison.jpg")
  plt.show()
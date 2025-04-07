#include <stdio.h>

#include <atomic>
#include <thread>

#include "CycleTimer.h"
#include "mandel.h"

typedef struct {
  float x0, x1;
  float y0, y1;
  unsigned int width;
  unsigned int height;
  int maxIterations;
  int* output;
  int threadId;
  int pointsPerThread;
  int numThreads;
  float* time;
} WorkerArgs;

extern void mandelbrotSerial(float x0, float y0, float x1, float y1, int width,
                             int height, int startRow, int numRows,
                             int maxIterations, int output[]);

//
// workerThreadStart --
//
// Thread entrypoint.
void workerThreadStart(WorkerArgs* const args) {
  // TODO FOR CS149 STUDENTS: Implement the body of the worker
  // thread here. Each thread should make a call to mandelbrotSerial()
  // to compute a part of the output image.  For example, in a
  // program that uses two threads, thread 0 could compute the top
  // half of the image and thread 1 could compute the bottom half.
  double startTime = CycleTimer::currentSeconds();

  // get next task and add by 1

  const float dx = (args->x1 - args->x0) / args->width;
  const float dy = (args->y1 - args->y0) / args->height;

  // now the start and end row must change.
  // int point = args->threadId;
  int point = args->width * args->height - args->threadId + 1;
  int totalPoints = args->width * args->height;

  //  for (int s = args->threadId * args->pointsPerThread; point < totalPoints;
  //     point += args->numThreads, s++) {
  for (int s = args->threadId * args->pointsPerThread; point >= 0;
       point -= args->numThreads, s++) {
    int i = point % args->width;
    int j = point / args->width;
    const float x = args->x0 + i * dx;
    const float y = args->y0 + j * dy;
    // args->output[point] = mandel(x, y, args->maxIterations);
    args->output[s] = mandel(x, y, args->maxIterations);
  }

  double endTime = CycleTimer::currentSeconds();
  args->time[args->threadId] += endTime - startTime;
}

//
// MandelbrotThread --
//
// Multi-threaded implementation of mandelbrot set image generation.
// Threads of execution are created by spawning std::threads.
void mandelbrotThread(int numThreads, float x0, float y0, float x1, float y1,
                      int width, int height, int maxIterations, int output[],
                      float time[]) {
  // observing that the max computation time is four times as bigger as the edge
  // case computation time, when devided by the number of threads
  // so we can part the computation by height/(4*numThreads)
  // and build a pool for threads to compute

  static constexpr int MAX_THREADS = 32;

  if (numThreads > MAX_THREADS) {
    fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
    exit(1);
  }
  if (height * width % numThreads != 0) {
    fprintf(stderr, "Error: height*width must be divisible by numThreads\n");
    exit(1);
  }
  const int pointsPerThread = height * width / numThreads;

  // Creates thread objects that do not yet represent a thread.
  std::thread workers[MAX_THREADS];
  WorkerArgs args[MAX_THREADS];

  for (int i = 0; i < numThreads; i++) {
    // TODO FOR CS149 STUDENTS: You may or may not wish to modify
    // the per-thread arguments here.  The code below copies the
    // same arguments for each thread
    args[i].x0 = x0;
    args[i].y0 = y0;
    args[i].x1 = x1;
    args[i].y1 = y1;
    args[i].width = width;
    args[i].height = height;
    args[i].maxIterations = maxIterations;
    args[i].output = output;
    args[i].time = time;
    args[i].pointsPerThread = pointsPerThread;
    args[i].numThreads = numThreads;
    args[i].threadId = i;
  }

  // Spawn the worker threads.  Note that only numThreads-1 std::threads
  // are created and the main application thread is used as a worker
  // as well.
  for (int i = 1; i < numThreads; i++) {
    workers[i] = std::thread(workerThreadStart, &args[i]);
  }

  // the main thread is used as a worker as well
  workerThreadStart(&args[0]);

  // join worker threads
  for (int i = 1; i < numThreads; i++) {
    workers[i].join();
  }
}

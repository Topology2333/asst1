#include <getopt.h>
#include <math.h>
#include <stdio.h>

#include <algorithm>
#include <cstdint>

#include "CS149intrin.h"
#include "logger.h"
using namespace std;

#define EXP_MAX 10

Logger CS149Logger;

void usage(const char* progname);
void initValue(float* values, int* exponents, float* output, float* gold,
               unsigned int N);
void absSerial(float* values, float* output, int N);
void absVector(float* values, float* output, int N);
void clampedExpSerial(float* values, int* exponents, float* output, int N);
void clampedExpVector(float* values, int* exponents, float* output, int N);
float arraySumSerial(float* values, int N);
float arraySumVector(float* values, int N);
bool verifyResult(float* values, int* exponents, float* output, float* gold,
                  int N);

int main(int argc, char* argv[]) {
  int N = 16;
  bool printLog = false;

  // parse commandline options ////////////////////////////////////////////
  int opt;
  static struct option long_options[] = {{"size", 1, 0, 's'},
                                         {"log", 0, 0, 'l'},
                                         {"help", 0, 0, '?'},
                                         {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "s:l?", long_options, NULL)) != EOF) {
    switch (opt) {
      case 's':
        N = atoi(optarg);
        if (N <= 0) {
          printf("Error: Workload size is set to %d (<0).\n", N);
          return -1;
        }
        break;
      case 'l':
        printLog = true;
        break;
      case '?':
      default:
        usage(argv[0]);
        return 1;
    }
  }

  float* values = new float[N + VECTOR_WIDTH];
  int* exponents = new int[N + VECTOR_WIDTH];
  float* output = new float[N + VECTOR_WIDTH];
  float* gold = new float[N + VECTOR_WIDTH];
  initValue(values, exponents, output, gold, N);

  clampedExpSerial(values, exponents, gold, N);
  clampedExpVector(values, exponents, output, N);

  // absSerial(values, gold, N);
  // absVector(values, output, N);

  printf("\e[1;31mCLAMPED EXPONENT\e[0m (required) \n");
  bool clampedCorrect = verifyResult(values, exponents, output, gold, N);
  if (printLog) CS149Logger.printLog();
  CS149Logger.printStats();

  printf(
      "************************ Result Verification "
      "*************************\n");
  if (!clampedCorrect) {
    printf("@@@ Failed!!!\n");
  } else {
    printf("Passed!!!\n");
  }

  printf("\n\e[1;31mARRAY SUM\e[0m (bonus) \n");
  if (N % VECTOR_WIDTH == 0) {
    float sumGold = arraySumSerial(values, N);
    float sumOutput = arraySumVector(values, N);
    float epsilon = 0.1;
    bool sumCorrect = abs(sumGold - sumOutput) < epsilon * 2;
    if (!sumCorrect) {
      printf("Expected %f, got %f\n.", sumGold, sumOutput);
      printf("@@@ Failed!!!\n");
    } else {
      printf("Passed!!!\n");
    }
  } else {
    printf(
        "Must have N %% VECTOR_WIDTH == 0 for this problem (VECTOR_WIDTH is "
        "%d)\n",
        VECTOR_WIDTH);
  }

  delete[] values;
  delete[] exponents;
  delete[] output;
  delete[] gold;

  return 0;
}

void usage(const char* progname) {
  printf("Usage: %s [options]\n", progname);
  printf("Program Options:\n");
  printf("  -s  --size <N>     Use workload size N (Default = 16)\n");
  printf("  -l  --log          Print vector unit execution log\n");
  printf("  -?  --help         This message\n");
}

void initValue(float* values, int* exponents, float* output, float* gold,
               unsigned int N) {
  for (unsigned int i = 0; i < N + VECTOR_WIDTH; i++) {
    // random input values
    values[i] = -1.f + 4.f * static_cast<float>(rand()) / RAND_MAX;
    exponents[i] = rand() % EXP_MAX;
    output[i] = 0.f;
    gold[i] = 0.f;
  }
}

bool verifyResult(float* values, int* exponents, float* output, float* gold,
                  int N) {
  int incorrect = -1;
  float epsilon = 0.00001;
  for (int i = 0; i < N + VECTOR_WIDTH; i++) {
    if (abs(output[i] - gold[i]) > epsilon) {
      incorrect = i;
      break;
    }
  }

  if (incorrect != -1) {
    if (incorrect >= N) printf("You have written to out of bound value!\n");
    printf("Wrong calculation at value[%d]!\n", incorrect);
    printf("value  = ");
    for (int i = 0; i < N; i++) {
      printf("% f ", values[i]);
    }
    printf("\n");

    printf("exp    = ");
    for (int i = 0; i < N; i++) {
      printf("% 9d ", exponents[i]);
    }
    printf("\n");

    printf("output = ");
    for (int i = 0; i < N; i++) {
      printf("% f ", output[i]);
    }
    printf("\n");

    printf("gold   = ");
    for (int i = 0; i < N; i++) {
      printf("% f ", gold[i]);
    }
    printf("\n");
    return false;
  }
  printf("Results matched with answer!\n");
  return true;
}

// computes the absolute value of all elements in the input array
// values, stores result in output
void absSerial(float* values, float* output, int N) {
  for (int i = 0; i < N; i++) {
    float x = values[i];
    if (x < 0) {
      output[i] = -x;
    } else {
      output[i] = x;
    }
  }
}

// implementation of absSerial() above, but it is vectorized using CS149
// intrinsics
void absVector(float* values, float* output, int N) {
  __cs149_vec_float x;
  __cs149_vec_float result;
  __cs149_vec_float zero = _cs149_vset_float(0.f);
  __cs149_mask maskAll, maskIsNegative, maskIsNotNegative;

  //  Note: Take a careful look at this loop indexing.  This example
  //  code is not guaranteed to work when (N % VECTOR_WIDTH) != 0.
  //  Why is that the case?
  for (int i = 0; i < N; i += VECTOR_WIDTH) {
    // All ones
    maskAll = _cs149_init_ones();

    // All zeros
    maskIsNegative = _cs149_init_ones(0);

    // Load vector of values from contiguous memory addresses
    _cs149_vload_float(x, values + i, maskAll);  // x = values[i];

    // Set mask according to predicate
    _cs149_vlt_float(maskIsNegative, x, zero, maskAll);  // if (x < 0) {

    // Execute instruction using mask ("if" clause)
    _cs149_vsub_float(result, zero, x, maskIsNegative);  //   output[i] = -x;

    // Inverse maskIsNegative to generate "else" mask
    maskIsNotNegative = _cs149_mask_not(maskIsNegative);  // } else {

    // Execute instruction ("else" clause)
    _cs149_vload_float(result, values + i,
                       maskIsNotNegative);  //   output[i] = x; }

    // Write results back to memory
    _cs149_vstore_float(output + i, result, maskAll);
  }
}

// accepts an array of values and an array of exponents
//
// For each element, compute values[i]^exponents[i] and clamp value to
// 9.999.  Store result in output.
void clampedExpSerial(float* values, int* exponents, float* output, int N) {
  for (int i = 0; i < N; i++) {
    float x = values[i];
    int y = exponents[i];
    if (y == 0) {
      output[i] = 1.f;
    } else {
      float result = x;
      int count = y - 1;
      while (count > 0) {
        result *= x;
        count--;
      }
      if (result > 9.999999f) {
        result = 9.999999f;
      }
      output[i] = result;
    }
  }
}

void clampedExpVector(float* values, int* exponents, float* output, int N) {
  //
  // CS149 STUDENTS TODO: Implement your vectorized version of
  // clampedExpSerial() here.
  //
  // Your solution should work for any value of
  // N and VECTOR_WIDTH, not just when VECTOR_WIDTH divides N
  //
  // assume that N%width == 0
  int count = N / VECTOR_WIDTH;
  if (N % VECTOR_WIDTH != 0) count++;
  __cs149_mask lastMask = _cs149_init_ones(N % VECTOR_WIDTH);

  float nine = 9.999999f;

  // set mask to all ones
  __cs149_mask maskAll = _cs149_init_ones();
  __cs149_vec_int zeros;
  _cs149_vset_int(zeros, 0, maskAll);
  __cs149_vec_int ones;
  _cs149_vset_int(ones, 1, maskAll);
  __cs149_vec_float nines;
  _cs149_vset_float(nines, nine, maskAll);

  __cs149_vec_float x;
  __cs149_vec_int y;

  // set temp output
  // but the stack space is limited?
  __cs149_vec_float tempOutput = _cs149_vset_float(1.f);

  for (int i = 0; i < count; i++) {
    // init x and y
    _cs149_vset_float(x, 0.f, maskAll);
    _cs149_vset_int(y, 0, maskAll);
    _cs149_vset_float(tempOutput, 1.0f, maskAll);

    // get x and y from input
    _cs149_vload_float(x, values + VECTOR_WIDTH * i, maskAll);
    _cs149_vload_int(y, exponents + VECTOR_WIDTH * i, maskAll);

    // init not zero mask, init mask tmp;
    __cs149_mask maskIsZero = _cs149_init_ones();
    __cs149_mask maskIsNotZero = _cs149_init_ones();
    __cs149_mask maskBiggerThanNines = _cs149_init_ones();

    while (true) {
      // check if all y is 0, maskTmp now contains all 0s
      _cs149_veq_int(maskIsZero, y, zeros, maskAll);
      // check if all zero, then break
      if (_cs149_cntbits(maskIsZero) == VECTOR_WIDTH) break;
      // update maskIsNotZero
      maskIsNotZero = _cs149_mask_not(maskIsZero);
      // else, for all the y that is not zero, y = y - 1
      _cs149_vsub_int(y, y, ones, maskIsNotZero);
      // x = result * x
      _cs149_vmult_float(tempOutput, tempOutput, x, maskIsNotZero);
      // for all the tempOutput bigger than nines, set it to nines
      _cs149_vgt_float(maskBiggerThanNines, tempOutput, nines, maskAll);
      // set tempOutput to nine
      _cs149_vset_float(tempOutput, nine, maskBiggerThanNines);
    }
    // write back to the output
    // notice that the last should not be maskAll
    if (i == count - 1 && N % VECTOR_WIDTH != 0)
      _cs149_vstore_float(output + VECTOR_WIDTH * i, tempOutput, lastMask);
    else
      _cs149_vstore_float(output + VECTOR_WIDTH * i, tempOutput, maskAll);
  }
}

// returns the sum of all elements in values
float arraySumSerial(float* values, int N) {
  float sum = 0;
  for (int i = 0; i < N; i++) {
    sum += values[i];
  }

  return sum;
}

// returns the sum of all elements in values
// You can assume N is a multiple of VECTOR_WIDTH
// You can assume VECTOR_WIDTH is a power of 2
float arraySumVector(float* values, int N) {
  //
  // CS149 STUDENTS TODO: Implement your vectorized version of arraySumSerial
  // here
  //

  __cs149_vec_float tmp;
  __cs149_vec_float local_vec;

  __cs149_mask maskAll = _cs149_init_ones();

  int newSize = N / VECTOR_WIDTH;
  if (N % VECTOR_WIDTH != 0) newSize++;
  float B[newSize] = {0};

  for (int i = 0; i < newSize; i++) {
    int local = VECTOR_WIDTH / 2;
    _cs149_vset_float(local_vec, 0.f, maskAll);
    _cs149_vload_float(local_vec, values + i * VECTOR_WIDTH, maskAll);

    while (local) {
      // compute the sum from 0 to local - 1
      // add every adj elements
      _cs149_hadd_float(tmp, local_vec);
      // swap odd and even indexing
      _cs149_interleave_float(local_vec, tmp);
      local /= 2;
    }
    // load the result into B
    B[i] = local_vec.value[0];
  }

  return arraySumSerial(B, newSize);
}

#include "threadpool.hpp"

int f(int x, int y) {
  return x + y;
}

int main() {
  ThreadPool threadPool;
  threadPool.submitTask(f, 1, 2);
  
}
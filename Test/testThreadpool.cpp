#include "threadpool.hpp"
#include <iostream>
#include <unistd.h>
#include <string>

void f(int sleepTime, int iterateCount) {
  sleep(sleepTime);
  int tid = gettid();
  for (int i = 0; i < iterateCount; ++i) {
    std::string s = "[thread id " + std::to_string(tid) + "] " + "output " + std::to_string(i) + "\n";
    std::cout << s;
  }
}

int main() {
  std::cout << "thread pool created" << std::endl;
  {
    ThreadPool threadPool(5);
    threadPool.submitTask(f, 0, 3);
    threadPool.submitTask(f, 0, 3);
    threadPool.submitTask(f, 0, 3);
    threadPool.submitTask(f, 0, 3);
    threadPool.submitTask(f, 1, 3);
  }
  std::cout << "thread pool destroyed" << std::endl;
  sleep(3);
  return 0;
}
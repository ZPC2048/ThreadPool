#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <memory>
#include <future>

class ThreadPool {
public:

  explicit ThreadPool(const int threadPoolSie = 10) {
    threads.resize(threadPoolSie);
  }

  ~ThreadPool() {
    closed = true;
    conditionVariable.notify_all();
    for (auto& i : threads) {
      if (i.joinable()) {
        i.join();
      }
    }
  }

  template<typename F, typename... Args>
  auto submitTask(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
    std::function<decltype(f(args...))> func = std::bind(std::forward(f), std::forward(args), std::forward(...));
    auto funcPtr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
    auto wrappedFunction = [funcPtr]() {
      (*funcPtr)();
    };
    taskQueue.emplace(wrappedFunction);
    conditionVariable.notify_one();
    return funcPtr->get_future();
  }

protected:
  class ThreadWorker {
  public:
    ThreadWorker(int threadId, ThreadPool* pool) :
      threadId(threadId), pool(pool) {}
    
    void operator() () {
      while (!pool->closed) {
        std::unique_lock<std::mutex> threadMutex(pool->threadMutex);
        while (pool->taskQueue.empty()) {
          pool->conditionVariable.wait(threadMutex);
        }

      }
    }
    int threadId;
    ThreadPool* pool;
  };

  bool closed;
  std::mutex threadMutex;
  std::vector<std::thread> threads;
  std::queue<std::function<void()>> taskQueue;
  std::condition_variable conditionVariable;
};
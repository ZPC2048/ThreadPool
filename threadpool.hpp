#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <memory>
#include <future>

class ThreadPool {
public:

  explicit ThreadPool(const int threadPoolSie = 10) :
    threadPoolPtr(std::make_shared<ThreadPoolPtr>()) {
    threadPoolPtr->threads.resize(threadPoolSie);
    threadPoolPtr->closed = false;
    for (int i = 0; i < threadPoolSie; ++i) {
      threadPoolPtr->threads[i] = std::move(std::thread(ThreadWorker(i, threadPoolPtr)));
      threadPoolPtr->threads[i].detach();
    }
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> poolMutex(threadPoolPtr->poolMutex);
      threadPoolPtr->closed = true;
    }
    threadPoolPtr->conditionVariable.notify_all();
  }

  template<typename F, typename... Args>
  auto submitTask(F&& f, Args&&... args) -> std::future<decltype(std::forward<F>(f)(std::forward<Args>(args)...))> {
    using returnType = decltype(std::forward<F>(f)(std::forward<Args>(args)...));
    std::function<returnType()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    auto funcPtr = std::make_shared<std::packaged_task<returnType()>>(func);
    auto wrappedFunction = [funcPtr]() {
      (*funcPtr)();
    };
    {
      std::unique_lock<std::mutex> poolMutex(threadPoolPtr->poolMutex);
      threadPoolPtr->taskQueue.emplace(wrappedFunction);
    }
    threadPoolPtr->conditionVariable.notify_one();
    return funcPtr->get_future();
  }

protected:
  struct ThreadPoolPtr {
    bool closed;
    std::mutex poolMutex;
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> taskQueue;
    std::condition_variable conditionVariable;
  };

  class ThreadWorker {
  public:
    ThreadWorker(int threadId, std::shared_ptr<ThreadPoolPtr> pool) :
      threadId(threadId), pool(pool) {}
    
    void operator() () {
      std::function<void()> f;
      std::unique_lock<std::mutex> poolMutex(pool->poolMutex);
      while (true) {
        if (!pool->taskQueue.empty()) {
          f = std::move(pool->taskQueue.front());
          pool->taskQueue.pop();
          poolMutex.unlock();
          f();
          poolMutex.lock();
        } else if (pool->closed) {
          break;
        } else {
          pool->conditionVariable.wait(poolMutex);
        }
      }
    }
    int threadId;
    std::shared_ptr<ThreadPoolPtr> pool;
  };

  std::shared_ptr<ThreadPoolPtr> threadPoolPtr;
};
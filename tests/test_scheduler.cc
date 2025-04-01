#include "../East/include/Elog.h"
#include "../East/include/Scheduler.h"

East::Logger::sptr g_logger = ELOG_ROOT();

void test_fiber() {
  ELOG_INFO(g_logger) << "test fiber";
}

class simpleScheduler {
 public:
  simpleScheduler() {}

  void push_back(East::Fiber::sptr fiber) { m_fibers.push_back(fiber); }

  void run() {
    East::Fiber::sptr task{};
    auto it = m_fibers.begin();
    while (it != m_fibers.end()) {
      task = *it;
      it = m_fibers.erase(it);
      task->resume();
    }
  }

 private:
  std::list<East::Fiber::sptr> m_fibers;
};

void hello_world(int i) {
  // std::cout << "hello world " << i << std::endl;
  ELOG_INFO(g_logger) << "hello world: " << i;
}

void test_scheduler() {
  ELOG_INFO(g_logger) << "test scheduler start";
  East::Scheduler scheduler(1, true, "test_scheduler");
  scheduler.start();
  scheduler.schedule(&test_fiber);
  scheduler.stop();
  ELOG_INFO(g_logger) << "test scheduler end";
}

void test_simple_scheduler() {
  East::Fiber::GetThis();
  simpleScheduler ss{};
  for (int i = 0; i < 10; ++i) {
    East::Fiber::sptr fiber =
        std::make_shared<East::Fiber>(std::bind(hello_world, i), 0, false);
    ss.push_back(fiber);
  }
  ss.run();
}
int main() {
  ELOG_INFO(g_logger) << "main start";
  test_simple_scheduler();
  //test_scheduler();
  ELOG_INFO(g_logger) << "main end";
  return 0;
}
# East
A high performance Linux server.

## 开发环境
Linux
gcc 10.2.1
cmake

## 项目路径
bin -- 二进制文件
build -- 中间文件路径
cmake -- cmake函数文件夹
CMakeLists.txt -- cmake的定义文件
lib -- 库的输出路径
Makefile
East -- 源代码路径
     -- include 头文件
     -- src 源文件
tests -- 测试代码路径

## 日志系统
1)
Log4j
```cpp
    LoggerMgr(包含所有的Logger，默认有一个root)
        |
        |
    Logger（定义日志类别）
        |
        |---------LogFormatter(日志格式)
        |
    LogAppender（日志输出目标，std，file...）  ---- 目前有两种Appender: File appender, stdout appender. 后者主要用于测试

    LogEvent（定义日志事件的各种信息）
        |
        |
    LogEventWrap(LogEvent的包装器，在输出日志时，构造一个临时对象包含一个LogEvent(RAII), 在日志语句结束时，该对象析构时打印日志。支持流式输出)
```

Note: 每一个Logger默认有一个名称，并且支持配置，如formatter，appenders等。
Logger默认构造含有formatter和level，但是并没有默认的appender。
如果我们从LoggerMgr中获取一个不存在的Logger，会去创建一个Logger，如果使用这个Logger去输出日志，则会使用LoggerMgr中创建的root logger(默认formatter和StdoutAppender)。

## 配置系统

基于yaml实现配置系统，如日志格式，日志输出
基本原则：约定优于配置

依赖：yaml-cpp

```cpp
ConfigVarBase(抽象基类，提供配置项的接口) --> ConfigVar<T>(派生类，同时是模板类，因为配置项的属性可能不一样，整数/浮点数/字符   串...， 支持增加监听回调，在值有变化的时候会触发)
                        |
                        |
                      Config(存储所有的配置项，并且对外提供了Lookup接口，同时支持从YML中读取更新配置)


Config核心方法:
static void LoadFromYML(const YAML::Node& root);     //从yml文件中读取配置， 并且更新内存中已经定义过的配置信息，并且ConfigVar<T>对象可以设置监听，在有更新时执行

template <class T>
static typename ConfigVar<T>::sptr Lookup( const std::string& name, const T& default_val, const std::string description = "");  //在本地寻找配置项，如果没有则根据实参新创建一个保存起来

目前支持基本的stl容器：vector, list, set, unordered_set, map, unordered_map
    如果是自定义类型，需要特化LexicalCast以支持自定义类和字符串之间的相互转换。

```

1. 目前已经支持日志系统的配置解析及更新

TODO：
    m_has_formatter的语义不清晰

## 线程库
1.通过pthread封装了一个Thread类，因此我们可以给Thread添加一些额外的信息，例如，线程名称等
```cpp
相关知识点：
pthread_create(); 
pthread_join();
pthread_detach();
pthread_setname_np();

thread_loack 关键字
```

2.信号量
note:c++17已经支持shared_lock和unique_lock, 用于控制并发读写，但是我们还是自己封装一下。


信号量(Semaphore):
```cpp
sem_init();   //初始化信号量个数，POSIX禁止设置负数。 注意，如果为0的话一般用于同步协作，大于0一般用于资源管理。在Thread中的信号量就是用于控制线程初始化顺序的
sem_wait();   //如果信号量大于0，则减1后直接返回，否则阻塞当前线程，直到信号量大于0或者中断
sem_post();   //将信号量加1，并且唤醒等待线程队列
sem_destory(); //销毁信号量
```

3.互斥锁
基于pthread_rwlock_t 封装了读锁和写锁

4.自旋锁
基于pthread_spinlock_t 封装了自旋锁


日志系统整合：
在此之前，日志系统不是线程安全的，多个线程都会去修改一个文件。所以我们要利用上面的锁来控制。
四个线程
无控制：         写log大概 100MB/s
使用Mutex:       写log大概 40MB/s
使用SpinLock：   写log大概 25MB/s

理论上来说写log，冲突发生较少，所以spinlock效率较高才合理，但是四个线程下mutex的确快很多，
但是我增加到10个线程测试结果还是一样，Mutex会快很多

spinlock适用于非常短的临界区，我们测试下:
bg: 5个线程, 每个线程都对一个全局变量执行++ 1000 0000次

使用Mutex执行十次的平均耗时是：4874.5 ms
使用SpinLock执行十次的平均耗时是：9112 ms
原因就是高竞争其实Spinlock会让CPU过度忙等待，而mutex会让出CPU，在我们这种场景下线程切换其实不太频繁，所以mutex效率更高。

## 协程库

### 协程
基于ucontext_t 实现的一个协程类-Fiber
```c++
setcontext();  //跳转到某个协程上执行
getcontext();  //获取当前函数上下文
makecontext(); //将函数与某个上下文关联起来
swapcontext(old, new); //保存当前上下文到old中，执行new上下文，最后回到old
```
我们定义，每个线程有一个主协程，用来调度其他协程

```c++       
          sub_fiber
             ^
             |
             |
             v
Thread -> main_fiber  <-------->  sub_fiber

```


### 协程调度器

```c++
         1 : N       1 : M
scheduler --> Thread  --> Fiber

scheduler中含有：
1.线程池
2.协程调度器，将协程放在指定的线程上执行
```
```c++
调度器的逻辑有些复杂，用一个简单的case来模拟上流程：
业务方代码：
void test_fiber() {
  ELOG_INFO(g_logger) << "test fiber";
}

int main() {
  East::Scheduler scheduler(2, true, "test_scheduler");
  scheduler.start();
  scheduler.schedule(&test_fiber);
  scheduler.stop();
  return 0;
}

Flow:
0.-------业务方构造scheduler
1.首先创建一个调度器，第二个参数use_caller为true，所以只会再创建一个线程。
2.在该线程创建一个主协程 main fiber
3.将this设置为当前线程的调度器
4.在该调度器中创建一个调度协程 root fiber， 运行函数绑定为Scheduler::run
5.将root fiber设置为当前调度器的主协程
6.将当前线程id放到scheduler的线程id数组里
7.-------业务方调用start
8.根据线程数量创建线程，运行函数绑定为Scheduler::run
9.-------业务方调用schedule, 添加任务
10.Scheduler通过线程安全的方法（互斥量）将任务添加到任务队列中
11.因为有一个线程在跑run函数，它会去队列中找到可以执行的任务执行，执行完后检查任务状态，如果是ready的话继续放到队列中，如果是不是term或者except的话就置为hold，如果当前线程没有可执行的任务了，就执行idle协程任务，如果没有stop，就让出资源给main fiber执行，否则，idle协程置为term。
12.------业务方调用stop，停止调度器
13.idle协程检测到stop将自身状态置为term，完成线程任务退出。

```
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

#### 关于fiber的几个变量

程序中有几个static thread_local的变量，我们梳理一下他们的关系：
```cpp
static thread_local Fiber* t_fiber;              //当前正在执行的协程
static thread_local Fiber::sptr t_master_fiber;  //当前线程中的主协程，切换到这里面，就相当于切换到了主协程中运行
static thread_local Fiber* t_scheduler_fiber;    //当前线程的调度协程，每个协程都独有一个，包括caller线程


因为他们是thread_local描述的，所以每个线程中都有一个变量，其中比较好理解的是t_fiber, 它始终指向当前线程正在执行的协程，
而其他两个先看注释，下面用实际场景来解释：

1. 一个线程，不使用caller -这种情况是对应创建一个线程进行协程的调度，main函数不参与
2. 一个线程，使用caller   -这种情况是对应只使用main函数线程进行协程调度

第一种case：
main thread start
    |
create scheduler
    |
start scheduler                        ---------->    create one thread
    |                                                        |
add task                                    t_scheduler_fiber = t_fiber = t_master_fiber
    |                                                        |
stop scheduler(wait all task done)                     swap in     ---------->   swap(t_scheduler_fiber, sub fiber)
    |                                                                                       |
main thread end                                    schedule over   <----------       done, swap out

这种情况下， t_shceduler_fiber 没有实体，只是一个指针而已，它和t_master_fiber的含义是一样的，保存调度协程的上下文



第二种case：
这种case要复杂一些，因为caller线程参与调度会新建一个root fiber即t_scheduler_fiber
共有三种协程： main函数线程对应的主协程， 调度协程， 任务协程

main thread start
    |
create scheduler         --------------->  create t_fiber, and t_master_fiber = t_fiber
    |
start scheduler(do nothing)
    |
add task
    |
stop scheduler            --------------->    call root fiber : swap(t_master_fiber, t_scheduler_fiber)
    |                                                          |
wait all task done                                    find task, swap in  --------> swap(t_scheduler_fiber, sub fiber)
    |                                                                                            |
main thread end                                       scheduler  done      <---------     done, swap out
                                                              |
                                           swap(t_scheduler_fiber, t_master_fiber)

```
综上，如果在scheduler里的fiber要执行，会和调度协程交换执行权，在调度协程完成所有的调度后，和master 协程交换回到主协程的执行。如果没有调度器，那只会涉及任务协程和master协程之间的切换，这种情况比较简单。我们将代码优化一下，在Fiber中增加一个m_run_in_scheduler的bool成员，如果是在调度器中运行的协程，应该与调度协程交换，否则直接与主协程交换。
```cpp
void Fiber::resume() {
  EAST_ASSERT2(m_state != EXEC, m_state);
  SetThis(this);  
  setState(EXEC);

  if (!m_run_in_scheduler) {
    if (swapcontext(&t_master_fiber->m_ctx,
                    &m_ctx)) {  
      EAST_ASSERT2(false, "swapcontext: master fiber to cur fiber failed.");
    }
  } else {
    if (swapcontext(&Scheduler::GetMainFiber()->m_ctx,
                    &m_ctx)) {  
      EAST_ASSERT2(false, "swapcontext: scheduler fiber to cur fiber failed.");
    }
  }
}

void Fiber::yield() {
  //EAST_ASSERT2(m_state == EXEC, m_state);
  if (m_run_in_scheduler)
    SetThis(Scheduler::GetMainFiber());
  else
    SetThis(t_master_fiber.get());  //当前协程交还给主协程
  if (getState() != TERM) {
    setState(READY);
  }

  if (!m_run_in_scheduler) {
    if (swapcontext(&m_ctx,
                    &t_master_fiber->m_ctx)) {  
      EAST_ASSERT2(false, "swapcontext: cur fiber to master fiber failed.");
    }
  } else {
    if (swapcontext(
            &m_ctx,
            &Scheduler::GetMainFiber()->m_ctx)) { 
      EAST_ASSERT2(false, "swapcontext: cur fiber to scheduler fiber failed.");
    }
  }
}
```

## IO协程调度模块

epoll的两种模式：
LT模式：水平触发，内核检测到fd就绪后，若没有处理过，每一次epoll_wait都会返回该事件直到该事件被处理。兼容阻塞和非阻塞IO。
ET模式：边缘触发，内核仅在fd变化时通知一次，之后没有变化的话不再通知。要求使用非阻塞IO。
我们这里使用ET模式，更高效。
注意ET模式下， socket要使用非阻塞模式， 不然可能会阻塞当前线程。

epoll的核心函数：
```cpp
epoll_create1, 创建一个根fd-epfd，其他fd围绕这个fd进行管理

epoll_ctl, 修改epfd下某一个fd的相关属性， 例如要监听的事件类型

epoll_wait, 等待epfd上的fd有没有io事件发生，可以设置超时时间，0表示立即返回
```

IOManager类
它继承自Scheduler，用来管理IO事件。
```cpp
针对IO事件，目前支持四个操作：
int addEvent(int fd, Event event,std::function<void()> cb = nullptr);   对fd添加关心的事件
bool removeEvent(int fd, Event event);              删除fd中的某个事件
bool cancelEvent(int fd, Event event);             取消fd中的某个事件并触发回调
bool cancelAll(int fd);                         取消某个fd所有的事件并触发
```

核心函数idle：
```cpp
在空闲时，他会不停调用epoll_wait来检查有没有IO事件通知，注意这里有两种情况：
1.有新的调度任务，例如其他线程的tickle，当前协程会让出执行权
2.有我们关心的IO事件触发，当前协程也会让出执行权去执行对应的回调函数
```

### 定时器
需求：服务器要能支持定时启动某个任务，或者某个任务当前没有正在阻塞，我们可以设置一个超时时间，先把当前任务挂起去执行其他任务，超时之后再回来执行。
我们的定时任务基于epoll_wait， 在之前，我们idle协程的epoll_wait默认的超时时间是3s，如果我们有一个定时任务计划在某个时间执行，或者说我们有个任务需要周期性的执行，可以将这些任务放在一个定时器队列中(按时间从小到大排序)，在idle协程陷入epoll_wait之前，拿出队列头部最小的执行时间和当前时间做一个对比，如果时间差小于3s，就将epoll_wait的超时时间设置为这个时间，也就意味着epoll_wait返回的时候一定是有一个任务可以执行的。





## Hook模块

基于C语言全局符号唯一，我们可以提前link自己的动态库，之后链接libc的时候由于符号已经存在，我们就会调用到我们实现的函数中去了，但并不是所有的场景都需要调用我们hook的函数，所以我们利用dlsym将libc中的符号的函数地址保存到我们提前声明的函数指针中，借助这个函数指针，我们可以调用原先的库函数。

void *dlsym(void *handle, const char *symbol);
eg: dlsym(RLDT_NEXT, "sleep")
RLDT_NEXT的含义：下一个含有这个符号的动态库运行时的地址，而不是下一个加载的库,ref：[理解RLDT_NEXT](https://csstormq.github.io/blog/%E8%AE%A1%E7%AE%97%E6%9C%BA%E7%B3%BB%E7%BB%9F%E7%AF%87%E4%B9%8B%E9%93%BE%E6%8E%A5%EF%BC%8816%EF%BC%89%EF%BC%9A%E7%9C%9F%E6%AD%A3%E7%90%86%E8%A7%A3%20RTLD_NEXT%20%E7%9A%84%E4%BD%9C%E7%94%A8)

目前已经hook的函数：
sleep()
usleep()

测试中发现我们的Fiber有一点问题，TODO

和socket相关的函数普遍都与IO操作相关，我们封装了一个模板函数，他的主要流程如下：
```cpp
调用 do_io()
    ↓
执行真实系统调用 → 如果成功 → 返回
    ↓
errno = EAGAIN ?
    ↓
addEvent(fd, event)
    ↓
设置 timeout timer（可选）
    ↓
当前 fiber yield 掉
    ↓
等待 epoll 唤醒
    ↓
epoll 事件来了？
     ↙               ↘
  是，取消 timer     否，timer 超时触发
     ↓               ↓
   fiber resume      fiber resume
     ↓               ↓
   重试       errno = ETIMEDOUT, return -1

```

## Address

1.通过抽象和封装了ipv4，ipv6，unix address等相关类型，提高了API的统一性。
2.适配小端机器，网络传输默认大端。
3.几个便捷的API：
- Lookup 根据域名及服务映射出所有的IP地址
- LookupAny 找到任意一个满足条件的地址
- LookupAnyIPAddress 找到任意一个满足条件的IP地址
- GetInterfaceAddresses 获取当前机器的网卡地址
4.Address类中抽象除了获取广播地址/网络地址/子网掩码/端口的方法


几个关键系统api：
```c++
/*根据主机和服务找到符合条件的地址*/
int getaddrinfo(const char *restrict node,
                const char *restrict service,
                const struct addrinfo *restrict hints,
                struct addrinfo **restrict res);

/*函数创建一个结构体链表，描述本地系统的网络接口，并将链表第一项的地址存储在 *ifap 中。*/
int getifaddrs(struct ifaddrs **ifap); 

/*此函数将字符串 src 转换为 af 地址族中的网络地址结构，然后将该网络地址结构复制到 dst。af 参数必须是 AF_INET 或 AF_INET6。dst 按网络字节顺序写入。*/
int inet_pton(int af, const char *restrict src, void *restrict dst);
```
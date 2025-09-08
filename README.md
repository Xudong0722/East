# East - 高性能C++服务器框架

East是一个基于协程的高性能C++服务器框架，采用现代C++17标准开发，提供了完整的网络编程基础设施。

## 特性

- **协程支持**: 基于ucontext_t实现的轻量级协程系统
- **异步IO**: 基于epoll的高性能IO多路复用
- **HTTP支持**: 完整的HTTP/1.1协议实现
- **配置系统**: 基于YAML的灵活配置管理
- **日志系统**: 高性能的异步日志记录
- **跨平台**: 支持Linux系统（Windows支持计划中）

## 项目结构

```
East/
├── CMakeLists.txt              # 主构建文件
├── East/                       # 核心源码目录
│   ├── include/                # 头文件
│   ├── src/                    # 源文件
│   ├── http/                   # HTTP模块
│   └── CMakeLists.txt         # 库构建文件
├── examples/                   # 示例代码
├── tests/                      # 测试代码
└── README.md                   # 项目文档
```

## benchmark

环境：

Linux 6.6.87.2-microsoft-standard-WSL2 #1 SMP PREEMPT_DYNAMIC x86_64 GNU/Linux

工具：
sudo apt install apache2-utils -y

AB tool

### 结果

benchmark/my_http_server.cc

ab -n 1000000 -c 200 ...


```
# 单线程
Server Software:        ast/1.0.0
Server Hostname:        172.23.160.183
Server Port:            8020

Document Path:          /East
Document Length:        137 bytes

Concurrency Level:      200
Time taken for tests:   19.406 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      247000000 bytes
HTML transferred:       137000000 bytes
Requests per second:    51530.58 [#/sec] (mean)
Time per request:       3.881 [ms] (mean)
Time per request:       0.019 [ms] (mean, across all concurrent requests)
Transfer rate:          12429.74 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.4      0       4
Processing:     0    3   1.1      3      12
Waiting:        0    3   1.2      3      12
Total:          1    4   1.0      4      12

Percentage of the requests served within a certain time (ms)
  50%      4
  66%      4
  75%      4
  80%      4
  90%      5
  95%      6
  98%      7
  99%      8
 100%     12 (longest request)
```

```
# 双线程
Server Software:        ast/1.0.0
Server Hostname:        172.23.160.183
Server Port:            8020

Document Path:          /East
Document Length:        137 bytes

Concurrency Level:      200
Time taken for tests:   16.922 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      247000000 bytes
HTML transferred:       137000000 bytes
Requests per second:    59096.12 [#/sec] (mean)
Time per request:       3.384 [ms] (mean)
Time per request:       0.017 [ms] (mean, across all concurrent requests)
Transfer rate:          14254.63 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   0.5      1       6
Processing:     0    2   0.8      2      11
Waiting:        0    2   0.7      2      10
Total:          1    3   0.8      3      12

Percentage of the requests served within a certain time (ms)
  50%      3
  66%      4
  75%      4
  80%      4
  90%      4
  95%      5
  98%      5
  99%      6
 100%     12 (longest request)
```

```
# 10个线程
Server Software:        ast/1.0.0
Server Hostname:        172.23.160.183
Server Port:            8020

Document Path:          /East
Document Length:        137 bytes

Concurrency Level:      200
Time taken for tests:   25.021 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      247000000 bytes
HTML transferred:       137000000 bytes
Requests per second:    39966.75 [#/sec] (mean)
Time per request:       5.004 [ms] (mean)
Time per request:       0.025 [ms] (mean, across all concurrent requests)
Transfer rate:          9640.42 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.3      0       6
Processing:     0    5   0.9      5      27
Waiting:        0    5   0.9      5      23
Total:          1    5   0.8      5      27

Percentage of the requests served within a certain time (ms)
  50%      5
  66%      5
  75%      5
  80%      6
  90%      6
  95%      6
  98%      7
  99%      7
 100%     27 (longest request)
```

```
# 20个线程
Server Software:        ast/1.0.0
Server Hostname:        172.23.160.183
Server Port:            8020

Document Path:          /East
Document Length:        137 bytes

Concurrency Level:      200
Time taken for tests:   28.449 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      247000000 bytes
HTML transferred:       137000000 bytes
Requests per second:    35151.12 [#/sec] (mean)
Time per request:       5.690 [ms] (mean)
Time per request:       0.028 [ms] (mean, across all concurrent requests)
Transfer rate:          8478.84 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.2      0       4
Processing:     1    5   1.1      5      52
Waiting:        0    5   1.1      5      52
Total:          1    6   1.0      6      55

Percentage of the requests served within a certain time (ms)
  50%      6
  66%      6
  75%      6
  80%      6
  90%      7
  95%      7
  98%      7
  99%      8
 100%     55 (longest request)
```

作为对比，我们看一下nginx相同参数下的性能情况：

nginx 默认使用系统的核心数
```
Server Software:        nginx/1.18.0
Server Hostname:        172.23.160.183
Server Port:            80

Document Path:          /East
Document Length:        162 bytes

Concurrency Level:      200
Time taken for tests:   26.203 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Total transferred:      321000000 bytes
HTML transferred:       162000000 bytes
Requests per second:    38163.85 [#/sec] (mean)
Time per request:       5.241 [ms] (mean)
Time per request:       0.026 [ms] (mean, across all concurrent requests)
Transfer rate:          11963.47 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    2   0.6      2       8
Processing:     1    3   0.7      3       9
Waiting:        0    2   0.7      2       9
Total:          2    5   0.7      5      13

Percentage of the requests served within a certain time (ms)
  50%      5
  66%      5
  75%      6
  80%      6
  90%      6
  95%      7
  98%      7
  99%      7
 100%     13 (longest request)
```

再来看一下长连接情况下的性能对比:

```
# East 单线程 长连接
Document Path:          /East
Document Length:        137 bytes

Concurrency Level:      200
Time taken for tests:   34.193 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Keep-Alive requests:    1000000
Total transferred:      252000000 bytes
HTML transferred:       137000000 bytes
Requests per second:    29246.08 [#/sec] (mean)
Time per request:       6.839 [ms] (mean)
Time per request:       0.034 [ms] (mean, across all concurrent requests)
Transfer rate:          7197.28 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.0      0       3
Processing:     0    7   1.0      7      14
Waiting:        0    7   1.0      7      14
Total:          0    7   1.0      7      14

Percentage of the requests served within a certain time (ms)
  50%      7
  66%      7
  75%      7
  80%      7
  90%      8
  95%      8
  98%      9
  99%      9
 100%     14 (longest request)
```

```
# East 10线程 长连接
Concurrency Level:      200
Time taken for tests:   12.498 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Keep-Alive requests:    1000000
Total transferred:      252000000 bytes
HTML transferred:       137000000 bytes
Requests per second:    80011.11 [#/sec] (mean)
Time per request:       2.500 [ms] (mean)
Time per request:       0.012 [ms] (mean, across all concurrent requests)
Transfer rate:          19690.23 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.0      0       2
Processing:     0    2   0.9      3       7
Waiting:        0    2   0.9      3       7
Total:          0    2   0.9      3       7

Percentage of the requests served within a certain time (ms)
  50%      3
  66%      3
  75%      3
  80%      3
  90%      3
  95%      3
  98%      3
  99%      3
 100%      7 (longest request)
```

```
#East 20线程 长连接
Document Path:          /East
Document Length:        137 bytes

Concurrency Level:      200
Time taken for tests:   13.104 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Keep-Alive requests:    1000000
Total transferred:      252000000 bytes
HTML transferred:       137000000 bytes
Requests per second:    76314.36 [#/sec] (mean)
Time per request:       2.621 [ms] (mean)
Time per request:       0.013 [ms] (mean, across all concurrent requests)
Transfer rate:          18780.49 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.0      0       2
Processing:     0    3   1.5      3      72
Waiting:        0    3   1.5      3      72
Total:          0    3   1.5      3      72

Percentage of the requests served within a certain time (ms)
  50%      3
  66%      3
  75%      3
  80%      3
  90%      4
  95%      4
  98%      4
  99%      4
 100%     72 (longest request)
```

```
# Nginx 长连接
Server Software:        nginx/1.18.0
Server Hostname:        172.23.160.183
Server Port:            80

Document Path:          /East
Document Length:        162 bytes

Concurrency Level:      200
Time taken for tests:   3.882 seconds
Complete requests:      1000000
Failed requests:        0
Non-2xx responses:      1000000
Keep-Alive requests:    990099
Total transferred:      325950495 bytes
HTML transferred:       162000000 bytes
Requests per second:    257632.09 [#/sec] (mean)
Time per request:       0.776 [ms] (mean)
Time per request:       0.004 [ms] (mean, across all concurrent requests)
Transfer rate:          82007.14 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.1      0       4
Processing:     0    1   0.3      1       4
Waiting:        0    1   0.3      1       4
Total:          0    1   0.3      1       5

Percentage of the requests served within a certain time (ms)
  50%      1
  66%      1
  75%      1
  80%      1
  90%      1
  95%      1
  98%      2
  99%      2
 100%      5 (longest request)
```

## 核心模块

### 1. 协程系统 (Fiber)

基于ucontext_t实现的轻量级协程，支持协程间切换和调度。

**核心组件:**
- `Fiber`: 协程基类，管理协程上下文和状态
- `Scheduler`: 协程调度器，支持多线程协程调度
- `IOManager`: IO协程调度器，集成epoll事件处理

**关键特性:**
- 支持1:N线程模型（一个调度器管理多个线程）
- 支持1:M线程模型（一个线程管理多个协程）
- 智能的任务调度和负载均衡

### 2. IO管理 (IOManager)

基于epoll的高性能IO多路复用系统，支持边缘触发(ET)模式。

**核心功能:**
- 文件描述符事件管理（读/写事件）
- 异步IO操作和回调处理
- 定时器集成
- 线程间通信（pipe唤醒机制）

**事件类型:**
```cpp
enum Event {
    NONE = 0x0,
    READ = 0x1,    // EPOLLIN
    WRITE = 0x4,   // EPOLLOUT
};
```

### 3. 网络模块

#### Socket抽象
- 支持TCP/UDP协议
- 非阻塞IO操作
- 地址族抽象（IPv4/IPv6/Unix Domain）

#### Address类
- 统一的地址抽象接口
- 支持域名解析和网络接口查询
- 字节序转换（小端/大端）

#### TcpServer
- 高性能TCP服务器实现
- 支持多地址绑定
- 协程化的连接处理
- 可配置的读取超时

### 4. HTTP模块

完整的HTTP/1.1协议实现，包括：

**HTTP解析器:**
- 基于Ragel的状态机解析器
- 支持HTTP请求和响应解析
- 高效的流式解析

**核心组件:**
- `HttpRequest`: HTTP请求封装
- `HttpResponse`: HTTP响应封装
- `HttpConnection`: 连接管理
- `HttpServer`: HTTP服务器
- `Servlet`: 请求处理器接口

**支持特性:**
- 完整的HTTP方法支持（GET, POST, PUT, DELETE等）
- HTTP状态码和头部管理
- 可扩展的Servlet架构

### 5. 配置系统 (Config)

基于YAML的灵活配置管理系统。

**核心功能:**
- 类型安全的配置变量
- 配置热更新支持
- 配置变更监听器
- 支持STL容器类型

**配置示例:**
```yaml
tcp_server:
  read_timeout: 120000  # 120秒
  name: "East/1.0.0"

logging:
  level: INFO
  formatter: "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
```

### 6. 日志系统 (Logger)

高性能的异步日志记录系统。

**架构设计:**
```
LoggerMgr (全局管理器)
    |
    |-- Logger (日志类别)
        |
        |-- LogFormatter (格式器)
        |-- LogAppender (输出目标)
            |-- StdoutAppender (控制台输出)
            |-- FileAppender (文件输出)
```

**特性:**
- 异步日志记录，不阻塞业务逻辑
- 可配置的日志格式和级别
- 支持多种输出目标
- 线程安全的日志操作

### 7. 工具模块

#### ByteArray
- 高效的字节流处理
- 支持不同字节序的整数读写
- 使用ZigZag算法压缩整数
- 链表式存储，自动扩容

#### URI解析
- 基于Ragel的URI解析器
- 支持标准URI格式
- 高效的解析性能

#### Hook系统
- 系统调用拦截和重定向
- 支持sleep、socket等函数hook
- 协程友好的异步操作

### 8. 线程和同步

#### Thread类
- pthread封装，支持线程命名
- 线程生命周期管理

#### 锁机制
- `Mutex`: 读写锁（基于pthread_rwlock_t）
- `SpinLock`: 自旋锁（基于pthread_spinlock_t）
- `Semaphore`: 信号量

## 开发环境

- **操作系统**: Linux (推荐Ubuntu 20.04+)
- **编译器**: GCC 10.2.1+
- **构建工具**: CMake 3.22+
- **C++标准**: C++17
- **依赖库**: 
  - yaml-cpp (自动下载)
  - pthread
  - dl

## 快速开始

### 1. 克隆项目
```bash
git clone <repository-url>
cd East
```

### 2. 构建项目
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 3. 运行示例
```bash
# 运行echo服务器
./bin/echo_server

# 运行测试
./bin/test_tcp_server
./bin/test_http_server
```

## 使用示例

### TCP服务器示例
```cpp
#include "TcpServer.h"
#include "IOManager.h"

class MyTcpServer : public East::TcpServer {
protected:
    void handleClient(East::Socket::sptr sock) override {
        // 处理客户端连接
        char buffer[1024];
        int len = sock->recv(buffer, sizeof(buffer));
        if (len > 0) {
            sock->send(buffer, len);
        }
    }
};

int main() {
    East::IOManager::sptr iom = std::make_shared<East::IOManager>(4);
    MyTcpServer::sptr server = std::make_shared<MyTcpServer>(iom.get(), iom.get());
    
    East::Address::sptr addr = East::Address::LookupAny("0.0.0.0:8080");
    server->bind(addr);
    server->start();
    
    iom->run();
    return 0;
}
```

### HTTP服务器示例
```cpp
#include "HttpServer.h"
#include "Http.h"

class MyServlet : public East::Http::Servlet {
public:
    int32_t handle(East::Http::HttpRequest::sptr req,
                   East::Http::HttpResponse::sptr rsp,
                   East::Http::HttpSession::sptr session) override {
        rsp->setBody("Hello, East!");
        return 0;
    }
};

int main() {
    East::Http::HttpServer::sptr server = std::make_shared<East::Http::HttpServer>();
    server->addServlet("/hello", std::make_shared<MyServlet>());
    
    East::Address::sptr addr = East::Address::LookupAny("0.0.0.0:8080");
    server->bind(addr);
    server->start();
    
    return 0;
}
```

## 测试

项目包含完整的测试套件，覆盖所有核心模块：

```bash
# 运行所有测试
make test

# 运行特定测试
./bin/test_fiber      # 协程测试
./bin/test_scheduler  # 调度器测试
./bin/test_http       # HTTP模块测试
./bin/test_socket     # Socket测试
```

##  贡献

欢迎提交Issue和Pull Request！在贡献代码前，请确保：

1. 代码符合项目的编码规范
2. 添加适当的测试用例
3. 更新相关文档
4. 遵循Google C++代码风格

## 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 相关链接

- [日志模块详细说明](https://xudong0722.github.io/2025/05/22/East-Log-Module/)
- [项目博客](https://xudong0722.github.io/)

---

**East** - 构建高性能服务器的现代C++框架
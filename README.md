# East - 高性能C++服务器框架

East是一个基于协程的高性能C++服务器框架，采用现代C++17标准开发，提供了完整的网络编程基础设施。

## 🚀 特性

- **协程支持**: 基于ucontext_t实现的轻量级协程系统
- **异步IO**: 基于epoll的高性能IO多路复用
- **HTTP支持**: 完整的HTTP/1.1协议实现
- **配置系统**: 基于YAML的灵活配置管理
- **日志系统**: 高性能的异步日志记录
- **跨平台**: 支持Linux系统（Windows支持计划中）

## 🏗️ 项目结构

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

## 📦 核心模块

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

## 🛠️ 开发环境

- **操作系统**: Linux (推荐Ubuntu 20.04+)
- **编译器**: GCC 10.2.1+
- **构建工具**: CMake 3.22+
- **C++标准**: C++17
- **依赖库**: 
  - yaml-cpp (自动下载)
  - pthread
  - dl

## 🚀 快速开始

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

## 📚 使用示例

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

## 🧪 测试

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

## 📖 API文档

详细的API文档请参考各模块的头文件，所有公共接口都包含完整的Google风格注释。

## 🤝 贡献

欢迎提交Issue和Pull Request！在贡献代码前，请确保：

1. 代码符合项目的编码规范
2. 添加适当的测试用例
3. 更新相关文档
4. 遵循Google C++代码风格

## 📄 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 🔗 相关链接

- [日志模块详细说明](https://xudong0722.github.io/2025/05/22/East-Log-Module/)
- [项目博客](https://xudong0722.github.io/)

---

**East** - 构建高性能服务器的现代C++框架
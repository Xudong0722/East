/*
 * @Author: Xudong0722 
 * @Date: 2025-05-29 21:58:07 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-11 22:23:18
 */

#include "TcpServer.h"
#include "Config.h"
#include "Elog.h"

namespace East {

/**
 * @brief TCP服务器读取超时配置变量
 * 
 * 默认值为120秒（60 * 1000 * 2毫秒），可通过配置文件修改
 */
static East::ConfigVar<uint64_t>::sptr g_tcp_server_read_timeout =
    East::Config::Lookup("tcp_server.read_timeout", uint64_t(60 * 1000 * 2),
                         "tcp server read timeout");

/**
 * @brief 系统日志记录器
 */
static East::Logger::sptr g_logger = ELOG_NAME("system");

/**
 * @brief 构造函数实现
 * 
 * 初始化TCP服务器的各个成员变量：
 * - 设置工作协程管理器和接受连接协程管理器
 * - 从配置读取默认读取超时时间
 * - 设置默认服务器名称
 * - 初始化停止标志为true
 */
TcpServer::TcpServer(East::IOManager* worker, East::IOManager* accept_worker)
    : m_worker(worker),
      m_acceptWorker(accept_worker),
      m_readTimeout(g_tcp_server_read_timeout->getValue()),
      m_name("East/1.0.0"),
      m_isStop(true) {}

/**
 * @brief 析构函数实现
 * 
 * 清理资源：
 * - 关闭所有监听socket连接
 * - 清空socket列表
 */
TcpServer::~TcpServer() {
  for (auto& i : m_socks) {
    i->close();
  }
  m_socks.clear();
}

/**
 * @brief 绑定单个地址到服务器
 * 
 * 将单个地址绑定转换为多地址绑定调用，简化接口使用
 * 
 * @param addr 要绑定的地址
 * @return 绑定成功返回true，失败返回false
 */
bool TcpServer::bind(East::Address::sptr addr) {
  std::vector<Address::sptr> addrs{addr};
  std::vector<Address::sptr> fails;
  return bind(addrs, fails);
}

/**
 * @brief 绑定多个地址到服务器
 * 
 * 核心绑定逻辑实现：
 * 1. 遍历所有地址，为每个地址创建TCP socket
 * 2. 尝试绑定地址，失败则记录到fails列表
 * 3. 尝试监听连接，失败则记录到fails列表
 * 4. 如果所有地址都绑定成功，则保存socket到m_socks
 * 5. 如果有任何地址绑定失败，则清空所有socket并返回false
 * 
 * @param addrs 要绑定的地址列表
 * @param fails 绑定失败的地址列表（输出参数）
 * @return 所有地址绑定成功返回true，有失败返回false
 */
bool TcpServer::bind(const std::vector<Address::sptr>& addrs,
                     std::vector<Address::sptr>& fails) {
  for (auto& addr : addrs) {
    Socket::sptr sock = Socket::CreateTCP(addr);
    if (!sock->bind(addr)) {
      ELOG_ERROR(g_logger) << "bind fail errno: " << errno
                           << " strerror: " << strerror(errno) << " addr:[ "
                           << addr->toString() << "]";
      fails.emplace_back(addr);
      continue;
    }
    if (!sock->listen()) {
      ELOG_ERROR(g_logger) << "listen fail errno: " << errno
                           << " strerror: " << strerror(errno) << " addr:[ "
                           << addr->toString() << "]";
      fails.emplace_back(addr);
      continue;
    }
    m_socks.emplace_back(sock);
  }

  if (!fails.empty()) {
    m_socks.clear();
    return false;
  }

  for (auto& i : m_socks) {
    ELOG_INFO(g_logger) << "Server bind success: " << *i;
  }
  return true;
}

/**
 * @brief 启动服务器
 * 
 * 启动逻辑：
 * 1. 检查服务器是否已经启动，如果已启动则直接返回
 * 2. 设置停止标志为false
 * 3. 为每个监听socket启动一个接受连接的协程任务
 * 4. 使用m_acceptWorker调度协程，避免阻塞主线程
 * 
 * @return 启动成功返回true
 */
bool TcpServer::start() {
  if (!m_isStop) {
    return true;
  }
  m_isStop = false;
  for (auto& sock : m_socks) {
    m_acceptWorker->schedule(
        std::bind(&TcpServer::startAccept, shared_from_this(), sock));
  }
  return true;
}

/**
 * @brief 停止服务器
 * 
 * 停止逻辑：
 * 1. 设置停止标志为true，通知所有接受连接协程退出循环
 * 2. 在accept_worker中异步执行清理操作
 * 3. 取消所有socket的异步操作
 * 4. 关闭所有socket连接
 * 5. 清空socket列表
 * 
 * 注意：使用shared_from_this()增加引用计数，确保协程执行期间对象不被销毁
 */
void TcpServer::stop() {
  m_isStop = true;
  auto self = shared_from_this();  //增加一个引用计数
  m_acceptWorker->schedule([this, self]() {
    for (auto& sock : m_socks) {
      sock->cancelAll();
      sock->close();
    }
    m_socks.clear();
  });
}

/**
 * @brief 处理客户端连接
 * 
 * 默认实现只是记录日志，子类应该重写此函数来实现具体的业务逻辑
 * 
 * @param sock 客户端socket连接
 */
void TcpServer::handleClient(Socket::sptr sock) {
  ELOG_INFO(g_logger) << "handleClient: " << *sock;
}

/**
 * @brief 开始接受连接
 * 
 * 核心接受连接逻辑：
 * 1. 在循环中持续接受新连接，直到服务器停止
 * 2. 为每个接受的客户端连接设置读取超时时间
 * 3. 将客户端处理任务调度到工作协程管理器
 * 4. 使用shared_from_this()确保协程执行期间对象不被销毁
 * 5. 如果接受连接失败，记录错误日志但继续循环
 * 
 * @param sock 监听socket
 */
void TcpServer::startAccept(Socket::sptr sock) {
  while (!m_isStop) {
    Socket::sptr client = sock->accept();
    if (client) {
      client->setRecvTimeout(m_readTimeout);
      m_worker->schedule(
          std::bind(&TcpServer::handleClient, shared_from_this(), client));
    } else {
      ELOG_DEBUG(g_logger) << "Accept error: " << errno
                           << " strerrno: " << strerror(errno);
    }
  }
}
}  //namespace East
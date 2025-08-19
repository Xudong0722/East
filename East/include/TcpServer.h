/*
 * @Author: Xudong0722 
 * @Date: 2025-05-29 20:55:45 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-11 22:25:12
 */

#pragma once

#include <functional>
#include <memory>
#include "IOManager.h"
#include "Noncopyable.h"
#include "Socket.h"

namespace East {

/**
 * @brief TCP服务器类，提供TCP连接监听和客户端处理功能
 * 
 * TcpServer是一个基于协程的TCP服务器实现，支持多地址绑定、
 * 异步接受连接和客户端连接处理。该类继承自std::enable_shared_from_this
 * 以支持协程调度，同时继承noncopymoveable防止拷贝和移动。
 */
class TcpServer : public std::enable_shared_from_this<TcpServer>,
                  protected noncopymoveable {
 public:
  using sptr = std::shared_ptr<TcpServer>;
  
  /**
   * @brief 构造函数
   * @param worker 工作协程管理器，用于处理客户端连接
   * @param accept_worker 接受连接协程管理器，用于监听新连接
   */
  TcpServer(East::IOManager* worker = East::IOManager::GetThis(),
            East::IOManager* accept_worker = East::IOManager::GetThis());
  
  /**
   * @brief 析构函数，关闭所有socket连接
   */
  virtual ~TcpServer();

  /**
   * @brief 绑定单个地址到服务器
   * @param addr 要绑定的地址
   * @return 绑定成功返回true，失败返回false
   */
  virtual bool bind(East::Address::sptr addr);
  
  /**
   * @brief 绑定多个地址到服务器
   * @param addrs 要绑定的地址列表
   * @param fails 绑定失败的地址列表（输出参数）
   * @return 所有地址绑定成功返回true，有失败返回false
   */
  virtual bool bind(const std::vector<Address::sptr>& addrs,
                    std::vector<Address::sptr>& fails);
  
  /**
   * @brief 启动服务器，开始接受客户端连接
   * @return 启动成功返回true
   */
  virtual bool start();
  
  /**
   * @brief 停止服务器，关闭所有连接
   */
  virtual void stop();

  /**
   * @brief 获取读取超时时间
   * @return 读取超时时间（毫秒）
   */
  uint64_t getReadTimeout() const { return m_readTimeout; }
  
  /**
   * @brief 获取服务器名称
   * @return 服务器名称字符串
   */
  std::string getName() const { return m_name; }
  
  /**
   * @brief 设置读取超时时间
   * @param v 超时时间（毫秒）
   */
  void setReadTimeout(uint64_t v) { m_readTimeout = v; }
  
  /**
   * @brief 设置服务器名称
   * @param name 服务器名称
   */
  void setName(const std::string& name) { m_name = name; }

  /**
   * @brief 检查服务器是否已停止
   * @return 服务器已停止返回true，运行中返回false
   */
  bool isStop() const { return m_isStop; }

 protected:
  /**
   * @brief 处理客户端连接的虚函数，子类可重写此函数
   * @param sock 客户端socket连接
   */
  virtual void handleClient(Socket::sptr sock);
  
  /**
   * @brief 开始接受连接的虚函数，子类可重写此函数
   * @param sock 监听socket
   */
  virtual void startAccept(Socket::sptr sock);

 private:
  IOManager* m_worker{nullptr};        ///< 工作协程管理器，处理客户端连接
  IOManager* m_acceptWorker{nullptr};  ///< 接受连接协程管理器，监听新连接
  std::vector<Socket::sptr> m_socks;   ///< 监听socket列表
  uint64_t m_readTimeout{0};           ///< 读取超时时间，防止资源浪费

  std::string m_name;                  ///< 服务器名称
  bool m_isStop{false};                ///< 服务器停止标志
};
}  // namespace East

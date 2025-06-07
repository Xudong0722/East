/*
 * @Author: Xudong0722 
 * @Date: 2025-05-29 20:55:45 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-30 00:31:37
 */

#pragma once

#include <functional>
#include <memory>
#include "IOManager.h"
#include "Noncopyable.h"
#include "Socket.h"

namespace East {
class TcpServer : public std::enable_shared_from_this<TcpServer>,
                  protected noncopymoveable {
 public:
  using sptr = std::shared_ptr<TcpServer>;
  TcpServer(East::IOManager* worker = East::IOManager::GetThis(),
            East::IOManager* accept_worker = East::IOManager::GetThis());
  virtual ~TcpServer();

  virtual bool bind(East::Address::sptr addr);
  virtual bool bind(const std::vector<Address::sptr>& addrs,
                    std::vector<Address::sptr>& fails);
  virtual bool start();
  virtual void stop();

  uint64_t getReadTimeout() const { return m_readTimeout; }
  std::string getName() const { return m_name; }
  void setReadTimeout(uint64_t v) { m_readTimeout = v; }
  void setName(const std::string& name) { m_name = name; }

  bool isStop() const { return m_isStop; }

 protected:
  virtual void handleClient(Socket::sptr sock);
  virtual void startAccept(Socket::sptr sock);

 private:
  IOManager* m_worker{nullptr};
  IOManager* m_acceptWorker{nullptr};
  std::vector<Socket::sptr> m_socks;
  uint64_t m_readTimeout{0};  //防止资源浪费

  std::string m_name;
  bool m_isStop{false};
};
}  // namespace East

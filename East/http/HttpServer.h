/*
 * @Author: Xudong0722 
 * @Date: 2025-06-11 22:19:47 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-11 22:28:44
 */

#pragma once

#include "../include/TcpServer.h"
#include "HttpSession.h"

namespace East {
namespace Http {

class HttpServer : public TcpServer {
public:
  using sptr = std::shared_ptr<HttpServer>;
  HttpServer(bool keep_alive = false, East::IOManager* worker = East::IOManager::GetThis(),
            East::IOManager* accept_worker = East::IOManager::GetThis());
            
protected:
  virtual void handleClient(Socket::sptr client) override;
private:
  bool m_isKeepAlive{false};
};
} // namespace Http
} // namespace East

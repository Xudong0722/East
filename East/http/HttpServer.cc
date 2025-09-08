/*
 * @Author: Xudong0722 
 * @Date: 2025-06-11 22:25:42 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-11 22:33:55
 */

#include "HttpServer.h"
#include "../include/Elog.h"

namespace East {
namespace Http {

static Logger::sptr g_logger = ELOG_NAME("system");

HttpServer::HttpServer(bool keep_alive, East::IOManager* worker,
                       East::IOManager* accept_worker)
    : TcpServer(worker, accept_worker),
      m_isKeepAlive(keep_alive),
      m_dispatch(std::make_shared<ServletDispatch>()) {}

void HttpServer::handleClient(Socket::sptr client) {
  HttpSession::sptr session = std::make_shared<HttpSession>(client);
  do {
    auto req = session->recvRequest();
    if (nullptr == req) {
      ELOG_DEBUG(g_logger) << "recv http request fail, errno: " << errno
                          << ", strerrno: " << strerror(errno)
                          << ", client: " << *client;
      break;
    }

    HttpResp::sptr rsp = std::make_shared<HttpResp>(
        req->getVersion(), req->isClose() || !m_isKeepAlive);
    m_dispatch->handle(req, rsp, session);
    // rsp->setBody("hello world");
    session->sendResponse(rsp);
    if(!m_isKeepAlive || req->isClose()) break;
  } while (true);
  session->close();
}

}  //namespace Http
}  //namespace East
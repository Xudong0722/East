/*
 * @Author: Xudong0722 
 * @Date: 2025-06-12 21:57:48 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-12 23:32:31
 */

#include "Servlet.h"

namespace East {
namespace Http {

FunctionServlet::FunctionServlet(callback cb)
 : Servlet("FunctionServlet"), m_cb(cb) {

}

int32_t FunctionServlet::handle(HttpReq::sptr req, HttpResp::sptr rsp, HttpSession::sptr session) {
  return m_cb(req, rsp, session);
}

ServletDispatch::ServletDispatch() 
  : Servlet("ServletDispatch") {
  setDefault(std::make_shared<NotFoundServlet>());
}

int32_t ServletDispatch::handle(HttpReq::sptr req, HttpResp::sptr rsp, HttpSession::sptr session) {
  auto slt = getMatchedServlet(req->getPath());
  if(nullptr != slt) {
    slt->handle(req, rsp, session);
  }
  return 0;
}

void ServletDispatch::addServlet(const std::string& uri, Servlet::sptr slt) {
  MutexType::WLockGuard wlock(m_mutex);
  m_datas[uri] = slt;
}

void ServletDispatch::addServlet(const std::string& uri, FunctionServlet::callback cb) {
  MutexType::WLockGuard wlock(m_mutex);
  Servlet::sptr function_servlet = std::make_shared<FunctionServlet>(cb);
  m_datas[uri] = function_servlet;
}

void ServletDispatch::addGlobServlet(const std::string& uri, Servlet::sptr slt) {
  MutexType::WLockGuard wlock(m_mutex);
  auto it = std::find_if(m_globs.begin(), m_globs.end(), [&uri](const auto& item){
    return item.first == uri;
  });
  if(it != m_globs.end()) m_globs.erase(it);
  m_globs.emplace_back(std::pair{uri, slt});
}

void ServletDispatch::addGlobServlet(const std::string& uri, FunctionServlet::callback cb) {
  return addGlobServlet(uri, std::make_shared<FunctionServlet>(cb));
}

void ServletDispatch::delServlet(const std::string& uri) {
  MutexType::WLockGuard wlock(m_mutex);
  m_datas.erase(uri);
}

void ServletDispatch::delGlogServlet(const std::string& uri) {
  MutexType::WLockGuard wlock(m_mutex);
  auto it = std::find_if(m_globs.begin(), m_globs.end(), [&uri](const auto& item){
    return item.first == uri;
  });
  if(it != m_globs.end()) m_globs.erase(it);
}

Servlet::sptr ServletDispatch::getServlet(const std::string& uri) {
  MutexType::RLockGuard rlock(m_mutex);
  if(m_datas.count(uri)){
    return m_datas.at(uri);
  }
  return nullptr;
}

Servlet::sptr ServletDispatch::getGlobServlet(const std::string& uri) {
  MutexType::RLockGuard rlock(m_mutex);
  auto it = std::find_if(m_globs.begin(), m_globs.end(), [&uri](const auto& item){
    return item.first == uri;
  });
  if(it == m_globs.end()) return nullptr;
  return it->second;
}

Servlet::sptr ServletDispatch::getMatchedServlet(const std::string& uri) {
  MutexType::RLockGuard rlock(m_mutex);
  auto slt = getServlet(uri);
  if(nullptr != slt) return slt; 
  slt = getGlobServlet(uri);
  if(nullptr != slt) return slt;
  return m_default;
}

NotFoundServlet::NotFoundServlet() 
  : Servlet("NotFoundServlet") {

}

int32_t NotFoundServlet::handle(HttpReq::sptr req, HttpResp::sptr rsp, HttpSession::sptr session) {
  static const std::string RSP_BODY = "<html><head><title>404 Not Found"
    "</title></head><body><center><h1>404 Not Found</h1></center>"
    "<hr><center>East/1.0.0</center></body></html>";

    rsp->setStatus(HttpStatus::NOT_FOUND);
    rsp->setHeader("Content-Type", "text/html");
    rsp->setHeader("Server", "East/1.0.0");
    rsp->setBody(RSP_BODY);
    return 0;
}

}// namespace Http
} // namespace East
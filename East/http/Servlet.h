/*
 * @Author: Xudong0722 
 * @Date: 2025-06-12 21:39:10 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-12 21:56:09
 */

#pragma once
#include "Http.h"
#include "HttpSession.h"
#include "../include/Mutex.h"
#include <memory>
#include <functional>
#include <string>
#include <unordered_map>

namespace East {
namespace Http {
    
class Servlet {
public:
  using sptr = std::shared_ptr<Servlet>;
  Servlet(const std::string& name) : m_name(name) {}
  virtual~Servlet() {}

  virtual int32_t handle(HttpReq::sptr req, HttpResp::sptr rsp, HttpSession::sptr session) = 0;
  const std::string& getName() const { return m_name; }

protected:
  std::string m_name;
};

class FunctionServlet : public Servlet {
public:
  using sptr = std::shared_ptr<FunctionServlet>;
  using callback = std::function<int32_t(HttpReq::sptr, HttpResp::sptr, HttpSession::sptr)>;

  FunctionServlet(callback cb);
  virtual int32_t handle(HttpReq::sptr req, HttpResp::sptr rsp, HttpSession::sptr session) override;

private:
  callback m_cb;
};

class ServletDispatch : public Servlet {
public:
  using sptr = std::shared_ptr<ServletDispatch>;
  using MutexType = RWLock;
  
  virtual int32_t handle(HttpReq::sptr req, HttpResp::sptr rsp, HttpSession::sptr session) override;
  
  void addServlet(const std::string& uri, Servlet::sptr slt);
  void addServlet(const std::string& uri, FunctionServlet::callback cb);
  void addGlobServlet(const std::string& uri, Servlet::sptr slt);
  void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

  void delServlet(const std::string& uri);
  void delGlogServlet(const std::string& uri);

  Servlet::sptr getDefault() const { return m_default; }
  void setDefault(Servlet::sptr dv) { m_default = dv; }

  Servlet::sptr getServlet(const std::string& uri);
  Servlet::sptr getGlobServlet(const std::string& uri);

  Servlet::sptr getMatchedServlet(const std::string& uri);
private:
  //uri(/east/xxx) -> Servlet   精准匹配
  std::unordered_map<std::string, Servlet::sptr> m_datas;
  //uri(/east/*) -> Servlets    模糊匹配
  std::vector<std::pair<std::string, Servlet::sptr>> m_globs;
  //默认Servlet，所有路径都没有匹配到的时候使用
  Servlet::sptr m_default;

  MutexType m_mutex;
};

} // namespace Htpp
} // namespace East

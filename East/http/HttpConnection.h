/*
 * @Author: Xudong0722 
 * @Date: 2025-06-18 23:25:58 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-22 13:52:59
 */

#pragma once
#include <atomic>
#include <list>
#include "../include/Mutex.h"
#include "../include/SocketStream.h"
#include "../include/uri.h"
#include "Http.h"

namespace East {
namespace Http {

struct HttpResult {
  using sptr = std::shared_ptr<HttpResult>;
  HttpResult(int _res, HttpResp::sptr _resp, const std::string& _error = "")
      : result(_res), resp(_resp), error(_error) {}
  enum class ErrorCode {
    OK = 0,
    INVALID_URL = 1,
    INVALID_HOST = 2,
    CONNECTION_FAIL = 3,
    SEND_CLOSE_BY_PEER = 4,
    SEND_SOCKET_ERROR = 5,
    TIMEOUT = 6,
    POOL_GET_CONNECTION_FAIL = 7,
    POOL_INVALID_CONNECTION = 8,
  };
  int result{0};
  HttpResp::sptr resp;
  std::string error;
};

//Connection是client端的概念，而Session是Server端的概念
class HttpConnection : public SocketStream {
 public:
  using sptr = std::shared_ptr<HttpConnection>;

  static HttpResult::sptr DoRequest(HttpMethod method, Uri::sptr uri,
                                    const HttpReq::MapType& headers = {},
                                    const std::string& body = {},
                                    uint64_t timeout_ms = 3000);

  static HttpResult::sptr DoRequest(HttpMethod method, const std::string& url,
                                    const HttpReq::MapType& headers = {},
                                    const std::string& body = {},
                                    uint64_t timeout_ms = 3000);

  static HttpResult::sptr DoRequest(HttpReq::sptr req, Uri::sptr uri,
                                    uint64_t timeout_ms = 3000);

  static HttpResult::sptr DoGet(const std::string& url,
                                const HttpReq::MapType& headers = {},
                                const std::string& body = {},
                                uint64_t timeout_ms = 3000);

  static HttpResult::sptr DoGet(Uri::sptr uri,
                                const HttpReq::MapType& headers = {},
                                const std::string& body = {},
                                uint64_t timeout_ms = 3000);

  static HttpResult::sptr DoPost(const std::string& url,
                                 const HttpReq::MapType& headers = {},
                                 const std::string& body = {},
                                 uint64_t timeout_ms = 3000);

  static HttpResult::sptr DoPost(Uri::sptr uri,
                                 const HttpReq::MapType& headers = {},
                                 const std::string& body = {},
                                 uint64_t timeout_ms = 3000);

  HttpConnection(Socket::sptr sock, bool owner = true);

public:
  HttpResp::sptr recvResponse();
  int sendRequest(HttpReq::sptr req);
  
  void setCreateTime(uint64_t t) { m_createTime = t; }
  uint64_t getCreateTime() const { return m_createTime; }
  
private:
  uint64_t m_createTime{0};  //创建时间
};

//对于同一个host，可以复用连接
class HttpConnectionPool {
 public:
  using sptr = std::shared_ptr<HttpConnectionPool>;
  using MutexType = Mutex;

  HttpConnectionPool(const std::string& host, const std::string& vhost,
                     uint32_t port, uint32_t maxSize, uint32_t maxAliveTime,
                     uint32_t maxRequest);

  HttpConnection::sptr getConnection();

  HttpResult::sptr doRequest(HttpMethod method, Uri::sptr uri,
                             const HttpReq::MapType& headers = {},
                             const std::string& body = {},
                             uint64_t timeout_ms = 3000);

  HttpResult::sptr doRequest(HttpMethod method, const std::string& url,
                             const HttpReq::MapType& headers = {},
                             const std::string& body = {},
                             uint64_t timeout_ms = 3000);

  HttpResult::sptr doRequest(HttpReq::sptr req,
                             uint64_t timeout_ms = 3000);

  HttpResult::sptr doGet(const std::string& url,
                         const HttpReq::MapType& headers = {},
                         const std::string& body = {}, uint64_t timeout_ms = 3000);

  HttpResult::sptr doGet(Uri::sptr uri, const HttpReq::MapType& headers = {},
                         const std::string& body = {}, uint64_t timeout_ms = 3000);

  HttpResult::sptr doPost(const std::string& url,
                          const HttpReq::MapType& headers = {},
                          const std::string& body = {}, uint64_t timeout_ms = 3000);

  HttpResult::sptr doPost(Uri::sptr uri, const HttpReq::MapType& headers = {},
                          const std::string& body = {}, uint64_t timeout_ms = 3000);

 private:
  static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);

 private:
  std::string m_host;
  std::string m_vhost;
  uint32_t m_port{0};
  uint32_t m_maxSize{0};
  uint32_t m_maxAliveTime{0};
  uint32_t m_maxRequest{0};

  MutexType m_mutex;
  std::list<HttpConnection*> m_conns;
  std::atomic<uint32_t> m_total{0};
};
}  //namespace Http
}  //namespace East
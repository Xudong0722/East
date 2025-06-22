/*
 * @Author: Xudong0722 
 * @Date: 2025-06-18 23:25:58 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-22 13:52:59
 */

#pragma once
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
  };
  int result{0};
  HttpResp::sptr resp;
  std::string error;
};

//Connection是client端的概念，而Session是Server端的概念
class HttpConnection: public SocketStream {
public:
  using sptr = std::shared_ptr<HttpConnection>;

  static HttpResult::sptr DoRequest(HttpMethod method,
                                    Uri::sptr uri,
                                    const HttpReq::MapType& headers,
                                    const std::string& body,
                                    uint64_t timeout_ms = 3000);

  static HttpResult::sptr DoRequest(HttpMethod method,
                                    const std::string& url,
                                    const HttpReq::MapType& headers,
                                    const std::string& body,
                                    uint64_t timeout_ms = 3000);                              

  static HttpResult::sptr DoRequest(HttpReq::sptr req,
                                    Uri::sptr uri,
                                    uint64_t timeout_ms = 3000);
  
  static HttpResult::sptr DoGet(const std::string& url,
                                const HttpReq::MapType& headers,
                                const std::string& body,
                                uint64_t timeout_ms = 3000);

  static HttpResult::sptr DoGet(Uri::sptr uri,
                                const HttpReq::MapType& headers,
                                const std::string& body,
                                uint64_t timeout_ms = 3000);

  static HttpResult::sptr DoPost(const std::string& url,
                                 const HttpReq::MapType& headers,
                                 const std::string& body,
                                 uint64_t timeout_ms = 3000);         

  static HttpResult::sptr DoPost(Uri::sptr uri,
                                 const HttpReq::MapType& headers,
                                 const std::string& body,
                                 uint64_t timeout_ms = 3000);

  HttpConnection(Socket::sptr sock, bool owner = true);
  HttpResp::sptr recvResponse();
  int sendRequest(HttpReq::sptr req);
};

}//namespace Http
}//namespace East
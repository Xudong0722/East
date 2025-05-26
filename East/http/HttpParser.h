/*
 * @Author: Xudong0722 
 * @Date: 2025-05-23 00:13:02 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-23 00:28:55
 */

#pragma once

#include "Http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace East {
namespace Http {
    
class HttpReqParser {
public:
  using sptr = std::shared_ptr<HttpReqParser>;
  HttpReqParser();

  size_t execute(char* data, size_t len);
  int isFinished();
  int hasError();

  HttpReq::sptr getData() const { return m_req; }
  void setError(int v) { m_error = v; }

private:
  HttpReq::sptr m_req;
  http_parser m_parser;
  int m_error {0};
  /*
  error code:
  1000: Invalid HTTP method
  1001: Invalid HTTP version
  1002: Invalid HTTP field length
  */
}; //class HttpReqParser

class HttpRespParser {
public:
  using sptr = std::shared_ptr<HttpRespParser>;
  HttpRespParser();
  size_t execute(const char* data, size_t len, size_t off);
  int isFinished();
  int hasError();
private:
  httpclient_parser m_parser;
  HttpResp::sptr m_resp;
  int m_error{0};
};  //class HttpRespParser
} //namespace Http
} //namespace East
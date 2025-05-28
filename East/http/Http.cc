/*
 * @Author: Xudong0722 
 * @Date: 2025-05-19 17:18:43 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-28 22:31:04
 */

#include "Http.h"
#include <iostream>
namespace East {
namespace Http {
  
static std::map<std::string, HttpMethod> s_str_method{
#define XX(num, name, string) \
  {#string, static_cast<HttpMethod>(num)},
  HTTP_METHOD_MAP(XX)
#undef XX
};

HttpMethod StringToHttpMethod(const std::string& m) {
  if(s_str_method.find(m) == s_str_method.end()) {
    return HttpMethod::INVALID;
  }
  return s_str_method.at(m);
}

HttpMethod CharsToHttpMethod(const char* m) {
  return StringToHttpMethod(std::string(m));
}

const char* HttpMethodToString(HttpMethod m) {
  switch(m){
#define XX(num, name, string) \
  case HttpMethod::name:\
    return #string;
  HTTP_METHOD_MAP(XX)
#undef XX
  default:
    return "<unknown>";
  }
}

const char* HttpStatusToString(HttpStatus s) {
  switch(s) {
#define XX(num, name, desc) \
    case HttpStatus::name :\
      return #desc;
  HTTP_STATUS_MAP(XX)
#undef XX
    default:
      return "<unknown>";
  }
}

bool CaseInsensitiveLess::operator()(const std::string& lhs, const std::string& rhs) const {
  return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

HttpReq::HttpReq(uint8_t version, bool close) 
  : m_version(version)
  , m_close(close)
  , m_path("/") {

}

std::string HttpReq::getHeader(const std::string& key, const std::string& def) {
  auto it = m_headers.find(key);
  if(it == m_headers.end()) {
    return def;
  }
  return it->second;
}

std::string HttpReq::getParam(const std::string& key, const std::string& def) {
  auto it = m_params.find(key);
  if(it == m_params.end()) {
    return def;
  }
  return it->second;
}

std::string HttpReq::getCookie(const std::string& key, const std::string& def) {
  auto it = m_cookie.find(key);
  if(it == m_cookie.end()) {
    return def;
  }
  return it->second;
}

void HttpReq::setHeader(const std::string& key, const std::string& val) {
  m_headers[key] = val;
}

void HttpReq::setParam(const std::string& key, const std::string& val) {
  m_params[key] = val;
}

void HttpReq::setBody(const std::string& key, const std::string& val) {
  m_cookie[key] = val;
}

void HttpReq::delHeader(const std::string& key) {
  m_headers.erase(key);
}

void HttpReq::delParam(const std::string& key) {
  m_params.erase(key);
}

void HttpReq::delCookie(const std::string& key) {
  m_cookie.erase(key);
}

bool HttpReq::hasHeader(const std::string& key, std::string* val) {
  auto it = m_headers.find(key);
  if(it == m_headers.end()) {
    return false;
  }
  if(nullptr != val) {
    *val = it->second;
  }
  return true;
}

bool HttpReq::hasParam(const std::string& key, std::string* val) {
  auto it = m_params.find(key);
  if(it == m_params.end()) {
    return false;
  }
  if(nullptr != val) {
    *val = it->second;
  }
  return true;
}

bool HttpReq::hasCookie(const std::string& key, std::string* val) {
  auto it = m_cookie.find(key);
  if(it == m_cookie.end()) {
    return false;
  }
  if(nullptr != val) {
    *val = it->second;
  }
  return true;
}

std::ostream&  HttpReq::dump(std::ostream& os) const {
  os << HttpMethodToString(m_method) << " "
     << m_path 
     << (m_query.empty() ? "" : "?")
     << m_query
     << (m_fragment.empty() ? "" : "#")
     << " HTTP/"
     << ((uint32_t)(m_version >> 4))
     << "."
     << ((uint32_t)(m_version & 0x0F))
     << "\r\n";

  os << "connnection: " << (m_close ? "close" : "keep-alive") << "\r\n";
  for(const auto&item : m_headers) {
    if(strcasecmp(item.first.c_str(), "connection") == 0) continue;
    os << item.first <<":" << item.second << "\r\n";
  }
  
  if(!m_body.empty()) {
    os << "content-length: " << m_body.size() << "\r\n\r\n"
      << m_body;
  }else{
    os << "\r\n";
  }
  return os;
}

std::string HttpReq::toString() const {
  std::stringstream ss;
  dump(ss);
  return ss.str();
}

HttpResp::HttpResp(uint8_t version, bool close)
 : m_version(version)
 , m_close(close) {

}

std::string HttpResp::getHeader(const std::string& key, const std::string& def) {
  auto it = m_headers.find(key);
  if(it == m_headers.end()) {
    return def;
  }
  return it->second; 
}

void HttpResp::setHeader(const std::string& key, const std::string& val) {
  m_headers[key] = val;
}

void HttpResp::delHeader(const std::string& key) {
  m_headers.erase(key);
}

std::ostream& HttpResp::dump(std::ostream& os) const {
  os << "HTTP/"
     << (uint32_t)(m_version >> 4)
     <<"."
     << (uint32_t)(m_version & 0x0F)
     << " "
     << (uint32_t)m_status
     << " "
     << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
     << "\r\n";

    for(const auto& item : m_headers) {
      if(strcasecmp(item.first.c_str(), "connection") == 0) continue;
      os << item.first << ":" << item.second << "\r\n";
    }

    os << "connnection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    if(!m_body.empty()) {
      os << "content-length: " << m_body.size() << "\r\n\r\n"
        << m_body;
    }else{
      os << "\r\n";
    }
    return os;
}

std::string HttpResp::toString() const {
  std::stringstream ss;
  dump(ss);
  return ss.str();
}

} //namespace Http
} //namespace East
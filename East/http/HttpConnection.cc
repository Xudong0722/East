/*
 * @Author: Xudong0722 
 * @Date: 2025-06-18 23:26:15 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-22 13:57:45
 */
#include <iostream>

#include "../include/util.h"
#include "../include/Elog.h"
#include "HttpConnection.h"
#include "HttpParser.h"
namespace East {
namespace Http {

static Logger::sptr g_logger = ELOG_NAME("system");

std::string HttpResult::toString() const {
  std::stringstream ss;
  ss << "[HttpResult result=" << result
     //<< ", resp = " << (resp ? resp->toString() : "nullptr")
     << ", error = " << error << "]";
  return ss.str();
}

HttpConnection::HttpConnection(Socket::sptr sock, bool owner)
    : SocketStream(sock, owner) {}

HttpResp::sptr HttpConnection::recvResponse() {
  HttpRespParser::sptr parser = std::make_shared<HttpRespParser>();

  size_t buf_size = HttpRespParser::GetHttpRespParserBufferSize();
  std::unique_ptr<char[]> buf(new char[buf_size + 1]);
  int offset = 0;  //当前读到哪里了
  char* data = buf.get();
  do {
    int len = read(data + offset, buf_size - offset);
    if (len <= 0) {
      close();
      return nullptr;
    }
    len += offset;  //所有已读数据的长度
    data[len] = '\0';  //保证字符串结尾
    int parse_len = parser->execute(data, len, false);  //当前已经可以解析的长度
    if (parser->hasError()) {
      close();
      return nullptr;
    }
    //[...parse_len...len.....buf_size]
    //因为execute会将data的起始变成data+parse_len, 所以这里的offset就可以直接用len-parse_len
    //这样后面read(data+offset...)还是从上次没读过的地方继续读的
    offset = len - parse_len;
    if (offset == (int)buf_size) {
      close();
      return nullptr;
    }
    if (parser->isFinished()) {
      break;
    }
  } while (true);
  auto& client_parser = parser->getParser();
  std::cout << "----------------------" << client_parser.chunked << std::endl;
  if (client_parser.chunked) {
    std::string body;
    int len = offset;
    do {
      do {
        int rt = read(data + len, buf_size - len);
        if (rt <= 0) {
          close();
          return nullptr;
        }
        len += rt;
        data[len] = '\0';  //保证字符串结尾
        size_t parser_len = parser->execute(data, len, true);
        if (parser->hasError()) {
          close();
          return nullptr;
        }
        len -= parser_len;
        if (len == (int)buf_size) {
          close();
          return nullptr;
        }
      } while (!parser->isFinished());
      if (client_parser.content_len <= len) {
        body.append(data, client_parser.content_len);
        memmove(data, data + client_parser.content_len,
                len - client_parser.content_len);
        len -= client_parser.content_len;
      } else {
        body.append(data, len);
        int left = client_parser.content_len - len;
        while (left > 0) {
          int rt = read(data, left > (int)buf_size ? buf_size : left);
          if (rt <= 0) {
            close();
            return nullptr;
          }
          body.append(data, rt);
          left -= rt;
        }
        len = 0;
      }
    } while (!client_parser.chunks_done);
    parser->getData()->setBody(body);
  } else {
    int64_t body_len = parser->getContentLength();  //body需要我们单独解析
    if (body_len > 0) {
      std::string body(body_len, ' ');
      //注意parser的execute只会处理header，并且会把剩下的部分移动到data前面，所以直接从开始读即可
      int len = 0;
      if (body_len >= offset) {
        //先读这么多
        memcpy(&body[0], data, offset);
        len = offset;
      } else {
        memcpy(&body[0], data, len);
        len = body_len;
      }
      body_len -= offset;
      if (body_len > 0) {
        if (readFixSize(&body[len], body_len) <= 0) {
          close();
          return nullptr;
        }
        parser->getData()->setBody(body);
      }
    }
  }

  return parser->getData();
}

int HttpConnection::sendRequest(HttpReq::sptr req) {
  std::stringstream ss;
  ss << *req;
  std::string data = ss.str();
  return writeFixSize(data.c_str(), data.size());
}

HttpConnection::~HttpConnection() {
  ELOG_INFO(g_logger) << "HttpConnection::~HttpConnection()";
}

HttpResult::sptr HttpConnection::DoRequest(HttpMethod method, Uri::sptr uri,
                                           const HttpReq::MapType& headers,
                                           const std::string& body,
                                           uint64_t timeout_ms) {
  HttpReq::sptr req = std::make_shared<HttpReq>();
  req->setPath(uri->getPath());
  req->setMethod(method);
  req->setBody(body);
  req->setQuery(uri->getQuery());
  req->setFragment(uri->getFragment());
  bool has_host{false};
  for (const auto& item : headers) {
    if (strcasecmp(item.first.c_str(), "connection") == 0) {
      if (strcasecmp(item.second.c_str(), "keep-alive") == 0) {
        req->setClose(false);
        continue;
      }
    }

    if (!has_host && strcasecmp(item.first.c_str(), "host") == 0) {
      has_host = !item.second.empty();
    }
    req->setHeader(item.first, item.second);
  }
  if (!has_host) {
    req->setHeader("Host", uri->getHost());
  }
  return DoRequest(req, uri, timeout_ms);
}

HttpResult::sptr HttpConnection::DoRequest(HttpMethod method,
                                           const std::string& url,
                                           const HttpReq::MapType& headers,
                                           const std::string& body,
                                           uint64_t timeout_ms) {
  Uri::sptr uri = Uri::Create(url);
  if (nullptr == uri) {
    return std::make_shared<HttpResult>(
        Enum2Utype(HttpResult::ErrorCode::INVALID_URL), nullptr,
        "Invalid URL: " + url);
  }
  return DoRequest(method, uri, headers, body, timeout_ms);
}

HttpResult::sptr HttpConnection::DoRequest(HttpReq::sptr req, Uri::sptr uri,
                                           uint64_t timeout_ms) {
  Address::sptr addr = uri->createAddress();
  if (nullptr == addr) {
    return std::make_shared<HttpResult>(
        Enum2Utype(HttpResult::ErrorCode::INVALID_HOST), nullptr,
        "Invalid host: " + uri->getHost());
  }
  Socket::sptr sock = Socket::CreateTCP(addr);
  bool conn_res = sock->connect(addr, timeout_ms);
  if (!conn_res) {
    return std::make_shared<HttpResult>(
        Enum2Utype(HttpResult::ErrorCode::CONNECTION_FAIL), nullptr,
        "Connect failed: " + addr->toString());
  }

  sock->setRecvTimeout(timeout_ms);
  HttpConnection::sptr conn = std::make_shared<HttpConnection>(sock);

  int rt = conn->sendRequest(req);
  if (rt == 0) {
    return std::make_shared<HttpResult>(
        Enum2Utype(HttpResult::ErrorCode::SEND_CLOSE_BY_PEER), nullptr,
        "Send close by peer: " + addr->toString());
  } else if (rt < 0) {
    return std::make_shared<HttpResult>(
        Enum2Utype(HttpResult::ErrorCode::SEND_SOCKET_ERROR), nullptr,
        "Send socket error: " + std::to_string(errno) +
            " errstr: " + std::string(strerror(errno)));
  }

  auto resp = conn->recvResponse();
  if (nullptr == resp) {
    return std::make_shared<HttpResult>(
        Enum2Utype(HttpResult::ErrorCode::TIMEOUT), nullptr,
        "Receive response timeout: " + addr->toString() +
            " timeout ms: " + std::to_string(timeout_ms));
  }
  return std::make_shared<HttpResult>(Enum2Utype(HttpResult::ErrorCode::OK),
                                      resp,
                                      "Request success: " + addr->toString());
}

HttpResult::sptr HttpConnection::DoGet(const std::string& url,
                                       const HttpReq::MapType& headers,
                                       const std::string& body,
                                       uint64_t timeout_ms) {
  Uri::sptr uri = Uri::Create(url);
  if (nullptr == uri) {
    return std::make_shared<HttpResult>(
        Enum2Utype(HttpResult::ErrorCode::INVALID_URL), nullptr,
        "Invalid URL: " + url);
  }
  return DoGet(uri, headers, body, timeout_ms);
}

HttpResult::sptr HttpConnection::DoGet(Uri::sptr uri,
                                       const HttpReq::MapType& headers,
                                       const std::string& body,
                                       uint64_t timeout_ms) {
  return DoRequest(HttpMethod::GET, uri, headers, body, timeout_ms);
}

HttpResult::sptr HttpConnection::DoPost(const std::string& url,
                                        const HttpReq::MapType& headers,
                                        const std::string& body,
                                        uint64_t timeout_ms) {
  Uri::sptr uri = Uri::Create(url);
  if (nullptr == uri) {
    return std::make_shared<HttpResult>(
        Enum2Utype(HttpResult::ErrorCode::INVALID_URL), nullptr,
        "Invalid URL: " + url);
  }
  return DoPost(uri, headers, body, timeout_ms);
}

HttpResult::sptr HttpConnection::DoPost(Uri::sptr uri,
                                        const HttpReq::MapType& headers,
                                        const std::string& body,
                                        uint64_t timeout_ms) {
  return DoRequest(HttpMethod::POST, uri, headers, body, timeout_ms);
}

HttpConnectionPool::HttpConnectionPool(const std::string& host,
                                       const std::string& vhost, uint32_t port,
                                       uint32_t maxSize, uint32_t maxAliveTime,
                                       uint32_t maxRequest) 
  : m_host(host)
  , m_vhost(vhost)
  , m_port(port)
  , m_maxSize(maxSize)
  , m_maxAliveTime(maxAliveTime)
  , m_maxRequest(maxRequest){}

HttpConnection::sptr HttpConnectionPool::getConnection() {
  uint64_t now = GetCurrentTimeInMs();
  MutexType::LockGuard lock(m_mutex);
  std::vector<HttpConnection*> to_delete;
  HttpConnection* res{nullptr};
  while(!m_conns.empty()) {
    auto conn = m_conns.front();
    m_conns.pop_front();
    if(!conn->isConnected()) {
      //连接已经断开了
      to_delete.push_back(conn);
      m_conns.pop_front();
      --m_total;
      continue;
    }
    if(conn->getCreateTime() + m_maxAliveTime < now) {
      //连接过期
      to_delete.push_back(conn);
      --m_total;
      m_conns.pop_front();
      continue;
    }
    
    res = conn;
    break;
  }
  
  lock.unlock();
  for(auto& conn : to_delete) {
    //释放连接
    if(conn) {
      conn->close();
      delete conn;
    }
  }

  if(nullptr == res) {
    if(m_total >= m_maxSize) {
      //连接池已满
      return nullptr;
    }

    //创建新的连接
    IPAddress::sptr addr = Address::LookupAnyIPAddress(m_host);
    if(nullptr == addr) {
      ELOG_ERROR(g_logger) << "Lookup host failed: " <<m_host;
      return nullptr;
    }

    addr->setPort(m_port);
    Socket::sptr sock = Socket::CreateTCP(addr);
    if(!sock->connect(addr)) {
      ELOG_ERROR(g_logger) << "Connect to host failed: " << addr->toString();
      return nullptr;
    }

    res = new HttpConnection(sock);
    ++m_total;
    res->setCreateTime(GetCurrentTimeInMs());
  }
  return HttpConnection::sptr(res, std::bind(&HttpConnectionPool::ReleasePtr, std::placeholders::_1, this));
}

HttpResult::sptr HttpConnectionPool::doRequest(HttpMethod method, Uri::sptr uri,
                                               const HttpReq::MapType& headers,
                                               const std::string& body,
                                               uint64_t timeout_ms) {
  std::stringstream ss;
  ss << uri->getPath()
     << (uri->getQuery().empty() ? "" : "?" + uri->getQuery())
     << (uri->getFragment().empty() ? "" : "#" + uri->getFragment());
  return doRequest(method, ss.str(), headers, body, timeout_ms);
}

HttpResult::sptr HttpConnectionPool::doRequest(HttpMethod method,
                                               const std::string& url,
                                               const HttpReq::MapType& headers,
                                               const std::string& body,
                                               uint64_t timeout_ms) {
  HttpReq::sptr req = std::make_shared<HttpReq>();
  req->setPath(url);
  req->setMethod(method);
  req->setBody(body);
  req->setClose(false);
  bool has_host{false};
  for (const auto& item : headers) {
    if (strcasecmp(item.first.c_str(), "connection") == 0) {
      if (strcasecmp(item.second.c_str(), "keep-alive") == 0) {
        req->setClose(false);
        continue;
      }
    }

    if (!has_host && strcasecmp(item.first.c_str(), "host") == 0) {
      has_host = !item.second.empty();
    }
    req->setHeader(item.first, item.second);
  }
  if (!has_host) {
    if(m_vhost.empty()) {
      req->setHeader("Host", m_host);
    } else {
      req->setHeader("Host", m_vhost);
    }
  }
  return doRequest(req, timeout_ms);
}

HttpResult::sptr HttpConnectionPool::doRequest(HttpReq::sptr req,
                                               uint64_t timeout_ms) {
  auto conn = getConnection();
  if (nullptr == conn) {
    return std::make_shared<HttpResult>(
        Enum2Utype(HttpResult::ErrorCode::POOL_GET_CONNECTION_FAIL), nullptr,
        "Get connection from pool failed: " + m_host + ":" + std::to_string(m_port));
  }
  auto sock = conn->getSocket();
  if (nullptr == sock) {
    return std::make_shared<HttpResult>(
        Enum2Utype(HttpResult::ErrorCode::POOL_INVALID_CONNECTION), nullptr,
        "Invalid connection in pool: " + m_host + ":" + std::to_string(m_port)); 
  }
  sock->setRecvTimeout(timeout_ms);
  auto addr = sock->getRemoteAddr();
  int rt = conn->sendRequest(req);
  if (rt == 0) {
    return std::make_shared<HttpResult>(
        Enum2Utype(HttpResult::ErrorCode::SEND_CLOSE_BY_PEER), nullptr,
        "Send close by peer: " + addr->toString());
  } else if (rt < 0) {
    return std::make_shared<HttpResult>(
        Enum2Utype(HttpResult::ErrorCode::SEND_SOCKET_ERROR), nullptr,
        "Send socket error: " + std::to_string(errno) +
            " errstr: " + std::string(strerror(errno)));
  }

  auto resp = conn->recvResponse();
  if (nullptr == resp) {
    return std::make_shared<HttpResult>(
        Enum2Utype(HttpResult::ErrorCode::TIMEOUT), nullptr,
        "Receive response timeout: " + addr->toString() +
            " timeout ms: " + std::to_string(timeout_ms));
  }
  return std::make_shared<HttpResult>(Enum2Utype(HttpResult::ErrorCode::OK),
                                      resp,
                                      "Request success: " + addr->toString());
}

HttpResult::sptr HttpConnectionPool::doGet(const std::string& url,
                                           const HttpReq::MapType& headers,
                                           const std::string& body,
                                           uint64_t timeout_ms) {
  return doRequest(HttpMethod::GET, url, headers, body, timeout_ms);
}

HttpResult::sptr HttpConnectionPool::doGet(Uri::sptr uri,
                                           const HttpReq::MapType& headers,
                                           const std::string& body,
                                           uint64_t timeout_ms) {
  std::stringstream ss;
  ss << uri->getPath()
     << (uri->getQuery().empty() ? "" : "?" + uri->getQuery())
     << (uri->getFragment().empty() ? "" : "#" + uri->getFragment());
  return doRequest(HttpMethod::GET, ss.str(), headers, body, timeout_ms);
}

HttpResult::sptr HttpConnectionPool::doPost(const std::string& url,
                                            const HttpReq::MapType& headers,
                                            const std::string& body,
                                            uint64_t timeout_ms) {
  return doRequest(HttpMethod::POST, url, headers, body, timeout_ms);
}

HttpResult::sptr HttpConnectionPool::doPost(Uri::sptr uri,
                                            const HttpReq::MapType& headers,
                                            const std::string& body,
                                            uint64_t timeout_ms) {
  std::stringstream ss;
  ss << uri->getPath()
     << (uri->getQuery().empty() ? "" : "?" + uri->getQuery())
     << (uri->getFragment().empty() ? "" : "#" + uri->getFragment());
  return doRequest(HttpMethod::POST, ss.str(), headers, body, timeout_ms);
}

void HttpConnectionPool::ReleasePtr(HttpConnection* ptr,
                                    HttpConnectionPool* pool) {
  if(nullptr == ptr || nullptr == pool) return ;
  ptr->setRequestCount(ptr->getRequestCount() + 1);
  //将仍旧可以利用的连接放回连接池
  auto b1 = ptr->isConnected();
  auto b2 = (ptr->getCreateTime() + pool->m_maxAliveTime) < GetCurrentTimeInMs();
  auto b3 = ptr->getRequestCount() >= pool->m_maxRequest;
  ELOG_INFO(g_logger) << "ReleasePtr: isConnected=" << b1
                       << ", getCreateTime() + m_maxAliveTime > GetCurrentTimeInMs()="
                       << b2
                       << "current time=" << GetCurrentTimeInMs()
                        << ", ptr->getCreateTime()=" << ptr->getCreateTime()
                        << "maxAliveTime=" << pool->m_maxAliveTime
                       << ", getRequestCount() >= m_maxRequest=" << b3;

  if(!ptr->isConnected() 
     || ((ptr->getCreateTime() + pool->m_maxAliveTime) < GetCurrentTimeInMs())
     || ptr->getRequestCount() >= pool->m_maxRequest) {
    //连接已经断开了或者连接过期了，请求次数超过限制了
      ptr->close();
      delete ptr;
      --pool->m_total;
      return ;
  }
  MutexType::LockGuard lock(pool->m_mutex);
  pool->m_conns.push_back(ptr);
}

}  //namespace Http
}  //namespace East
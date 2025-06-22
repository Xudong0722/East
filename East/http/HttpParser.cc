/*
 * @Author: Xudong0722 
 * @Date: 2025-05-23 00:14:54 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-21 16:57:15
 */

#include "HttpParser.h"
#include "../include/Elog.h"
#include "../include/Config.h"

namespace East {
namespace Http {

static East::Logger::sptr g_logger = ELOG_ROOT();
static East::ConfigVar<uint64_t>::sptr g_http_req_parser_buffer_size =
    East::Config::Lookup<uint64_t>("http.req_parser.buffer_size", 4 * 1024ull, "http request parser buffer size");
static East::ConfigVar<uint64_t>::sptr g_http_req_max_body_size =
    East::Config::Lookup<uint64_t>("http.req_parser.max_body_size", 6 * 1024 * 1024ull, "http request parser max body size");
static East::ConfigVar<uint64_t>::sptr g_http_resp_parser_buffer_size =
    East::Config::Lookup<uint64_t>("http.resp_parser.buffer_size", 4 * 1024ull, "http response parser buffer size");
static East::ConfigVar<uint64_t>::sptr g_http_resp_max_body_size =
    East::Config::Lookup<uint64_t>("http.resp_parser.max_body_size", 6 * 1024 * 1024ull, "http response parser max body size");

static uint64_t s_http_req_parser_buffer_size = 0;
static uint64_t s_http_req_max_body_size = 0;
static uint64_t s_http_resp_parser_buffer_size = 0;
static uint64_t s_http_resp_max_body_size = 0;

uint64_t HttpReqParser::GetHttpReqParserBufferSize() {
  return s_http_req_parser_buffer_size;
}

uint64_t HttpReqParser::GetHttpReqMaxBodySize() {
  return s_http_req_max_body_size;  
}

uint64_t HttpRespParser::GetHttpRespParserBufferSize() {
  return s_http_resp_parser_buffer_size;
}

uint64_t HttpRespParser::GetHttpRespMaxBodySize() {
  return s_http_resp_max_body_size;
}

namespace {

struct _RequestSizeIniter {
  _RequestSizeIniter() {
    s_http_req_parser_buffer_size = g_http_req_parser_buffer_size->getValue();
    s_http_req_max_body_size = g_http_req_max_body_size->getValue();
    s_http_resp_parser_buffer_size = g_http_resp_parser_buffer_size->getValue();
    s_http_resp_max_body_size = g_http_resp_max_body_size->getValue();
    
    g_http_req_parser_buffer_size->addListener([](const uint64_t& old_v, const uint64_t new_v) {
      s_http_req_parser_buffer_size = new_v;
      ELOG_INFO(g_logger) << "http.req_parser.buffer_size changed from " << old_v << " to " << new_v;
    });
    g_http_req_max_body_size->addListener([](const uint64_t& old_v, const uint64_t new_v) {
      s_http_req_max_body_size = new_v;
      ELOG_INFO(g_logger) << "http.req_parser.max_body_size changed from " << old_v << " to " << new_v;
    });
    g_http_resp_parser_buffer_size->addListener([](const uint64_t& old_v, const uint64_t new_v) {
      s_http_resp_parser_buffer_size = new_v;
      ELOG_INFO(g_logger) << "http.resp_parser.buffer_size changed from " << old_v << " to " << new_v;
    });
    g_http_resp_max_body_size->addListener([](const uint64_t& old_v, const uint64_t new_v) {
      s_http_resp_max_body_size = new_v;
      ELOG_INFO(g_logger) << "http.resp_parser.max_body_size changed from " << old_v << " to " << new_v;
    });
  }
};

static _RequestSizeIniter _req_initer;
}

void on_request_method(void* data, const char* at, size_t length) {
  HttpReqParser* parser = static_cast<HttpReqParser*>(data);
  if(nullptr == parser) return;
  
  HttpMethod method = CharsToHttpMethod(std::string(at, length).c_str());

  if(method == HttpMethod::INVALID) {
    ELOG_WARN(g_logger) << "Invalid HTTP method: " << std::string(at, length);
    parser->setError(1000);
  }
  parser->getData()->setMethod(method);
}

void on_request_uri(void* data, const char* at, size_t length) {
}

void on_request_fragment(void* data, const char* at, size_t length) {
  HttpReqParser* parser = static_cast<HttpReqParser*>(data);
  if(nullptr == parser) return;
  parser->getData()->setFragment(std::string(at, length));
}

void on_request_path(void* data, const char* at, size_t length) {
  HttpReqParser* parser = static_cast<HttpReqParser*>(data);
  if(nullptr == parser) return;
  parser->getData()->setPath(std::string(at, length));
}

void on_request_query(void* data, const char* at, size_t length) {
  HttpReqParser* parser = static_cast<HttpReqParser*>(data);
  if(nullptr == parser) return;
  parser->getData()->setQuery(std::string(at, length));
}

void on_request_version(void* data, const char* at, size_t length) {
  HttpReqParser* parser = static_cast<HttpReqParser*>(data);
  if(nullptr == parser) return;
  uint8_t v{0};
  if(strncmp(at, "HTTP/1.1", length) == 0) {
    v = 0x11;
  }else if(strncmp(at, "HTTP/1.0", length) == 0) {
    v = 0x10;
  }else{
    ELOG_WARN(g_logger) << "Invalid HTTP version: " << std::string(at, length);
    parser->setError(1001); // Invalid HTTP version error code
    return ;
  }
  parser->getData()->setVersion(v);
}

void on_request_header_done(void* data, const char* at, size_t length) {
}

void on_request_http_field(void* data, const char* field, size_t flen, const char* value, size_t vlen) {
  HttpReqParser* parser = static_cast<HttpReqParser*>(data);
  if(nullptr == parser) return;
  if(flen == 0) {
    ELOG_WARN(g_logger) << "Invalid http request field lenght == 0";
    parser->setError(1002); // Invalid field error code
  }
  parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}

HttpReqParser::HttpReqParser() {
  m_req.reset(new HttpReq());
  http_parser_init(&m_parser);
  m_parser.request_method = on_request_method;
  m_parser.request_uri = on_request_uri;
  m_parser.fragment = on_request_fragment;
  m_parser.request_path = on_request_path;
  m_parser.query_string = on_request_query;
  m_parser.http_version = on_request_version;
  m_parser.header_done = on_request_header_done;
  m_parser.http_field = on_request_http_field;
  m_parser.data = this;
}

/*
return value:
1:  解析完毕
-1: 解析出错
>0: 还没有解析完，返回值为已经解析的字节数
*/
size_t HttpReqParser::execute(char* data, size_t len) {
  size_t n = http_parser_execute(&m_parser, data, len, 0);
  //这里只会处理header，所以剩下的就是body，将body移动到前面，len-n就是body的长度
  //这里可能不会一次性就把header解析完毕，所以每次解析多少都会把后面的往前移动多少
  memmove(data, data + n, (len - n));
  return n;
}

int HttpReqParser::isFinished() {
  return http_parser_finish(&m_parser);
}

int HttpReqParser::hasError() {
  return m_error && http_parser_has_error(&m_parser);
}

uint64_t HttpReqParser::getContentLength() {
  return m_req->getHeaderAs<uint64_t>("content-length", 0);
}

void on_response_reason_phrase(void* data, const char* at, size_t length) {
  HttpRespParser* parser = static_cast<HttpRespParser*>(data);
  if(nullptr == parser) return;
  parser->getData()->setReason(std::string(at, length));
}

void on_response_status_code(void* data, const char* at, size_t length) {
  HttpRespParser* parser = static_cast<HttpRespParser*>(data);
  if(nullptr == parser) return;
  parser->getData()->setStatus(static_cast<HttpStatus>(atoi(at)));
}

void on_response_chunk_size(void* data, const char* at, size_t length) {
}

void on_response_version(void* data, const char* at, size_t length) {
  HttpRespParser* parser = static_cast<HttpRespParser*>(data);
  if(nullptr == parser) return;

  uint8_t version{0};
  if(strncasecmp(at, "HTTP/1.1", length) == 0) {
    version = 0x11;
  }else if(strncasecmp(at, "HTTP/1.0", length) == 0) {
    version = 0x10;
  }else{
    ELOG_WARN(g_logger) << "Invalid HTTP version: " << std::string(at, length);
    parser->setError(1001); // Invalid HTTP version error code
    return ;
  }
  parser->getData()->setVersion(version);
}

void on_response_header_done(void* data, const char* at, size_t length) {
}

void on_response_last_chunk(void* data, const char* at, size_t length) {
}

void on_response_http_field(void* data, const char* field, size_t flen, const char* value, size_t vlen) {
  HttpRespParser* parser = static_cast<HttpRespParser*>(data);
  if(flen == 0) {
    ELOG_WARN(g_logger) << "Invalid http request field lenght == 0";
    parser->setError(1002); // Invalid field error code
  }
  parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}

HttpRespParser::HttpRespParser() {
  m_resp.reset(new HttpResp());
  httpclient_parser_init(&m_parser);
  m_parser.reason_phrase = on_response_reason_phrase;
  m_parser.status_code = on_response_status_code;
  m_parser.chunk_size = on_response_chunk_size;
  m_parser.http_version = on_response_version;
  m_parser.header_done = on_response_header_done;
  m_parser.last_chunk = on_response_last_chunk;
  m_parser.http_field = on_response_http_field;
  m_parser.data = this;
}

size_t HttpRespParser::execute(char* data, size_t len, bool chunck) {
  if(chunck) {
    httpclient_parser_init(&m_parser);
  }
  size_t n = httpclient_parser_execute(&m_parser, data, len, 0);
  memmove(data, data + n, (len - n));
  return n;
}

int HttpRespParser::isFinished() {
  return httpclient_parser_finish(&m_parser);
}

int HttpRespParser::hasError() {
  return m_error && httpclient_parser_has_error(&m_parser);
}

uint64_t HttpRespParser::getContentLength() {
  return m_resp->getHeaderAs<uint64_t>("content-length", 0);
}
} //namespace Http
} //namespace East
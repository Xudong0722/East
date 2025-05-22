/*
 * @Author: Xudong0722 
 * @Date: 2025-05-23 00:14:54 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-23 00:32:43
 */

#include "HttpParser.h"

namespace East {
namespace Http {

void on_request_method(void* data, const char* at, size_t length) {
    
}

void on_request_uri(void* data, const char* at, size_t length) {
    
}

void on_request_fragment(void* data, const char* at, size_t length) {
    
}

void on_request_path(void* data, const char* at, size_t length) {
    
}

void on_request_query(void* data, const char* at, size_t length) {
    
}

void on_request_version(void* data, const char* at, size_t length) {
    
}

void on_request_header_done(void* data, const char* at, size_t length) {
    
}

void on_request_http_field(void* data, const char* field, size_t flen, const char* value, size_t vlen) {
    
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
}

size_t HttpReqParser::execute(const char* data, size_t len, size_t off) {
  return 0;
}

int HttpReqParser::isFinished() const {
  return 0;
}

int HttpReqParser::hasError() const {
  return 0;
}

void on_response_reason_phrase(void* data, const char* at, size_t length) {
    
}

void on_response_status_code(void* data, const char* at, size_t length) {
    
}

void on_response_chunk_size(void* data, const char* at, size_t length) {
    
}

void on_response_version(void* data, const char* at, size_t length) {
    
}

void on_response_header_done(void* data, const char* at, size_t length) {
    
}

void on_response_last_chunk(void* data, const char* at, size_t length) {
    
}

void on_response_http_field(void* data, const char* field, size_t flen, const char* value, size_t vlen) {
    
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
}

size_t HttpRespParser::execute(const char* data, size_t len, size_t off) {
  return 0;
}

int HttpRespParser::isFinished() const {
  return 0;
}

int HttpRespParser::hasError() const {
  return 0;
}
} //namespace Http
} //namespace East
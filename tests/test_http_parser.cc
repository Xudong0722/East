/*
 * @Author: Xudong0722 
 * @Date: 2025-05-28 21:57:16 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-28 22:10:48
 */

#include "../East/http/HttpParser.h"
#include "../East/include/Elog.h"

static East::Logger::sptr g_logger = ELOG_ROOT();

const char test_request_data[] =
    "GET / HTTP/1.1\r\n"
    "HOST: www.baidu.com\r\n"
    "Content-Length: 9\r\n\r\n"
    "123456789";

void test_request() {
  East::Http::HttpReqParser parser;
  std::string tmp = test_request_data;
  size_t s = parser.execute(&tmp[0], tmp.size());

  ELOG_INFO(g_logger) << "execute rt: " << s
                      << " has error: " << parser.hasError()
                      << " is finished: " << parser.isFinished()
                      << " total: " << tmp.size()
                      << " content-length: " << parser.getContentLength();

  tmp.resize(tmp.size() - s);
  ELOG_INFO(g_logger) << parser.getData()->toString();
  ELOG_INFO(g_logger) << tmp;
}

const char test_response_data[] =
    "HTTP/1.1 200 OK\r\n"
    "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
    "Server: Apache\r\n"
    "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
    "ETag: \"51-47cf7e6ee8400\"\r\n"
    "Accept-Ranges: bytes\r\n"
    "Content-Length: 81\r\n"
    "Cache-Control: max-age=86400\r\n"
    "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
    "Connection: Close\r\n"
    "Content-Type: text/html\r\n\r\n"
    "<html>\r\n"
    "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
    "</html>\r\n";

void test_response() {
  East::Http::HttpRespParser parser;
  std::string tmp = test_response_data;
  size_t s = parser.execute(&tmp[0], tmp.size());
  
  ELOG_INFO(g_logger) << "execute rt: " << s
                      << " has error: " << parser.hasError()
                      << " is finished: " << parser.isFinished()
                      << " total: " << tmp.size()
                      << " content_length: " << parser.getContentLength();

  tmp.resize(tmp.size() - s);
  ELOG_INFO(g_logger) << parser.getData()->toString();
  ELOG_INFO(g_logger) << tmp;
}

int main() {
  test_request();
  ELOG_INFO(g_logger) << "--------------------------------------------------";
  test_response();
  return 0;
}
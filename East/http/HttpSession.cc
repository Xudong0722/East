/*
 * @Author: Xudong0722 
 * @Date: 2025-06-10 22:11:26 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-21 17:26:48
 */

#include "HttpSession.h"
#include "HttpParser.h"
namespace East {
namespace Http {

HttpSession::HttpSession(Socket::sptr sock, bool owner)
    : SocketStream(sock, owner) {}

HttpReq::sptr HttpSession::recvRequest() {
  HttpReqParser::sptr parser = std::make_shared<HttpReqParser>();

  size_t buf_size = HttpReqParser::GetHttpReqParserBufferSize();
  std::unique_ptr<char[]> buf(new char[buf_size]);
  int offset = 0;  //当前读到哪里了
  char* data = buf.get();
  do {
    int len = read(data + offset, buf_size - offset);
    if (len <= 0) {
      close();
      return nullptr;
    }
    len += offset;                               //所有已读数据的长度
    int parse_len = parser->execute(data, len);  //当前已经可以解析的长度
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

  int64_t body_len = parser->getContentLength();  //body需要我们单独解析
  if (body_len > 0) {
    std::string body(' ', body_len);
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
  std::string keep_alive = parser->getData()->getHeader("Connection");
  if(!strcasecmp(keep_alive.c_str(), "keep-alive")){
    parser->getData()->setClose(false);
  }
  return parser->getData();
}

int HttpSession::sendResponse(HttpResp::sptr rsp) {
  std::stringstream ss;
  ss << *rsp;
  std::string data = ss.str();
  return writeFixSize(data.c_str(), data.size());
}

}  //namespace Http
}  //namespace East
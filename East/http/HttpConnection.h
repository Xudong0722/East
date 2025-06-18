/*
 * @Author: Xudong0722 
 * @Date: 2025-06-18 23:25:58 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-18 23:41:14
 */

#pragma once
#include "../include/SocketStream.h"
#include "Http.h"

namespace East {
namespace Http {

//Connection是client端的概念，而Session是Server端的概念
class HttpConnection: public SocketStream {
public:
  using sptr = std::shared_ptr<HttpConnection>;

  HttpConnection(Socket::sptr sock, bool owner = true);
  HttpResp::sptr recvResponse();
  int sendRequest(HttpReq::sptr req);
};

}//namespace Http
}//namespace East
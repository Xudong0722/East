/*
 * @Author: Xudong0722 
 * @Date: 2025-06-10 22:07:53 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-18 23:43:06
 */

#pragma once
#include "../include/SocketStream.h"
#include "Http.h"

namespace East {
namespace Http {

//Session是Server端的概念，而Connection是client端的概念
class HttpSession : public SocketStream {
 public:
  using sptr = std::shared_ptr<HttpSession>;

  HttpSession(Socket::sptr sock, bool owner = true);
  HttpReq::sptr recvRequest();
  int sendResponse(HttpResp::sptr rsp);
};

}  //namespace Http
}  //namespace East
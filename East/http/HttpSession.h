/*
 * @Author: Xudong0722 
 * @Date: 2025-06-10 22:07:53 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-10 22:16:56
 */

#pragma once
#include "../include/SocketStream.h"
#include "Http.h"

namespace East {
namespace Http {

class HttpSession: public SocketStream {
public:
  using sptr = std::shared_ptr<HttpSession>;

  HttpSession(Socket::sptr sock, bool owner = true);
  HttpReq::sptr recvRequest();
  int sendResponse(HttpResp::sptr rsp);
};

}//namespace Http
}//namespace East
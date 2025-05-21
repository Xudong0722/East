/*
 * @Author: Xudong0722 
 * @Date: 2025-05-19 14:08:39 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-05-19 18:31:42
 */

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <sstream>
#include <iostream>
#include <boost/lexical_cast.hpp>

namespace East {

namespace Http {
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  /* icecast */                     \
  XX(33, SOURCE,      SOURCE)       \

/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                 \
  XX(100, CONTINUE,                        Continue)                        \
  XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
  XX(102, PROCESSING,                      Processing)                      \
  XX(200, OK,                              OK)                              \
  XX(201, CREATED,                         Created)                         \
  XX(202, ACCEPTED,                        Accepted)                        \
  XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
  XX(204, NO_CONTENT,                      No Content)                      \
  XX(205, RESET_CONTENT,                   Reset Content)                   \
  XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
  XX(207, MULTI_STATUS,                    Multi-Status)                    \
  XX(208, ALREADY_REPORTED,                Already Reported)                \
  XX(226, IM_USED,                         IM Used)                         \
  XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
  XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
  XX(302, FOUND,                           Found)                           \
  XX(303, SEE_OTHER,                       See Other)                       \
  XX(304, NOT_MODIFIED,                    Not Modified)                    \
  XX(305, USE_PROXY,                       Use Proxy)                       \
  XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
  XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
  XX(400, BAD_REQUEST,                     Bad Request)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(402, PAYMENT_REQUIRED,                Payment Required)                \
  XX(403, FORBIDDEN,                       Forbidden)                       \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
  XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
  XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
  XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
  XX(409, CONFLICT,                        Conflict)                        \
  XX(410, GONE,                            Gone)                            \
  XX(411, LENGTH_REQUIRED,                 Length Required)                 \
  XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
  XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
  XX(414, URI_TOO_LONG,                    URI Too Long)                    \
  XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
  XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
  XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
  XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
  XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
  XX(423, LOCKED,                          Locked)                          \
  XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
  XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
  XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
  XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
  XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
  XX(501, NOT_IMPLEMENTED,                 Not Implemented)                 \
  XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
  XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
  XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
  XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
  XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
  XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
  XX(508, LOOP_DETECTED,                   Loop Detected)                   \
  XX(510, NOT_EXTENDED,                    Not Extended)                    \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \


enum class HttpMethod{
#define XX(num, name, string) name = num,
  HTTP_METHOD_MAP(XX)
#undef XX
  INVALID
};

enum class HttpStatus {
#define XX(code, name, desc) name = code,
  HTTP_STATUS_MAP(XX)
#undef XX
  INVALID
};

HttpMethod StringToHttpMethod(const std::string& m);
HttpMethod CharsToHttpMethod(const char* m);
const char* HttpMethodToString(HttpMethod m);
const char* HttpStatusToString(HttpStatus s);

struct CaseInsensitiveLess {
  bool operator()(const std::string& lhs, const std::string& rhs) const;
};

class HttpReq {
public:
  using sptr = std::shared_ptr<HttpReq>;
  using MapType = std::map<std::string, std::string, CaseInsensitiveLess>;

  HttpReq(uint8_t version = 0x11, bool close = true);

  HttpMethod getMethod() const {return m_method; }
  uint8_t getVersion() const { return m_version; }
  HttpStatus getStatus() const { return m_status; }
  const std::string& getPath() const { return m_path; }
  const std::string& getQuery() const { return m_query; }
  const std::string& getBody() const { return m_body; }

  const MapType& getHeaders() const { return m_headers; }
  const MapType& getParams() const { return m_params; }
  const MapType& getCookie() const { return m_cookie; }

  void setMethod(HttpMethod m) { m_method = m; }
  void setStatus(HttpStatus s) { m_status = s; }
  void setVersion(uint8_t v) { m_version = v; }

  void setPath(const std::string& v) { m_path = v; }
  void setQuery(const std::string& v) { m_query = v; }
  void setFragment(const std::string& v) { m_fragment = v; }
  void setBody(const std::string& v) { m_body = v; }

  void setHeaders(const MapType& v) { m_headers = v; }
  void setParams(const MapType& v) { m_params = v; }
  void setCookies(const MapType& v) { m_cookie = v; }
  void setClose(bool v) { m_close = v; }

  //查看对应结构中是否含有key，如果没有返回指定的默认字符串
  std::string getHeader(const std::string& key, const std::string& def = "");
  std::string getParam(const std::string& key, const std::string& def = "");
  std::string getCookie(const std::string& key, const std::string& def = "");

  bool isClose() const { return m_close; }
  void setHeader(const std::string& key, const std::string& val);
  void setParam(const std::string& key, const std::string& val);
  void setBody(const std::string& key, const std::string& val);

  void delHeader(const std::string& key);
  void delParam(const std::string& key);
  void delCookie(const std::string& key);

  bool hasHeader(const std::string& key, std::string* val = nullptr);
  bool hasParam(const std::string& key, std::string* val = nullptr);
  bool hasCookie(const std::string& key, std::string* val = nullptr);

  template<class T>
  bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T{}) {
    return checkGetAs(m_headers, key, val, def);
  }

  template<class T>
  T getHeaderAs(const std::string& key, const T& def = T{}) {
    return getAs(m_headers, key, def);
  }

  template<class T>
  bool checkGetParamsAs(const std::string& key, T& val, const T& def = T{}) {
    return checkGetAs(m_params, key, val, def);
  }

  template<class T>
  T getParamsAs(const std::string& key, const T& def = T{}) {
    return getAs(m_params, key, def);
  }

  template<class T>
  bool checkGetCookieAs(const std::string& key, T& val, const T& def = T{}) {
    return checkGetAs(m_cookie, key, val, def);
  }

  template<class T>
  T getCookieAs(const std::string& key, const T& def = T{}) {
    return getAs(m_cookie, key, def);
  }

  std::ostream& dump(std::ostream& os);
private:
  //获取m中key对应的val，并尝试将其转换为T类型，返回是否成功
  template<class T>
  bool checkGetAs(const MapType& m, const std::string& key, T& val, const T& def = T{}) {
    auto it = m.find(key);
    if(it == m.end()) {
      val = def;
      return false;
    }
    try {
        val = boost::lexical_cast<T>(it->second);
        return true;
    }catch(...) {
        val =def;
    }
    return false;
  }
  
  //获取m中的key对应的val，并尝试将其转换为T类型，成功则直接返回，失败则返回默认值
  template<class T>
  T getAs(const MapType& m, const std::string& key, const T& def = T{}) {
    auto it = m.find(key);
    if(it == m.end()) {
        return def;
    }
    try{
        return boost::lexical_cast<T>(it->second);
    }catch(...){
        //nothing
    }
    return def;
  }

private:
  HttpMethod m_method{HttpMethod::GET};
  HttpStatus m_status{HttpStatus::INVALID};
  uint8_t m_version{0};   //高4位为主版本， 低四位为小版本， 默认1.1 即0x11
  bool m_close{true};   //http支持长连接

  std::string m_path;
  std::string m_query;
  std::string m_fragment;
  std::string m_body;

  MapType m_headers;
  MapType m_params;
  MapType m_cookie;
};

class HttpResp {
public:
  using sptr = std::shared_ptr<HttpResp>;
  using MapType = std::map<std::string, std::string, CaseInsensitiveLess>;

  HttpResp(uint8_t version = 0x11, bool close = true);

  HttpStatus getStatus() const { return m_status; }
  uint8_t getVersion() const { return m_version; }
  const std::string& getReason() const { return m_reason; }
  const std::string& getBody() const { return m_body; }
  const MapType& getHeaders() const { return m_headers; }

  void setStatus(HttpStatus s) { m_status = s; }
  void setVersion(uint8_t v) { m_version = v; }
  void setReason(const std::string& v) { m_reason = v; }
  void setBody(const std::string& v) { m_body = v; }
  void setHeaders(const MapType& v) { m_headers = v; }

  bool isClose() const { return m_close; }
  void setClose(bool v) { m_close = v; }

  std::string getHeader(const std::string& key, const std::string& def = "");
  void setHeader(const std::string& key, const std::string& val);
  void delHeader(const std::string& key);

  template<class T>
  bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T{}) {
    return checkGetAs(m_headers, key, val, def);
  }

  template<class T>
  T getHeaderAs(const std::string& key, const T& def = T{}) {
    return getAs(m_headers, key, def);
  }

  std::ostream& dump(std::ostream& os);
private:
  template<class T>
  bool checkGetAs(const MapType& m, const std::string& key, T& val, const T& def = T{}) {
    auto it = m.find(key);
    if(it == m.end()) {
      val = def;
      return false;
    }
    try {
        val = boost::lexical_cast<T>(it->second);
        return true;
    }catch(...) {
        val =def;
    }
    return false;
  }
  
  //获取m中的key对应的val，并尝试将其转换为T类型，成功则直接返回，失败则返回默认值
  template<class T>
  T getAs(const MapType& m, const std::string& key, const T& def = T{}) {
    auto it = m.find(key);
    if(it == m.end()) {
        return def;
    }
    try{
        return boost::lexical_cast<T>(it->second);
    }catch(...){
        //nothing
    }
    return def;
  }

private:
  HttpStatus m_status {HttpStatus::INVALID};
  uint8_t m_version{0};   //高4位为主版本， 低四位为小版本， 默认1.1 即0x11
  bool m_close{true};   //http支持长连接

  std::string m_body;
  std::string m_reason;
  MapType m_headers;
};
} // namespace Http
} // namespace East

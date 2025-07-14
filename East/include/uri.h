/*
 * @Author: Xudong0722 
 * @Date: 2025-06-22 10:57:10 
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-06-22 11:15:27
 */

#pragma once

#include <memory>
#include <string>

#include "Address.h"
/*
    foo://example.com:8042/over/there?name=ferret#nose
    \_/   \______________/\_________/ \_________/ \__/
    |           |            |            |        |
    scheme     authority       path        query   fragment

    authority   = [ userinfo "@" ] host [ ":" port ]
*/

namespace East {
class Uri {
 public:
  using sptr = std::shared_ptr<Uri>;
  static Uri::sptr Create(const std::string& uri);
  Uri();

  const std::string& getScheme() const { return m_scheme; }
  const std::string& getUserinfo() const { return m_userinfo; }
  const std::string& getHost() const { return m_host; }
  const std::string& getQuery() const { return m_query; }
  const std::string& getFragment() const { return m_fragment; }

  void setPort(int32_t port) { m_port = port; }
  void setScheme(const std::string& scheme) { m_scheme = scheme; }
  void setUserinfo(const std::string& userinfo) { m_userinfo = userinfo; }
  void setHost(const std::string& host) { m_host = host; }
  void setPath(const std::string& path) { m_path = path; }
  void setQuery(const std::string& query) { m_query = query; }
  void setFragment(const std::string& fragment) { m_fragment = fragment; }

  int32_t getPort() const;
  const std::string& getPath() const;

  bool isDefaultPort() const;
  std::ostream& dump(std::ostream& os) const;
  std::string toString() const;
  Address::sptr createAddress() const;

 private:
  int32_t m_port{0};       // port
  std::string m_scheme;    // scheme
  std::string m_userinfo;  // userinfo
  std::string m_host;      // host
  std::string m_path;      // path
  std::string m_query;     // query
  std::string m_fragment;  // fragment
};
};  //namespace East
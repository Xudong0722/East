/*
 * @Author: Xudong0722
 * @Date: 2025-03-04 00:25:34
 * @Last Modified by: Xudong0722
 * @Last Modified time: 2025-03-04 23:41:38
 */

#pragma once
#include <memory>

namespace East {

template <class T, class V = void, int N = 0>
class Singleton {
 public:
  static T* GetInst() {
    static T t;
    return &t;
  }
};

template <class T, class V = void, int N = 0>
class SingletonPtr {
 public:
  static std::shared_ptr<T> GetInst() {
    static std::shared_ptr<T> t = std::make_shared<T>();
    return t;
  }
};
}  // namespace East
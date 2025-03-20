# East
A high performance Linux server.

## 开发环境
Linux
gcc 10.2.1
cmake

## 项目路径
bin -- 二进制文件
build -- 中间文件路径
cmake -- cmake函数文件夹
CMakeLists.txt -- cmake的定义文件
lib -- 库的输出路径
Makefile
East -- 源代码路径
     -- include 头文件
     -- src 源文件
tests -- 测试代码路径

## 日志系统
1)
Log4j
```cpp
    LoggerMgr(包含所有的Logger，默认有一个root)
        |
        |
    Logger（定义日志类别）
        |
        |---------LogFormatter(日志格式)
        |
    LogAppender（日志输出目标，std，file...）  ---- 目前有两种Appender: File appender, stdout appender. 后者主要用于测试

    LogEvent（定义日志事件的各种信息）
        |
        |
    LogEventWrap(LogEvent的包装器，在输出日志时，构造一个临时对象包含一个LogEvent(RAII), 在日志语句结束时，该对象析构时打印日志。支持流式输出)
```

Note: 每一个Logger默认有一个名称，并且支持配置，如formatter，appenders等。
Logger默认构造含有formatter和level，但是并没有默认的appender。
如果我们从LoggerMgr中获取一个不存在的Logger，会去创建一个Logger，如果使用这个Logger去输出日志，则会使用LoggerMgr中创建的root logger(默认formatter和StdoutAppender)。

## 配置系统

基于yaml实现配置系统，如日志格式，日志输出
基本原则：约定优于配置

依赖：yaml-cpp

```cpp
ConfigVarBase(抽象基类，提供配置项的接口) --> ConfigVar<T>(派生类，同时是模板类，因为配置项的属性可能不一样，整数/浮点数/字符串...， 支持增加监听回调，在值有变化的时候会触发)
                        |
                        |
                      Config(存储所有的配置项，并且对外提供了Lookup接口，同时支持从YML中读取更新配置)


Config核心方法:
static void LoadFromYML(const YAML::Node& root);     //从yml文件中读取配置， 并且更新内存中已经定义过的配置信息，并且ConfigVar<T>对象可以设置监听，在有更新时执行

template <class T>
static typename ConfigVar<T>::sptr Lookup( const std::string& name, const T& default_val, const std::string description = "");  //在本地寻找配置项，如果没有则根据实参新创建一个保存起来

目前支持基本的stl容器：vector, list, set, unordered_set, map, unordered_map
    如果是自定义类型，需要特化LexicalCast以支持自定义类和字符串之间的相互转换。

```

1. 目前已经支持日志系统的配置解析及更新

## 协程库开发 
# East
A high performance Linux server.

## 开发环境
Linux
gcc 10.2.1
camke

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
```
    Logger（定义日志类别）
        |
        |---------LogFormatter(日志格式)
        |
    LogAppender（日志输出目标，std，file...）

    LogEvent（定义日志事件的各种信息）
```

## 配置系统

基于yaml实现配置系统，如日志格式，日志输出
基本原则：约定优于配置

依赖：yaml-cpp
Type:
Undefined = 0,
Null = 1,
Scalar = 2,
Sequence = 3,
Map = 4

```
ConfigVarBase(抽象基类，提供配置项的接口) --> ConfigVar<T> (派生类，同时是模板类，因为配置项的属性可能不一样，整数/浮点数/字符串...)
                        |
                        |
                      Config(存储所有的配置项，并且对外提供了Loopup接口，同时支持从YML中读取更新配置)

目前支持基本的stl容器：vector, list, set, unordered_set, map, unordered_map
```
## 协程库开发 
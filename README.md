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

## 协程库开发 
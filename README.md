# MySylar

## 日志系统

1. 

```c
	仿照Log4j
 
	Logger(定义日志类别)
		|
		|-------Formatter(日志格式)
		|
	Appender(日志输出的地方)
```

## 配置系统

Config --> Yaml

安装yaml-cpp库

约定优于配置, 这样即使有上百个配置信息, 因为大多数都有约定, 可以只配置那些和默认值不同的, 可以大大减少配置的工作

```c
template<T, FromStr, ToStr>
class ConfigVar;

template<F, T>
LexicalCast;

//容器片特化, 目前支持vector
//list, set, map, unordered_map, unordered_set
```

自定义类型必须实现 sylar::LexicalCast,偏特化 实现后,自定义类型可以和以上指定的容器一起使用

配置的事件机制 当一个配置项发生修改, 可以反向通知对应的代码, 回调

## 日志系统和配置系统整合

```c
logs:
    - name: root
      level: (debug, info, warn, errot, fatal)
      formatter: '%d%T%p%T%t%m%n'
      appender:
            -type : (StdoutLogAppener, FileAppender)
             level:(debug,...)
             file: /logs/xxx.log
    syalr::Logger g_logger = 
    sylar::LoggerMgr::GetInstance()->getLogger(name);
    SYLAR_LOG_INFO(g_logger) << "XXXX log";
static Logger::ptr g_log = SYLAR_LOG_NAME("system");
//m_root, m_system->m_root
//当logger的appenders为空, 使用root写logger
//定义LogDefine 和 LogAppenderDefine, 片特化LexocalCast
//实现日志配置解析
```

## 将输出绝对路径改为相对路径

在cmake中重定义__FILE__

## 总结

使用ProcessOn 画类图

## 线程库

Thread, Mutex

pthread_create

互斥量 mutex 信号量 semaphore

线程日志模块整合 自旋锁

## 协程库的封装

定义协程接口 准备工作 assert backtrace 在assert时打印栈

```c
协程:
Thread->main_fiber <------>sub_fiber
            ^   
            |
            |
            v
        idle_fiber
```

协程调度模块scheduler

```c
          1-N        1:M
scheduler --> thread --->fiber
1.线程池, 分配一组线程
2.协程调度器, 将协程指定到对应的线程上去执行

m_threads线程池
m_fibers协程队列 装的可以是函数 也可以是协程

核心函数
start()
stop()
run()
IOManager(epoll) ---> Scheduler
    |
    |
    v
   idle(epoll_wait)
   
异步IO 
使用pipe往写入端写入消息, 唤醒阻塞在epoll_wait的线程
Timer -> addTimer() -->cancel()
获取当前的定时器触发离现在的时间差
返回当前需要触发的定时器
            [Fiber]                  [Timer]
               ^N                       ^
               |                        |
               |1                       |
            [Thread]              [Timermanager]
               ^M                       ^
               |                        |
               |1                       |        
            [Scheduler] <------ [IOManager(epoll)]
               
```

## HOOK

sleep usleep

socket 相关的(socket, connect, accept) io相关 (read, write, send ,recv, ...) fd相关 (fcntl, setsockopt, ...)

## socket系列函数库开发

```c
[UnixAddress]   
    |
-----------                   |-[IPv4Address]
| Address | --- [IPAddress] --|
-----------                   |-[IPv6Address]
    |    
-----------
|  Socket |
```

accept
conncet close
read/write/close

## 序列化 bytearray

write(int, float, int64_t ...) read(int, float, int64_t ...)

## http协议开发

HTTP/1.1 - API

HttpRequest; HttpResponse;

GET / HTTP/1.1 Host: [www.baidu.com](http://www.baidu.com/)

```
uri: http:://www.sylar.top:80/page/xxx?id=10&v=20#fr
    http 协议
    www.sylar.top 主机(host) 
    80 端口
    /page/xxx 路径(path)
    id=10&v=20 param
    fr fragment
```

## http 解析器

使用开源http-parser ragel编译有限状态机

## stream 针对文件/socket封装

read/write/readFixSize/writeFixSize

## HttpSession/HttoConnection

Server.accept产生的socket -> session client.connect的socket -> connection

HttpServer : TcpServer

```
    Servlet <---- FunctionServlet
        |
    ServletDispatch
```

http调试工具, Postman

## 分布式协议库

## 用框架写一个推荐系统
# tinyhttpd


tinyhttpd is a minimum functional HTTP Server

### Compile and run：

```cmake
make
./tinyhttpd &
```
The default port is 8008.
Or
```cmake
make
 ./tinyhttpd port &
```

### Logs：

| Time         | Log           | 
| :------------- |:-------------|
| 2016/08/22 01:41:34 | `tinyhttpd`仅支持`GET`请求。<br> 对于每个`HTTP`请求，`fork`一个进程去处理。 |
| 2016/08/28 02:21:30 | 对于每个请求，不再由单独的进程来处理，而是由同一个进程的多个线程来处理。<br> 为了防止僵尸线程`(Zombie Threads)`，创建不需要返回的线程，称之为**独立线程**。 <br> ~~待解决：修复独立线程返回后主线程也随之退出的问题。~~|
| 2016/08/29 00:01:35 | 修复子线程执行完毕后导致主线程也随之退出的`bug`。 <br> 重新实现`do_ls`，使用`popen`读取`ls`程序的输出。|
| 2016/08/30 01:53:08 | 增加配置文件，设置默认端口。|
| 2016/09/04 00:53:57 | 增加对`CGI`的支持。|
| 2016/09/09 00:41:28 | 增加`http_header_parser`，将解析`HTTP`请求参数独立出来，以便后续增加对`Post`、`Put`等方法的支持。<br> 完善注释。|
| 2016/09/15 20:49:05 | 在线程中调用`fork`，创建新的进程来运行`CGI`程序（存在隐患）。|
| 2016/09/22 01:20:16 | 阅读 `CSAPP RIO(Robust I/O)`源码，`RIO`提供了方便、健壮和高效的`I/O`。 <br>  在`tinyhttpd`中引入`RIO`。|
| 2016/09/25 22:26:10 | 增加对资源访问的权限判定、可执行和可读校验。 <br> 完善`I/O`读取边界处理。<br>  调整代码结构。|
| 2016/10/04 16:39:52 | 使用`opendir`和`readdir`重写`do_ls`，避免`popen`执行不安全的命令。|
| 2016/10/09 23:20:30 | 扩展`tinyhttpd`对动态请求的支持，`tinyhttpd`支持带有请求参数的`GET`请求，`setenv`将参数设置为当前进程的环境变量，`CGI`程序读取环境变量。|
| 2016/10/11 00:50:51 | `tinyhttpd`支持`POST`请求。|

### More

* static resources (http://192.168.199.119:8008/doc/index.html)


![Alt Text](https://github.com/tinylcy/tinyhttpd/raw/master/doc/static.png)


* dynamic resources(CGI) (http://192.168.199.119:8008/cgi-bin/myscript.sh)

![Alt Text](https://github.com/tinylcy/tinyhttpd/raw/master/doc/dynamic.png)

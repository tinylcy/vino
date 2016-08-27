tinyhttpd
===

`tinyhttpd`是一个超轻量级的 `HTTP Server`

## how to use

```cmake
make
./tinyhttpd port &
```

-------
更新于`2016/8/28 02:21:30`

* `tinyhttpd(version 1)`仅支持`GET`请求。
* 对于每个`HTTP`请求，`fork`一个进程去处理。
* `tinyhttpd(version 2)`依旧只支持`GET`请求。
* 对于每个请求，不再由单独的进程来处理，而是由同一个进程的多个线程来处理。
* 为了防止僵尸线程`(Zombie Threads)`，创建不需要返回的线程，称之为**独立线程**。

######待解决：修复bug(解决独立线程返回后主线程也跟着退出的问题)。




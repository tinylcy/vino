# tinyhttpd


tinyhttpd is a minimum functional HTTP server written in the C programming language.

## Compile and run

For now tinyhttpd only support Linux 2.6 or later.

Please make sure you have [cmake](https://cmake.org) installed.

```cmake
mkdir build && cd build
cmake .. && make
cd .. && ./build/tinyhttpd &
```

By default the server runs on port 8008, if you want to assign other port for the server, run it as `./build/tinyhttpd port &`.


## Programming model

- [x] epoll
- [x] non-blocking I/O
- [x] threadpool

## More

* STATIC RESOURCES (http://host:port/doc/index.html)


![Alt Text](https://github.com/tinylcy/tinyhttpd/raw/master/doc/static.png)


* DYNAMIC RESOURCES(CGI) (http://host:port/cgi-bin/myscript.sh)

![Alt Text](https://github.com/tinylcy/tinyhttpd/raw/master/doc/dynamic.png)

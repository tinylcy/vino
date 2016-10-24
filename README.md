# tinyhttpd


tinyhttpd is a minimum functional HTTP server written in the C programming language.

## Compile and run

On Linux or on Mac OS X simply use Makefile to compile the server.

To run the server type `./tinyhttpd &` into a terminal that is in the directory where the executable file is located.

By default the server runs on port 8008, if you want to assign other port for the server, run it as `./tinyhttpd port &`.


## Programming modal

-[] epoll
-[] non-blocking I/O
-[x] threadpool

## More

* STATIC RESOURCES (http://your ip:your port/doc/index.html)


![Alt Text](https://github.com/tinylcy/tinyhttpd/raw/master/doc/static.png)


* DYNAMIC RESOURCES(CGI) (http://your ip:your port/cgi-bin/myscript.sh)

![Alt Text](https://github.com/tinylcy/tinyhttpd/raw/master/doc/dynamic.png)

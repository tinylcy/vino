# tinyhttpd


tinyhttpd is a minimum functional HTTP server written in the C programming language.

## Compile and run

On Linux or on Mac OS X simply use Makefile to compile the server.

To run the server type `./tinyhttpd &` into a terminal that is in the directory where the executable file is located.

By default the server runs on port 8008, if you want to assign other port for the server, run it as `./tinyhttpd port &`.

## Logs

* Mon Aug 22 01:42:21 2016: tinyhttpd only supports HTTP GET request, for each HTTP request，tinyhttpd fork a new process to handle it.

* Sun Aug 28 02:27:19 2016: Each dynamic request are still handled by a process, but other requests are handled by the same process of multiple threads to deal with. To avoid the Zombie Threads, tinyhttpd creates the threads which don't need to return.

* Mon Aug 29 00:02:55 2016: tinyhttpd reads the standard output of the execution of command 'ls' by popen, which helps to implement do_ls.

* Tue Aug 30 02:00:47 2016: Adding the configuration file which only contains the default port number at present.

* Sun Sep 4 00:58:56 2016: Adding the support for CGI program.

* Fri Sep 9 00:55:37 2016: Making the process of parsing the HTTP request headers as an independent work in order to better support more HTTP methods, like POST.

* Thu Sep 22 01:20:36 2016: Reading the source code of RIO(Robust I/O) in CSAPP, RIO achieves the convenient, robust and efficient I/O.

* Sun Sep 25 22:31:34 2016: Before accessing the static resources or executing the CGI programs, tinyhttpd will examine the corresponding permissions.

* Tue Oct 4 16:58:00 2016: Due to the risk of executing dangerous commands by popen, tinyhttpd reimplements the do_ls by opendir and readdir.

* Sun Oct 9 23:30:23 2016: Expanding the support for dynamic requests. tinyhttpd supports the HTTP GET requests with parameters, the parameters are passed by setting the environment variables and CGI programs fetch these parameters.

* Tue Oct 11 01:11:15 2016: tinyhttpd now supports the HTTP POST.



## More

* STATIC RESOURCES (http://your ip:your port/doc/index.html)


![Alt Text](https://github.com/tinylcy/tinyhttpd/raw/master/doc/static.png)


* DYNAMIC RESOURCES(CGI) (http://your ip:your port/cgi-bin/myscript.sh)

![Alt Text](https://github.com/tinylcy/tinyhttpd/raw/master/doc/dynamic.png)

# Vino

Vino is a lightweight and efficient web server written in the C programming language.

## Compile and run

For now Vino only support Linux 2.6 or later.

Please make sure you have [cmake](https://cmake.org) installed.

```cmake
mkdir build && cd build
cmake .. && make
./vino
```

If you want to build for debug and print debug information when compiling, use `cmake -DCMAKE_BUILD_TYPE=Debug <path>`. If you want to enable profiling, you can use `cmake -DCMAKE_C_FLAGS=-pg <path>`.

By default the server accepts connections on port 8080, if you want to assign other port for the server, run it as `./vino -p|--port <port>`. For more options, please type `./vino -h|--help` into terminal.

## Features

- [x] Single-threaded, non-blocking I/O based on event-driven model
- [x] HTTP persistent connection (HTTP Keep-Alive)
- [x] A timer for executing the handler after having waited the specified number of milliseconds
- [x] A parser for extracting request line and headers from HTTP request message
- [x] A unified memory pool
- [x] HTTP GET method

## Performance 

I have made a small performance test with [Nginx](https://nginx.org/en/), a high-performance web server and reverse proxy (I have learned a lot from it). In a way it is not a fair comparison, for the processing logic in Vino is relatively simpler than Nginx.

```markdown
ab -n 10000 -c 500 http://<IP>:<PORT>/index.html
```

```markdown
cpu: Intel(R) Xeon(R) CPU L5630  @ 2.13GHz, 8 cores.
mem: 32GB.
```

**Nginx**

```markdown
Server Software:        nginx/1.0.14
Server Hostname:        scut.xxxx.com
Server Port:            80

Document Path:          /index.html
Document Length:        151 bytes

Concurrency Level:      500
Time taken for tests:   5.193 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      3620000 bytes
HTML transferred:       1510000 bytes
Requests per second:    1925.85 [#/sec] (mean)
Time per request:       259.626 [ms] (mean)
Time per request:       0.519 [ms] (mean, across all concurrent requests)
Transfer rate:          680.82 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        7   93 292.0     20    2281
Processing:     4  104 257.5     20    4206
Waiting:        4  100 255.2     20    4206
Total:         20  197 416.3     41    4250

Percentage of the requests served within a certain time (ms)
  50%     41
  66%     47
  75%     57
  80%    250
  90%    673
  95%   1053
  98%   1504
  99%   2158
 100%   4250 (longest request)
 
```

**Vino**

```markdown

Server Software:        Vino
Server Hostname:        scut.xxxx.com
Server Port:            8080

Document Path:          /index.html
Document Length:        151 bytes

Concurrency Level:      500
Time taken for tests:   4.855 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      2490000 bytes
HTML transferred:       1510000 bytes
Requests per second:    2059.81 [#/sec] (mean)
Time per request:       242.741 [ms] (mean)
Time per request:       0.485 [ms] (mean, across all concurrent requests)
Transfer rate:          500.87 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        6  108 287.3     21    2305
Processing:     3   91 180.7     21    1987
Waiting:        3   87 178.3     20    1987
Total:         15  199 360.5     43    3165

Percentage of the requests served within a certain time (ms)
  50%     43
  66%     51
  75%    200
  80%    269
  90%    724
  95%   1060
  98%   1289
  99%   1644
 100%   3165 (longest request)

```
## TODO

- [ ] FastCGI
- [ ] HTTP POST and other methods
- [ ] More HTTP/1.1 features
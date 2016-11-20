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

## Performance 

```markdown
ab -n 10000 -c 500 http://127.0.1.1:8008/doc/
```

```markdown
cpu: Intel(R) Xeon(R) CPU           L5630  @ 2.13GHz, 4 cores.
mem: 32GB.
```

```markdown
Server Software:
Server Hostname:        127.0.1.1
Server Port:            8008

Document Path:          /doc/
Document Length:        68 bytes

Concurrency Level:      500
Time taken for tests:   2.604 seconds
Complete requests:      10000
Failed requests:        0
Total transferred:      1130000 bytes
HTML transferred:       680000 bytes
Requests per second:    3840.26 [#/sec] (mean)
Time per request:       130.199 [ms] (mean)
Time per request:       0.260 [ms] (mean, across all concurrent requests)
Transfer rate:          423.78 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0   45 205.3      0    1005
Processing:     8   35  14.5     32     233
Waiting:        8   35  14.5     32     233
Total:         27   80 207.4     32    1229

Percentage of the requests served within a certain time (ms)
  50%     32
  66%     33
  75%     34
  80%     34
  90%     45
  95%    108
  98%   1040
  99%   1045
 100%   1229 (longest request)
 
```


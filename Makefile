
tinyhttpd: tinyhttpd.c
	gcc -W -Wall tinyhttpd.c socklib.c http_headers_parser.c rio.c util.c -lpthread -o tinyhttpd

clean:
	rm tinyhttpd

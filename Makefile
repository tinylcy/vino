
tinyhttpd: tinyhttpd.c
	gcc -W -Wall tinyhttpd.c socklib.c -lpthread -o tinyhttpd

clean:
	rm tinyhttpd

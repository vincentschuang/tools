CC:=gcc
exe:=Web
obj:=web.o  tcpsocket.o http.o add_openssl.o http_jet.o

all:$(obj)
	$(CC) -o $(exe) $(obj) -pthread -I/usr/local/ssl/include -L/usr/local/ssl/lib/ -lssl -lcrypto -ldl
%.o:%.c
	$(CC) -c $^ -o $@ -I/usr/local/ssl/include

.PHONY:clean
clean:
	rm -rf $(obj) $(exe)

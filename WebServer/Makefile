CC:=gcc
exe:=Web
obj:=web.o  tcpsocket.o http.o

all:$(obj)
	$(CC) -o $(exe) $(obj) -pthread
%.o:%.c
	$(CC) -c $^ -o $@ 

.PHONY:clean
clean:
	rm -rf $(obj) $(exe)

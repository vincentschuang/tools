CC:=gcc
exe:=Speed
obj:=speed.c

all:$(obj)
	$(CC) -o $(exe) $(obj)
%.o:%.c
	$(CC) -c $^ -o $@

.PHONY:clean
clean:
	rm -rf $(obj) $(exe)


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "web.h"
#include "http.h"
#include "tcpsocket.h"


int TcpSocketCreate(){
	int fd;

	fd = socket(AF_INET,SOCK_STREAM,0);
	if(fd < 0){
		printf("Fail to Create Socket\n");
		return -1;
	}

	struct sockaddr_in serverAddress;
	memset(&serverAddress,0,sizeof(struct sockaddr_in));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(LISTEN_PORT);

	int optionReuseAddr = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &optionReuseAddr, sizeof(int));

	if(bind(fd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) >= 0){
		return fd;
	}else{
		printf("Fail to bind\n");
		close(fd);
		return -1;
	}
}

int SocketAccept(int fd){
	int newfd;

	newfd = accept(fd, NULL, NULL);
}


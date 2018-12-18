#ifndef __HTTP_H__
#define __HTTP_H__

#include "web.h"


#define BUFFER_SIZE 2048
#define PATH_LEN 128
#define BODY_BUFFER_LEN 655360
#define HEADER_LEN 512
#define WAITING_TIME 1000*000 //micro second

#define HOME_PAGE "index.html"

typedef struct httpRequest httpRequest_T;
struct httpRequest{
	char * method;
	char * action;
	int contentLength;
	char * payload;
	int newfd;
};

typedef struct fileType  fileType_T;
struct fileType{
    char *ext;
    char *fileType;
};

static const char getResponseHeader[] = ""
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zd\r\n"
        "Connection: close\r\n"
        "Server: Server/1.4.35\r\n"
        "\r\n";


void* httpHandler(void * data);
char * getMethod(char * buff, int *position);
char * getAction(char * buff, int *position);
int getContentLength(char * buff, int * position);

void getHandler(httpRequest_T * httpRequest);

#endif
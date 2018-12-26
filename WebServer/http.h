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
#ifdef USE_OPENSSL
    SSL *ssl;
#endif    
};

typedef struct fileType  fileType_T;
struct fileType{
    char *ext;
    char *fileType;
};

static const char getResponseHeader[] = ""
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "Server: Server/1.4.35\r\n"
        "\r\n";

static const char jetResponseHeader[] = ""
        "HTTP/1.1 200 OK\r\n"
        "Server: Apache-Coyote/1.1\r\n"
        "Content-Type: multipart/mixed;boundary=\"BOUNDARY\"\r\n"
        "Content-Length: %d\r\n"
        "Set-Cookie: JSESSIONID=C95F5C5D753E2354962F0EDB1A569BF0; Path=/saiene\r\n"
        "Connection: close\r\n"
        "Content-Language: ja\r\n"
        "\r\n";

static const char jetPayloadHeader[] = ""
        "--BOUNDARY\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Type: multipart/mixed;boundary=\"BOUNDARY\"\r\n"
        "Content-Disposition: attachment; filename=%s\r\n"
        "Content-Length: %d\r\n"
        "\r\n";

static const char jetPayloadTail[] = ""
        "\r\n--BOUNDARY--";

void* httpHandler(void * data);
char * getMethod(char * buff, int *position);
char * getAction(char * buff, int *position);
int getContentLength(char * buff, int * position);
void postHandler(httpRequest_T * httpRequest);
void getHandler(httpRequest_T * httpRequest);
void parseItemFromPayload(char *item,char *payload, char *ret );
int getFileSize(char *path);
void readFileToBuffer(char * file, char *buffer, int num );
void SendJetResponsePacket(char*fileFullPath, char*fileName, char* headBuffer,char* payloadBuffer, httpRequest_T * httpRequest);

#endif
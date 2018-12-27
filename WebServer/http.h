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

typedef struct postFunction  postFunction_T;
struct postFunction{
    char *cgiName;
    int len;
    void (*funcPtr)(httpRequest_T * );
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

static const char response404[] =""
    "HTTP/1.1 404 Not Found\r\n"
    "Server: Apache-Coyote/1.1\r\n"
    "Vary: Accept-Encoding\r\n"
    "Content-Length: 284\r\n"
    "Content-Type: text/html; charset=iso-8859-1\r\n"
    "\r\n"
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">"
    "<html><head>"
    "<title>404 Not Found</title>"
    "</head><body>"
    "<h1>Not Found</h1>"
    "<p>The requested URL /hello was not found on this server.</p>"
    "<hr>"
    "<address>Apache/2.2.22 (Debian) Server at 52.200.39.81 Port 8080</address>"
    "</body></html>";


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
void send404(httpRequest_T * httpRequest);


#endif
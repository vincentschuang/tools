
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#include "http.h"
#include "add_openssl.h"
#include "http_jet.h"

//only for small file
int getFileSize(char *filename){
	struct stat st;
	stat(filename, &st);
	 return (int)st.st_size;
}

char* freadFileToBuffer(char * file, int num ){
	char * buffer;
	buffer = (char *)malloc(sizeof(char)*num);

	FILE *fp;
	size_t result;
    fp = fopen (file, "r");
    result = fread(buffer,sizeof(char),num,fp);

    //printf("result=%d\n", result);

    if(result != num){
    	printf("READ Error\n");
    }
    fclose(fp);

    return buffer;
}

char * getMethod(char * buff, int * position){
	char * ptr = NULL;
	if( !strncmp(buff,"GET",3) ){
		ptr = buff;
		buff[3] = 0;
		*position = 4;
	}else if( !strncmp(buff,"POST",4) ){
		ptr = buff;
		buff[4] = 0;
		*position = 5;
	}

	return ptr;
}

char * getAction(char * buff, int * position){
	char * ptr = NULL;
	int i = 0 ;
	
	for(i=0; i<BUFFER_SIZE; i++){
		if(buff[i] == ' '){
			ptr = buff;
			buff[i] = 0;
			*position += i+1;
			break;
		}
	}

	return ptr;
}

int getContentLength(char * buff, int * position){
	char * ptr = NULL;
	//printf("buff=%s\n", buff);
	ptr = strstr(buff,"Content-Length:");
	if(ptr){
		ptr += strlen("Content-Length:");
		*position += (ptr - buff);
		return atoi(ptr);
	}else{
		return 0;
	}
}

char * getPayload(char * buff){
	char * ptr = NULL;
	ptr = strstr(buff,"\r\n\r\n");
	return ptr+4;
}

fileType_T extensions [] = {
    {"jpg", "image/jpeg"},
    {"jpeg","image/jpeg"},
    {"png", "image/png" },
    {"ico", "image/vnd.microsoft.icon" },
    {"js", "text/javascript;"},
    {"htm", "text/html" },
    {"html","text/html" },
    {"json","application/json"},
    {"css","text/css" },
    {"exe","text/plain" },
    {"gif", "image/gif" },
    {"zip", "image/zip" },
    {"gz",  "image/gz"  },
    {"tar", "image/tar" },
    {0,0} 
};

//todo: use return to indicate success or fail
void parseItemFromPayload(char *item,char *payload, char *ret){
	char * h;
	h = strstr(payload,item);
	if(h == NULL){
		printf("NULL\n");
		return ;
	}
	
	int i, s, len;
	for(i=0;  ; i++){
		if(h[i] == '='){
			s = i+1;
		}else if(h[i] == '&'  ){
			len = i-s;
			memcpy(ret,&h[s],len);
			ret[len]='\0';
			break;
		}else if(h[i] == '\0'){
			len = i-s;
			memcpy(ret,&h[s],len);
			ret[len]='\0';
			break;
		}

	}
}



void send404AndCloseSocket(httpRequest_T * httpRequest){
#ifdef USE_OPENSSL	
		SSL_write(httpRequest->ssl, response404, strlen(response404));
		SSL_free(httpRequest->ssl);
#else
		write(httpRequest->newfd, response404, strlen(response404));
#endif
	close(httpRequest->newfd);
}

//array of function pointer
postFunction_T postFunctions[]={
	{"/scheduleSend", 13, scheduleSend},
	{"/scheduleConfig", 15, scheduleConfig},
	{"/getScheduleConfig", 18, getScheduleConfig},
	{"0", 0, NULL}
};

void postHandler(httpRequest_T * httpRequest){
	printf("\n\n\n~~~~~~~~~~~postHandler~~~~~~~~~\n");
	
	printf("%s\n", httpRequest->action);
	printf("%d\n", httpRequest->contentLength);
	printf("%s\n", httpRequest->payload);
	
	int i=0;
	while(postFunctions[i].funcPtr != NULL ){
		if(!strncmp(httpRequest->action, postFunctions[i].cgiName, postFunctions[i].len)){
			postFunctions[i].funcPtr(httpRequest);
			break;
		}
		i++;
	}

	if(postFunctions[i].funcPtr == NULL){
		send404AndCloseSocket(httpRequest);
		return;
	}

#ifdef USE_OPENSSL	
	SSL_free(httpRequest->ssl);
#endif	
	close(httpRequest->newfd);

}

void getHandler(httpRequest_T * httpRequest){
	char header[HEADER_LEN];
	memset(header,0,HEADER_LEN);

	char * body;

	char * fileType=NULL;

	int file_fd, ret, len=0, i;

	char path[PATH_LEN];
	memset(path,0,PATH_LEN);
	strcat(path, WWW_FOLDER);
	strcat(path, httpRequest->action);


	if( !strcmp(httpRequest->action,"/") ){
		strcat(path, HOME_PAGE);
	}else{
		int actionLen = strlen(httpRequest->action);
		int extLen = 0;
		for(i=0; extensions[i].ext; i++){
			extLen = strlen(extensions[i].ext);
			if(!strncmp(&httpRequest->action[actionLen-extLen], extensions[i].ext, extLen)){
				fileType = extensions[i].fileType;
			}
		}
		if(fileType == NULL){
			printf("Not Allow Extensions\n");
			send404AndCloseSocket(httpRequest);
			return;
		}
	}


	if(access( path, R_OK) == -1){
		printf("File not found\n");
		send404AndCloseSocket(httpRequest);
		return;
	}

	int fileSize = getFileSize(path);
	body = freadFileToBuffer(path, fileSize );
	if(!body){
		printf("Read File Fail\n");
	}

    sprintf(header,getResponseHeader,fileType ,fileSize);

#ifdef USE_OPENSSL
    SSL_write(httpRequest->ssl, header, strlen(header));
    SSL_write(httpRequest->ssl, body, fileSize);
    SSL_free(httpRequest->ssl);
#else    
	write(httpRequest->newfd, header, strlen(header));
	write(httpRequest->newfd, body, fileSize);
#endif
	free(body);
	close(httpRequest->newfd);
}

void handleRequest(int newfd){
	httpRequest_T * httpRequest = (httpRequest_T *)malloc(sizeof(httpRequest_T));
	httpRequest->contentLength = 0;
	httpRequest->newfd = newfd;
#ifdef USE_OPENSSL
	httpRequest->ssl = SSL_new(ctx);
	SSL_set_fd(httpRequest->ssl, httpRequest->newfd);
#endif	

	char buffer[BUFFER_SIZE];
	bzero(buffer, BUFFER_SIZE);

	int position = 0, recvCount = 0;
#ifdef USE_OPENSSL
	if (SSL_accept(httpRequest->ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(httpRequest->ssl);
		close(newfd);
        return;
    }else{
    	recvCount = SSL_read(httpRequest->ssl, buffer, BUFFER_SIZE);
    }
#else
	recvCount = recv(newfd, buffer, BUFFER_SIZE, 0);
#endif
	if(recvCount <= 0){
		//printf("recv error\n");
#ifdef USE_OPENSSL		
		SSL_free(httpRequest->ssl);
#endif
		close(newfd);
		return;
	}

	httpRequest->method = getMethod(buffer, &position);
	if(!httpRequest->method){
		printf("NOT found this Method\n");
#ifdef USE_OPENSSL		
		SSL_free(httpRequest->ssl);
#endif
		close(newfd);		
		return;
	}

	httpRequest->action = getAction(buffer+position, &position);
	if(!httpRequest->action){
		printf("No action\n" );
#ifdef USE_OPENSSL		
		SSL_free(httpRequest->ssl);
#endif
		close(newfd);
		return;
	}

	if(!strncmp(buffer,"POST ",4) ){
		httpRequest->contentLength = getContentLength(buffer+position, &position);
		httpRequest->payload = getPayload(buffer+position);

		//somtimes post packet is big
		//need to recv more times
		int retry = 5;
		while( (&buffer[recvCount]-httpRequest->payload) <  httpRequest->contentLength && retry--){
			printf("retry=%d\n",retry );
#ifdef USE_OPENSSL			
			recvCount += SSL_read(httpRequest->ssl, &buffer[recvCount], BUFFER_SIZE-recvCount);
#else
			recvCount += recv(newfd, &buffer[recvCount], BUFFER_SIZE-recvCount, 0);
#endif			
		}

		if( (&buffer[recvCount]-httpRequest->payload) <  httpRequest->contentLength ){
			printf("Fail to recv all content\n");
#ifdef USE_OPENSSL		
			SSL_free(httpRequest->ssl);
#endif
			close(newfd);
			return;
		}else{
			postHandler(httpRequest);
		}
	}else if(!strncmp(httpRequest->method,"GET",3)){
		getHandler(httpRequest);
	}else{
		printf("NOT Support this Method\n");
	}

	free(httpRequest);
}

void* httpHandler(void * data){

	taskQueue_T* taskQueue = (taskQueue_T*) data;
	LIST_ENTRY_T * task;
	int newfd;

	while(1){
		pthread_mutex_lock(&taskQueue->mutex);	

		while (taskQueue->tasks == 0) {
            pthread_cond_wait(&taskQueue->workAvailable, &taskQueue->mutex);
        }

        //printf("taskQueue->tasks=%d\n", taskQueue->tasks);
		assert(taskQueue->tasks > 0);
		
		task = STAILQ_FIRST(&taskQueue->list_head);
		newfd = task->newfd;
		STAILQ_REMOVE_HEAD(&taskQueue->list_head, next);
		free(task);
		taskQueue->tasks--;
		pthread_mutex_unlock(&taskQueue->mutex);

		//printf("\n\nthread id=%d\n", (int)pthread_self()&0x1111);
		handleRequest(newfd);	
		
	}
}

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
#include "parsebinary.h"
#include "add_openssl.h"
#include "http_jet.h"

void freadFileToBuffer(char * file, char *buffer, int num ){
	FILE *fp;
    fp = fopen (file, "r");
    fread(buffer,sizeof(char),num,fp);
    fclose(fp);
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

void SendJetResponsePacket(char*fileFullPath, char*fileName, char* headBuffer,char* payloadBuffer, httpRequest_T * httpRequest){
	//generate return packet
			
	//get File
	int fileSize = getFileSize(fileFullPath);
	char *fileBuffer = (char *)malloc( sizeof(char)*fileSize );
	freadFileToBuffer(fileFullPath, fileBuffer, fileSize );

	int tailSize = strlen(jetPayloadTail);
	//write payload head
	sprintf(payloadBuffer,jetPayloadHeader, fileName, fileSize );
	int headLen = strlen(payloadBuffer);
	//write file
	memcpy(&payloadBuffer[headLen], fileBuffer, fileSize);
	free(fileBuffer);
	//write tail
	memcpy(&payloadBuffer[headLen+fileSize],jetPayloadTail,tailSize);
	//write head
	sprintf(headBuffer,jetResponseHeader,headLen+fileSize+tailSize);
#ifdef USE_OPENSSL	
	SSL_write(httpRequest->ssl, headBuffer, strlen(headBuffer));
    SSL_write(httpRequest->ssl, payloadBuffer, headLen+fileSize+tailSize);
#else
	write(httpRequest->newfd, headBuffer, strlen(headBuffer));
	write(httpRequest->newfd, payloadBuffer, headLen+fileSize+tailSize);
#endif

}

void send404(httpRequest_T * httpRequest){
#ifdef USE_OPENSSL	
		SSL_write(httpRequest->ssl, response404, strlen(response404));
		SSL_free(httpRequest->ssl);
#else
		write(httpRequest->newfd, response404, strlen(response404));
#endif
	close(httpRequest->newfd);
}

void postHandler(httpRequest_T * httpRequest){
	printf("\n\n\n~~~~~~~~~~~postHandler~~~~~~~~~\n");
	
	printf("%s\n", httpRequest->action);
	printf("%d\n", httpRequest->contentLength);
	printf("%s\n", httpRequest->payload);
	
//array of function pointer
	postFunction_T postFunctions[]={
		{"/scheduleSend", 13, scheduleSend},
		{"/scheduleConfig", 15, scheduleConfig},
		{"/getScheduleConfig", 18, getScheduleConfig},
		{"0", 0, NULL}
	};

	int i=0;
	while(postFunctions[i].funcPtr != NULL ){
		if(!strncmp(httpRequest->action, postFunctions[i].cgiName, postFunctions[i].len)){
			postFunctions[i].funcPtr(httpRequest);
			break;
		}
		i++;
	}

	if(postFunctions[i].funcPtr == NULL){
		send404(httpRequest);
		return;
	}

#ifdef USE_OPENSSL	
	SSL_free(httpRequest->ssl);
#endif	
	close(httpRequest->newfd);

}

void getHandler(httpRequest_T * httpRequest){
	char body[BODY_BUFFER_LEN], header[HEADER_LEN];
	memset(body,0,BODY_BUFFER_LEN);
	memset(header,0,HEADER_LEN);

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
			send404(httpRequest);
			return;
		}
	}

	//security check
	if(strstr(httpRequest->action,"..")){
		printf("SECURITY ISSUE\n");
		send404(httpRequest);
		return;
	}

	if(access( path, R_OK) == -1){
		printf("File not found\n");
		send404(httpRequest);
		return;
	}

	int fileSize = getFileSize(path);
	freadFileToBuffer(path, body, fileSize );

    sprintf(header,getResponseHeader,fileType ,fileSize);
#ifdef USE_OPENSSL
    SSL_write(httpRequest->ssl, header, strlen(header));
    SSL_write(httpRequest->ssl, body, fileSize);
    SSL_free(httpRequest->ssl);
#else    
	write(httpRequest->newfd, header, strlen(header));
	write(httpRequest->newfd, body, fileSize);
#endif

	close(httpRequest->newfd);
}

void handleRequest(int newfd){
	httpRequest_T httpRequest;
	httpRequest.contentLength = 0;
	httpRequest.newfd = newfd;
#ifdef USE_OPENSSL
	httpRequest.ssl = SSL_new(ctx);
	SSL_set_fd(httpRequest.ssl, httpRequest.newfd);
#endif	


	char buffer[BUFFER_SIZE];
	bzero(buffer, BUFFER_SIZE);

	int position = 0, recvCount = 0;
#ifdef USE_OPENSSL
	if (SSL_accept(httpRequest.ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        return;
    }else{
    	recvCount = SSL_read(httpRequest.ssl, buffer, BUFFER_SIZE);
    }
#else
	recvCount = recv(newfd, buffer, BUFFER_SIZE, 0);
	if(recvCount < 0){
		printf("recv error\n");
		return;
	}
#endif	
	//buffer[recvCount] = 0;

	//printf("\n\n\n\n\nrecvBuffer=%s\n", buffer);

	httpRequest.method = getMethod(buffer, &position);
	if(!httpRequest.method){
		printf("NOT found this Method\n");
		return;
	}
	//printf("Method=%s\n", httpRequest.method);


	httpRequest.action = getAction(buffer+position, &position);
	if(!httpRequest.action){
		printf("No action\n" );
		return;
	}
	//printf("Action=%s\n\n", httpRequest.action);

	if(!strncmp(buffer,"POST ",4) ){
		httpRequest.contentLength = getContentLength(buffer+position, &position);
		httpRequest.payload = getPayload(buffer+position);

		//somtimes post packet is big
		//need to recv more times
		int retry = 5;
		while( (&buffer[recvCount]-httpRequest.payload) <  httpRequest.contentLength && retry--){
			printf("retry=%d\n",retry );
#ifdef USE_OPENSSL			
			recvCount += SSL_read(httpRequest.ssl, &buffer[recvCount], BUFFER_SIZE-recvCount);
#else
			recvCount += recv(newfd, &buffer[recvCount], BUFFER_SIZE-recvCount, 0);
#endif			
		}

		if( (&buffer[recvCount]-httpRequest.payload) <  httpRequest.contentLength ){
			printf("Fail to recv all content\n");
			return;
		}else{
			postHandler(&httpRequest);
		}
	}else if(!strncmp(httpRequest.method,"GET",3)){
		getHandler(&httpRequest);
	}else{
		printf("NOT Support this Method\n");
	}
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

		assert(taskQueue->tasks > 0);

		task = (LIST_ENTRY_T *)malloc(sizeof(*task));
		if (task == NULL){
			printf("Malloc Task fail\n");
			pthread_mutex_unlock(&taskQueue->mutex);
			pthread_exit(NULL);
		}
		
		task = STAILQ_FIRST(&taskQueue->list_head);
		newfd = task->newfd;
		STAILQ_REMOVE_HEAD(&taskQueue->list_head, next);
		free(task);
		taskQueue->tasks--;
		pthread_mutex_unlock(&taskQueue->mutex);

		handleRequest(newfd);	
		
	}
}
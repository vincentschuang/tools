
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "http.h"


char * getMethod(char * buff, int * position){
	char * ptr = NULL;
	if( !strncmp(buff,"GET ",4) ){
		ptr = buff;
		buff[3] = 0;
		*position = 4;
	}else if( !strncmp(buff,"POST ",5) ){
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
			*position += i;
			break;
		}
	}

	return ptr;
}

int getContentLength(char * buff, int * position){
	char * ptr = NULL;
	
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
	return ptr;
}

fileType_T extensions [] = {
    {"jpg", "image/jpeg"},
    {"jpeg","image/jpeg"},
    {"png", "image/png" },
    {"ico", "image/vnd.microsoft.icon" },
    {"js", "text/javascript;"},
    {"htm", "text/html" },
    {"html","text/html" },
    {"css","text/css" },
    {"exe","text/plain" },
    {"gif", "image/gif" },
    {"zip", "image/zip" },
    {"gz",  "image/gz"  },
    {"tar", "image/tar" },
    {0,0} 
};

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
			return;
		}
	}

	//security check
	if(strstr(httpRequest->action,"..")){
		printf("SECURITY ISSUE\n");
		return;
	}

	struct stat fileStat;
	file_fd = open(path, O_RDONLY, 0644);
	//printf("path=%s fd =%d\n", path, file_fd);
	if(file_fd > -1 && fstat(file_fd, &fileStat) > -1){
		close(file_fd);
	}else{
		printf("FILE Not Exist\n");
		return;
	}

	FILE *fp;
    fp = fopen (path, "r");
    fread(body,sizeof(char),fileStat.st_size,fp);
    fclose(fp);

    sprintf(header,getResponseHeader,fileType ,fileStat.st_size);
	write(httpRequest->newfd, header, strlen(header));
	write(httpRequest->newfd, body, fileStat.st_size);
	
	close(httpRequest->newfd);
}

void handleRequest(int newfd){
	httpRequest_T httpRequest;
	httpRequest.contentLength = 0;
	httpRequest.newfd = newfd;

	char buffer[BUFFER_SIZE];
	bzero(buffer, BUFFER_SIZE);

	int position = 0, recvCount = 0;

	recvCount = recv(newfd, buffer, BUFFER_SIZE, 0);
	if(recvCount < 0){
		printf("recv error\n");
		return;
	}
	buffer[recvCount] = 0;

	printf("recvBuffer=%s\n", buffer);

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

	if(!strncmp(buffer,"POST ",5) ){
		httpRequest.contentLength = getContentLength(buffer+position, &position);
		int retry = 5;
		while(httpRequest.contentLength < recvCount && retry--){
			recvCount += recv(newfd, &buffer[recvCount], BUFFER_SIZE-recvCount, 0);
		}

		if(httpRequest.contentLength < recvCount){
			printf("Fail to recv all content\n");
			return;
		}else{
			httpRequest.payload = getPayload(buffer+position);
			//postHandler();
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
		if(taskQueue->tasks > 0){
			//free entry then

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
		}else{
			pthread_mutex_unlock(&taskQueue->mutex);
			usleep(WAITING_TIME);
		}
	}
}
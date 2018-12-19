
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





pthread_t * initThreadPool(taskQueue_T * taskQueue){
	int i = 0;
	pthread_t *pThreadID = NULL;

	pThreadID = (pthread_t *)malloc( sizeof(*pThreadID) * THREAD_LIMIT );
	for(i=0; i<THREAD_LIMIT; i++){
		 pthread_create(&pThreadID[i], NULL, httpHandler, taskQueue);
	}

	return pThreadID;
}

void addTask(taskQueue_T* taskQueue, int newsockfd){
	pthread_mutex_lock(&taskQueue->mutex);

	if(taskQueue->tasks == 0){
		pthread_cond_broadcast(&taskQueue->workAvailable);
	}

	LIST_ENTRY_T * newEntry = (LIST_ENTRY_T*)malloc(sizeof(*newEntry));

	if(newEntry  == NULL){
		printf("Malloc Fail\n");
		return;
	}

	newEntry->newfd = newsockfd;

	STAILQ_INSERT_TAIL(&taskQueue->list_head, newEntry, next);
	taskQueue->tasks++;

	pthread_mutex_unlock(&taskQueue->mutex);
}

int main()
{
	int fd = 0;

	fd = TcpSocketCreate();
 	if(fd < 0){
 		printf("Close Server\n");
 		return 0;
 	}

 	listen(fd, CONNECTION_LIMIT);
 	printf("Listening...\n");

 	//init task queue
 	taskQueue_T taskQueue = {
 		.tasks = 0,
		.mutex = PTHREAD_MUTEX_INITIALIZER,
		.workAvailable = PTHREAD_COND_INITIALIZER,
		.list_head = STAILQ_HEAD_INITIALIZER(taskQueue.list_head),
 	};

 	//init thread pool
 	pthread_t * threadID;
 	threadID = initThreadPool(&taskQueue);

 	struct sockaddr_in client_addr;
  	socklen_t cli_len = sizeof(client_addr);

 	//add task to taskQueue
 	while (1) {
		int newsockfd = accept(fd, (struct sockaddr *) &client_addr,  &cli_len);
		if (newsockfd < 0) 
			printf("Error on accept");

		printf("New socket: %d\n", newsockfd);

		addTask(&taskQueue, newsockfd);
	}


 	//finish

 	close(fd);
 	printf("Server Left\n");
 return 0;

}
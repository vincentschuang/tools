
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "add_openssl.h"
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

#ifdef USE_OPENSSL
	SSL_CTX *ctx;
#endif

//global value
GVALUE_T gValue={
	.suppress={
		.nextAccessTime = {2,0,1,8,1,2,2,5,1,0,3,0,2,5,'\0'},
		.todayValue = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48},
		.monthValue = 0,
	}
};

taskQueue_T* initTaskQueue(){
	taskQueue_T * queue;
	queue = (taskQueue_T *)malloc(sizeof(taskQueue_T));

	queue->tasks = 0;
	//queue->mutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_init(&queue->mutex, NULL);
	//queue->workAvailable = PTHREAD_COND_INITIALIZER;
	pthread_cond_init(&queue->workAvailable,NULL);
	//queue->list_head = STAILQ_HEAD_INITIALIZER(taskQueue->list_head);
	STAILQ_INIT(&queue->list_head);

	return queue;
}


int main()
{
	int fd = 0;

#ifdef USE_OPENSSL
    init_openssl();
    ctx = create_context();
    configure_context(ctx);
#endif

    sigignore(SIGPIPE);

    fd = TcpSocketCreate();

 	if(fd < 0){
 		printf("Close Server\n");
 		return 0;
 	}

 	listen(fd, CONNECTION_LIMIT);
 	printf("Listening...\n");

 	//init task queue
 	taskQueue_T* taskQueue;
 	taskQueue = initTaskQueue();

 	//init thread pool
 	pthread_t * threadID;
 	threadID = initThreadPool(taskQueue);

 	struct sockaddr_in client_addr;
  	socklen_t cli_len = sizeof(client_addr);

 	//add task to taskQueue
 	while (1) {
		int newsockfd = accept(fd, (struct sockaddr *) &client_addr,  &cli_len);
		if (newsockfd < 0) 
			printf("Error on accept");

		printf("New socket: %d\n", newsockfd);

		addTask(taskQueue, newsockfd);
	}


 	//finish

 	close(fd);
#ifdef USE_OPENSSL 	
 	SSL_CTX_free(ctx);
    cleanup_openssl();
#endif    
 	printf("Server Left\n");
 return 0;

}
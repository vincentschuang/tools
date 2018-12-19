#ifndef __WEB_H__
#define __WEB_H__

#define LISTEN_PORT		 8080
#define CONNECTION_LIMIT 10
#define WWW_FOLDER		 "/home/pvicm/tools/WebServer"
#define THREAD_LIMIT	10

#include <sys/queue.h>
#include <pthread.h>

typedef struct listEntryStruc    LIST_ENTRY_T;

struct listEntryStruc {
    STAILQ_ENTRY(listEntryStruc) next;     // link to next entry
    int               newfd;
};

typedef STAILQ_HEAD(useless, listEntryStruc)  INIT_LIST_HEAD_T;

typedef struct taskQueue taskQueue_T;

struct taskQueue{
	int 				  tasks;
	pthread_mutex_t       mutex;
	pthread_cond_t 		  workAvailable;
	INIT_LIST_HEAD_T	  list_head;
};


#endif

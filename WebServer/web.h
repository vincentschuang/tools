#ifndef __WEB_H__
#define __WEB_H__

#define USE_OPENSSL 1

#ifdef USE_OPENSSL
#define LISTEN_PORT		 443
#else
#define LISTEN_PORT		 8080
#endif

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

//global valuable
struct suppress {
	char nextAccessTime[14];
	char todayValue[1];
	char monthValue[1];
};
typedef struct suppress SUPPRESS_T;

struct gValue {
	SUPPRESS_T suppress;
};
typedef struct gValue GVALUE_T;


extern GVALUE_T gValue;

#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef USE_OPENSSL
extern	SSL_CTX *ctx;
#endif

#endif
#ifndef __STAILQ_H__
#define __STAILQ_H__

#include <stdint.h>
#include <sys/queue.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct listEntryStruc    LIST_ENTRY_T;

struct listEntryStruc {
    STAILQ_ENTRY(listEntryStruc) next;     // link to next entry
    int               payload_len;		   // self define data 1
    uint8_t*              payload;         // self define data 2
}__attribute__((packed));				   /* use less memery by structure; in ancient time,
											  embedded system has very limit resource
											  they tradeoff between memory and speed.
											*/
typedef STAILQ_HEAD(useless, listEntryStruc)  INIT_LIST_HEAD_T;

typedef struct STAILQListStatus {
    int                   entries;        // number of entries in the list
    pthread_mutex_t       mutex;          // exclusive mutex for list operation
    INIT_LIST_HEAD_T    list_head;      // list head
} STAILQ_LIST_STATUS_T;


//control block


#endif

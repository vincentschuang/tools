#include <stdio.h>
#include "STAILQ.h"

STAILQ_LIST_STATUS_T STAILQListStatus = {
	.entries    = 0,
	.mutex      = PTHREAD_MUTEX_INITIALIZER,
	.list_head  = STAILQ_HEAD_INITIALIZER(STAILQListStatus.list_head),
};


void main(){

	int i=0 ,c=0;
	//add entries
	LIST_ENTRY_T * newEntry = NULL;
	pthread_mutex_lock(&STAILQListStatus.mutex);

	for ( i = 1; i < 10; ++i){
		newEntry = (LIST_ENTRY_T*)malloc(sizeof(LIST_ENTRY_T));
		if(newEntry  == NULL){
			printf("Malloc Fail\n");
			return;
		}else{

		}

		newEntry->payload_len = i;
		newEntry->payload = (uint8_t *)malloc(i);
		if (newEntry->payload == NULL){
			printf("Malloc Fail\n");
			return;
		}

		for ( c = 0; c < newEntry->payload_len; ++c){
			newEntry->payload[c] = 'A'+ c;
		}

		STAILQ_INSERT_TAIL(&STAILQListStatus.list_head, newEntry, next);
	    STAILQListStatus.entries++;
	}

	pthread_mutex_unlock(&STAILQListStatus.mutex);



	//show all
	LIST_ENTRY_T * forEntry = NULL;
	pthread_mutex_lock(&STAILQListStatus.mutex);

	STAILQ_FOREACH(forEntry, &STAILQListStatus.list_head, next){
		if(forEntry){
			printf("entry payload_len= %d\n", forEntry->payload_len);
			for ( i = 0; i < forEntry->payload_len; ++i){
				printf("%c",forEntry->payload[i] );
			}
			printf("\n");
		}
	}

	pthread_mutex_unlock(&STAILQListStatus.mutex);

	//free one by one


	LIST_ENTRY_T * freeEntry = NULL;
	printf("\nbefore free\n");
	pthread_mutex_lock(&STAILQListStatus.mutex);
	while(STAILQListStatus.entries > 0){
		freeEntry = STAILQ_FIRST(&STAILQListStatus.list_head);
		STAILQ_REMOVE_HEAD(&STAILQListStatus.list_head, next); //only change head pointer, did not free
	    STAILQListStatus.entries--;

	    if (freeEntry != NULL){
	    	for ( i = 0; i < freeEntry->payload_len; ++i){
				printf("%c",freeEntry->payload[i] );
			}
			printf("\n");
	    	if (freeEntry->payload)
	    		free(freeEntry->payload);
	    	freeEntry->payload = NULL;
	    	free(freeEntry);
	    	freeEntry = NULL;
	    }

	}
	pthread_mutex_unlock(&STAILQListStatus.mutex);

	return ;

}
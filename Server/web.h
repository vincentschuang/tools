#define MAXEVENTS	10240
#define MAX_PORCESS	10240
#define BUF_SIZE	1024
#define MAX_URL_LENGTH	128


#include "list.h"
#include "hash.h"
#include "hashtable.h"

#define INDEX_FILE "/index.htm"

struct server_config {
    int efd;
 	int listen_sock;
 	int current_total_processes;
 	DECLARE_HASHTABLE(htable, 8);
 	char *index_root;
};

struct process {
    int sock;
    int status;
    int response_code;
    int fd;
    int read_pos;
    int write_pos;
    int total_length;
    char buf[BUF_SIZE];

    struct hlist_node node;
};

void send_response_header(struct process *process);

int setNonblocking(int fd);

void accept_sock(int listen_sock);

void read_request(struct process* process);

void send_response_header(struct process *process);

void send_response(struct process *process);

void cleanup(struct process *process);

void handle_error(struct process *process, char* error_string);

void reset_process(struct process *process);

int open_file(char *filename);

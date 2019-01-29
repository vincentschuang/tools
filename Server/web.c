#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <langinfo.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>

#include "web.h"



#define STATUS_READ_REQUEST_HEADER	0
#define STATUS_SEND_RESPONSE_HEADER	1
#define STATUS_SEND_RESPONSE		2

#define NO_SOCK -1
#define NO_FILE -1

#define RFC1123_DATE_FMT "%a, %d %b %Y %H:%M:%S %Z"

#define header_404 "HTTP/1.1 404 Not Found\r\nServer: server/1.0\r\nContent-Type: text/html\r\nConnection: Close\r\n\r\n<h1>Not found</h1>"
#define header_400 "HTTP/1.1 400 Bad Request\r\nServer: server/1.0\r\nContent-Type: text/html\r\nConnection: Close\r\n\r\n<h1>Bad request</h1>"
#define header_200_start "HTTP/1.1 200 OK\r\nServer: server/1.0\r\nContent-Type: text/html\r\nConnection: Close\r\n"
#define header_304_start "HTTP/1.1 304 Not Modified\r\nServer: clowwindyserver/1.0\r\nContent-Type: text/html\r\nConnection: Close\r\n"

#define header_end "\r\n"

#define HEADER_IF_MODIFIED_SINCE "If-Modified-Since: "

#define write_to_header(string_to_write) strcpy(process->buf + strlen(process->buf), string_to_write)

struct server_config gServerConfig;


int setNonblocking ( int fd )
{
    int flags;
    if ( -1 == ( flags = fcntl ( fd, F_GETFL, 0 ) ) )
        flags = 0;
    return fcntl ( fd, F_SETFL, flags | O_NONBLOCK );

}

static int create_and_bind ( char *port )
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, listen_sock;

    memset ( &hints, 0, sizeof ( struct addrinfo ) );
    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;     /* All interfaces */

    s = getaddrinfo ( NULL, port, &hints, &result );
    if ( s != 0 )
    {
        fprintf ( stderr, "getaddrinfo: %s\n", gai_strerror ( s ) );
        return -1;
    }

    for ( rp = result; rp != NULL; rp = rp->ai_next )
    {
        listen_sock = socket ( rp->ai_family, rp->ai_socktype, rp->ai_protocol );
        int opt = 1;
        setsockopt ( listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof ( opt ) );
        if ( listen_sock == -1 )
            continue;

        s = bind ( listen_sock, rp->ai_addr, rp->ai_addrlen );
        if ( s == 0 )
        {
            /* We managed to bind successfully! */
            break;
        }

        close ( listen_sock );
    }

    if ( rp == NULL )
    {
        fprintf ( stderr, "Could not bind\n" );
        return -1;
    }

    freeaddrinfo ( result );

    return listen_sock;
}

void handle_error ( struct process* process, char* error_string )
{
	printf("handle_error \n");
	printf("Error=%s\n" ,error_string );
    //cleanup ( process );
    
}

void cleanup ( struct process *process )
{
	//printf("cleanup\n\n");
    int s;
    if(process==NULL){
    	return;
    }
    if ( process->sock != NO_SOCK )
    {
        s = close ( process->sock );
        gServerConfig.current_total_processes--;
        if ( s == NO_SOCK )
        {
            perror ( "close sock" );
        }
    }
    if ( process->fd != -1 )
    {
        s = close ( process->fd );
        if ( s == NO_FILE )
        {
            printf ( "fd: %d\n",process->fd );
            printf ( "\n" );
            perror ( "close file" );
        }
    }
    /*free and remove htable*/
    hash_del(&process->node);
    free(process);
    process = NULL;
}

int write_all ( struct process *process, char* buf, int n )
{
    int done_write = 0;
    int total_bytes_write = 0;
    while ( !done_write && total_bytes_write != n )
    {
        int bytes_write = write( process->sock, buf + total_bytes_write, n - total_bytes_write );
        if ( bytes_write == -1 )
        {
            if ( errno != EAGAIN )
            {
                handle_error ( process, "write" );
                return 0;
            }
            else
            {
                //write buffer full
                return total_bytes_write;
            }
        }
        else
        {
            total_bytes_write += bytes_write;
        }
    }
    return total_bytes_write;
}

void send_response ( struct process *process )
{
	//printf("send_response\n");
    while ( 1 )
    {
        int s = sendfile ( process-> sock, process -> fd, &process->read_pos, process->total_length - process -> read_pos );
        if ( s == -1 )
        {
            if ( errno != EAGAIN )
            {
                handle_error ( process, "sendfile" );
                return;
            }
            else
            {
                // buffer full
                return;
            }
        }
        if ( process->read_pos == process->total_length )
        {
            // finish
            cleanup ( process );
            return;
        }
    }

}

void send_response_header ( struct process *process )
{
	//printf("send_response_header code=%d\n",process->response_code);
    if ( process->response_code != 200 )
    {
		//only 200 have to send response
        int bytes_writen = write_all ( process, process->buf+process->write_pos, strlen(process->buf)-process->write_pos );
        if ( bytes_writen == strlen(process->buf) + process->write_pos )
        {
            // wite finish
            //printf("srh cleanup\n");
            cleanup ( process );
        }
        else
        {
            process->write_pos += bytes_writen;
        }
    }
    else
    {
        int bytes_writen = write_all ( process, process->buf+process->write_pos, strlen ( process->buf )-process->write_pos );
        if ( bytes_writen == strlen(process->buf) + process->write_pos )
        {
            // wite all
            process->status = STATUS_SEND_RESPONSE;
            send_response ( process );
        }
        else
        {
            process->write_pos += bytes_writen;
        }
    }
}

void inline reset_process ( struct process* process )
{
    process->read_pos = 0;
    process->write_pos = 0;
}

int get_index_file ( char *filename_buf, struct stat *pstat )
{
	//printf("filename_buf=%s\n", filename_buf);
    struct stat stat_buf;
    int s;
    s = lstat ( filename_buf, &stat_buf );
    if ( s == -1 )
    {
        // 文件或目录不存在
        return -1;
    }
    if ( S_ISDIR ( stat_buf.st_mode ) )
    {
        // 是目录，追加index.htm(l)
        strcpy ( filename_buf + strlen ( filename_buf ), INDEX_FILE );
        // 再次判断是否是文件
        s = lstat ( filename_buf, &stat_buf );
        if ( s == -1 || S_ISDIR ( stat_buf.st_mode ) )
        {
            // 文件不存在，或者为目录
            int len = strlen ( filename_buf );
            filename_buf[len] = 'l';
            filename_buf[len + 1] = 0;
            s = lstat ( filename_buf, &stat_buf );
            if ( s == -1 || S_ISDIR ( stat_buf.st_mode ) )
            {
                // 文件不存在，或者为目录
                return -1;
            }
        }
    }
    *pstat = stat_buf;
    return 0;
}

void read_request ( struct process* process )
{
	//printf("Read Request\n");
    int sock = process->sock, s;
    char* buf=process->buf;
    char read_complete = 0;

    ssize_t count;

    while ( 1 )
    {
        count = read ( sock, buf + process->read_pos, BUF_SIZE - process->read_pos );
        if ( count == -1 )
        {
            if ( errno != EAGAIN )
            {
                handle_error ( process, "read request" );
                return;
            }
            else
            {
                //errno == EAGAIN means we have read all
                break;
            }
        }
        else if ( count == 0 )
        {
            // client close connection
            cleanup ( process );
            return;
        }
        else if ( count > 0 )
        {
            process->read_pos += count;
        }
    }

    int header_length = process->read_pos;
    // determine whether the request is complete
    if ( header_length > BUF_SIZE - 1 )
    {
    	process->response_code = 400;
    	process->status = STATUS_SEND_RESPONSE_HEADER;
    	strcpy ( process->buf, header_400 );
    	send_response_header( process );
    	handle_error( process, "bad request" );
    	return;
    }
    buf[header_length]=0;
    //printf("buf=%s\n", buf);
    read_complete = ( strstr ( buf, "\n\n" ) != 0 ) || ( strstr ( buf, "\r\n\r\n" ) != 0 );

    int error = 0;
    if ( read_complete )
    {
        //reset read position
        reset_process ( process );
        // get GET info
        if ( !strncmp ( buf, "GET", 3 ) == 0 )
        {
            process->response_code = 400;
            process->status = STATUS_SEND_RESPONSE_HEADER;
            strcpy ( process->buf, header_400 );
            send_response_header ( process );
            handle_error ( process, "bad request" );
            return;
        }
        // get first line
        int n_loc = ( int ) strchr ( buf, '\n' );
        int space_loc = ( int ) strchr ( buf + 4, ' ' );
        if ( n_loc <= space_loc )
        {
            process->response_code = 400;
            process->status = STATUS_SEND_RESPONSE_HEADER;
            strcpy ( process->buf, header_400 );
            send_response_header ( process );
            handle_error ( process, "bad request" );
            return;
        }
        char path[255];
        int len = space_loc - ( int ) buf - 4;
        if ( len > MAX_URL_LENGTH )
        {
            process->response_code = 400;
            process->status = STATUS_SEND_RESPONSE_HEADER;
            strcpy ( process->buf, header_400 );
            send_response_header ( process );
            handle_error ( process, "bad request" );
            return;
        }
        buf[header_length] = 0;
        strncpy ( path, buf+4, len );
        path[len] = 0;

        struct stat filestat;
        char fullname[256];
        char *prefix = gServerConfig.index_root;
        strcpy ( fullname, prefix );
        strcpy ( fullname + strlen ( prefix ), path );
        s = get_index_file ( fullname, &filestat);
        if ( s == -1 )
        {
            process->response_code = 404;
            process->status = STATUS_SEND_RESPONSE_HEADER;
            strcpy ( process->buf, header_404 );
            send_response_header ( process );
            handle_error ( process, "1 not found" );
            return;
        }

        int fd = open ( fullname, O_RDONLY );

        process->fd = fd;
        if ( fd<0 )
        {
            process->response_code = 404;
            process->status = STATUS_SEND_RESPONSE_HEADER;
            strcpy ( process->buf, header_404 );
            send_response_header ( process );
            handle_error ( process, "2 not found" );
            return;
        }
        else
        {
            process->response_code = 200;
        }
	
        char tempstring[256];

        //检查有无If-Modified-Since，返回304
        char* c = strstr ( buf, HEADER_IF_MODIFIED_SINCE );
    	if(c!=0){
    	    char* rn = strchr(c, '\r');
    	    if(rn==0){
                rn = strchr(c, '\n');
                if(rn==0){
                process->response_code = 400;
                process->status = STATUS_SEND_RESPONSE_HEADER;
                strcpy ( process->buf, header_400 );
                send_response_header ( process );
                handle_error ( process, "bad request" );
                return;
                }
    	    }
            int time_len = rn - c - sizeof(HEADER_IF_MODIFIED_SINCE) + 1;
            strncpy(tempstring, c + sizeof(HEADER_IF_MODIFIED_SINCE) - 1,time_len);
            tempstring[time_len]=0;
            {
                struct tm tm;
                time_t t;
                strptime(tempstring, RFC1123_DATE_FMT, &tm);
                tzset();
                t=mktime(&tm);
                t-=timezone;
                gmtime_r(&t, &tm);
                if(t >= filestat.st_mtime){
                    process->response_code = 304;
                }
            }
    	}

        //开始header
        process->buf[0] = 0;
    	if(process->response_code == 304){
    	  write_to_header ( header_304_start );
    	} else {
    	  write_to_header ( header_200_start );
    	}

        process->total_length = filestat.st_size;

        {
            //写入当前时间
            struct tm *tm;
            time_t tmt;
            tmt = time ( NULL );
            tm = gmtime ( &tmt );
            strftime ( tempstring, sizeof ( tempstring ), RFC1123_DATE_FMT, tm );
            write_to_header ( "Date: " );
            write_to_header ( tempstring );
            write_to_header ( "\r\n" );

            //写入文件修改时间
            tm = gmtime ( &filestat.st_mtime );
            strftime ( tempstring, sizeof ( tempstring ), RFC1123_DATE_FMT, tm );
            write_to_header ( "Last-modified: " );
            write_to_header ( tempstring );
            write_to_header ( "\r\n" );

            if(process->response_code == 200){
                //写入content长度
                sprintf ( tempstring, "Content-Length: %ld\r\n", filestat.st_size );
                write_to_header ( tempstring );
            }
        }

        //结束header
        write_to_header ( header_end );

        process->status = STATUS_SEND_RESPONSE_HEADER;
        //modify monitor status to write
        struct epoll_event event;
        event.data.fd = process->sock;
        event.events = EPOLLOUT | EPOLLET;
        s = epoll_ctl (gServerConfig.efd, EPOLL_CTL_MOD, process->sock, &event );
        if ( s == -1 )
        {
            perror ( "epoll_ctl" );
            abort ();
        }
        //发送header
        send_response_header ( process );
    }
}

void handle_event ( int sock )
{
	//printf("\n\nHandle event\n");
	int listen_sock = gServerConfig.listen_sock;
    if ( sock == listen_sock )
    {
        accept_sock ( sock );
    }
    else
    {
        /*Get process by hash Table*/
	    struct process* process;
	    hash_for_each_possible(gServerConfig.htable, process, node, sock) {
	        if(process->sock == sock) {
	            break;
	        }
	    }
	    //printf("Get Process sock =%d\n",sock);
	    //todo: if not found ??

        if ( process != NULL )
        {
            switch ( process->status )
            {
            case STATUS_READ_REQUEST_HEADER:
                read_request ( process );
                break;
            case STATUS_SEND_RESPONSE_HEADER:
                send_response_header ( process );
                break;
            case STATUS_SEND_RESPONSE:
                send_response ( process );
                break;
            default:
                break;
            }
        }
    }
}

void accept_sock ( int listen_sock )
{
	//printf("Accept socket\n");
    int s;
    // in Egde Trigger Mode do loop until accept return -1
    while ( 1 )
    {
        struct sockaddr in_addr;
        socklen_t in_len;
        int infd;
        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

        in_len = sizeof in_addr;
        infd = accept ( listen_sock, &in_addr, &in_len );
        if ( infd == -1 )
        {
            if ( ( errno == EAGAIN ) || ( errno == EWOULDBLOCK ) )
            {
                /* We have processed all incoming connections. */
                break;
            }
            else
            {
                perror( "accept" );
                break;
            }
        }

        getnameinfo ( &in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV );
        /* Make the incoming socket non-blocking and add it to the
           list of fds to monitor. */
        s = setNonblocking ( infd );
        if ( s == -1 )
            abort ();
        int on = 1;
        setsockopt ( infd, SOL_TCP, TCP_CORK, &on, sizeof ( on ) );

        struct epoll_event event;
        event.data.fd = infd;
        event.events = EPOLLIN | EPOLLET;
        s = epoll_ctl ( gServerConfig.efd, EPOLL_CTL_ADD, infd, &event );
        if ( s == -1 )
        {
            perror ( "epoll_ctl" );
            abort ();
        }

        /*Add infd to hash table with init status*/
        struct process* process = malloc(sizeof(*process));
        process->read_pos = 0;
    	process->write_pos = 0;
        process->sock = infd;
        process->fd = NO_FILE;
        process->status = STATUS_READ_REQUEST_HEADER;
        //printf("add Socket %d\n",infd );
        hash_add(gServerConfig.htable, &process->node, process->sock);
        gServerConfig.current_total_processes++;
    }
}



int main(int argc, char const *argv[])
{
	int s, efd;
	struct epoll_event *events, event;

	if ( argc != 3 )
    {
        fprintf ( stderr, "Usage: %s [port] [index root]\n", argv[0] );
        exit ( EXIT_FAILURE );
    }

    gServerConfig.current_total_processes = 0;
    /*init hashtable*/

    int listen_sock = create_and_bind ( argv[1] );
    gServerConfig.index_root = argv[2];

    if ( listen_sock == -1 )
        abort ();

    gServerConfig.listen_sock = listen_sock;

    s = setNonblocking ( listen_sock );
    if ( s == -1 )
        abort ();

    s = listen ( listen_sock, SOMAXCONN );
    if ( s == -1 )
    {
        perror ( "listen" );
        abort ();
    }

    efd = epoll_create1 ( 0 );
    if ( efd == -1 )
    {
        perror ( "epoll_create" );
        abort ();
    }

    gServerConfig.efd = efd;

    event.data.fd = listen_sock;
    event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl ( efd, EPOLL_CTL_ADD, listen_sock, &event );
    if ( s == -1 )
    {
        perror ( "epoll_ctl" );
        abort ();
    }

    events = calloc ( MAXEVENTS, sizeof event );
    while ( 1 )
    {
        int n, i;

        n = epoll_wait ( efd, events, MAXEVENTS, -1 );
        if ( n == -1 )
        {
            perror ( "epoll_wait" );
        }
        for ( i = 0; i < n; i++ )
        {
            if ( ( events[i].events & EPOLLERR ) || ( events[i].events & EPOLLHUP ) )
            {
                /* An error has occured on this fd, or the socket is not
                   ready for reading (why were we notified then?) */
                fprintf ( stderr, "epoll error\n" );
                close ( events[i].data.fd );
                continue;
            }

            handle_event( events[i].data.fd);

        }
    }

    free ( events );

    close ( listen_sock );

    return EXIT_SUCCESS;
	return 0;
}
#ifndef __ADD_OPENSSL_H__
#define __ADD_OPENSSL_H__

#include <openssl/ssl.h>
#include <openssl/err.h>





//functions
int openssl_create_socket(int port);
void init_openssl();
SSL_CTX *create_context();
void configure_context(SSL_CTX *ctx);

#endif
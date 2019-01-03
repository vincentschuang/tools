#ifndef __ADD_OPENSSL_H__
#define __ADD_OPENSSL_H__

#include <openssl/ssl.h>
#include <openssl/err.h>





//functions
int openssl_create_socket(int port);
void initOpenssl();
SSL_CTX *createContext();
void configureContext(SSL_CTX *ctx);

#endif
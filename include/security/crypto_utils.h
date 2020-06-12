/**
 * @file crypto.h
 * @author Riccardo Mancini
 * @brief Header for crypto utilities
 * @date 2020-06-07
 * 
 */

#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/hmac.h>
#include <openssl/kdf.h>
#include <string>
#include "logging.h"
#include <map>

using namespace std;

#define KEY_BIO_MAX_SIZE 256

/**
 * Writes the key internal byte representation to the given buffer.
 * 
 * @param key the key to serialize
 * @param buf the buffer
 * @param buflen the buffer length
 * @returns the number of written bytes
 */
int pkey2buf(EVP_PKEY **key, char* buf, int buflen);

/**
 * Reads the pkey from the given buffer.
 * 
 * @param buf the buffer
 * @param buflen the buffer length
 * @param key the key 
 * @returns 1 in case of success, <=0 otherwise
 */
int buf2pkey(char* buf, int buflen, EVP_PKEY **key);

/**
 * Writes the cert internal byte representation to the given buffer.
 * 
 * @param cert the key to serialize
 * @param buf the buffer
 * @param buflen the buffer length
 * @returns the number of written bytes
 */
int cert2buf(X509 **cert, char* buf, int buflen);

/**
 * Reads the cert from the given buffer.
 * 
 * @param buf the buffer
 * @param buflen the buffer length
 * @param cert the key 
 * @returns 1 in case of success, <=0 otherwise
 */
int buf2cert(char* buf, int buflen, X509 **cert);


/**
 * Extracts the username (aka CN) from the given certificate
 */
string usernameFromCert(X509* cert);

/**
 * Builds a map username-certificate from the given directory
 * 
 * This function matches the pattern *_cert.pem inside the directory.
 */
map<string,X509*> buildCertMapFromDirectory(string dir);

#endif // CRYPTO_UTILS_H
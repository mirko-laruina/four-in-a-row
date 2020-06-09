/**
 * @file crypto.h
 * @author Mirko Laruina
 * @brief Header for crypto algorithms
 * @date 2020-06-07
 * 
 */

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/hmac.h>
#include <openssl/kdf.h>
#include <string.h>

typedef uint32_t nonce_t;

/**
 * Encrypts using AES in GCM mode
 * 
 * @param plaintext     buffer where the plaintext is stored
 * @param plaintext_len length of said buffer
 * @param aad           additional authenticated data buffer
 * @param aad_len       length of said buffer
 * @param key           encryption key
 * @param iv            initialization vector
 * @param ciphertext    buffer (already allocated) where the ct will be stored
 * @param tag           tag buffer
 * 
 * @return number of written bytes
 */
int aes_gcm_encrypt(unsigned char *plaintext, int plaintext_len,
                    unsigned char *aad, int aad_len,
                    unsigned char *key, unsigned char *iv,
                    unsigned char *ciphertext,
                    unsigned char *tag);

/**
 * Decrypts using AES in GCM mode
 * 
 * @param ciphertext        buffer where the ciphertext is stored
 * @param ciphertext_len    length of said buffer
 * @param aad               additional authenticated data buffer
 * @param aad_len           length of said buffer
 * @param key               decryption key
 * @param iv                initialization vector
 * @param plaintext         buffer (already allocated) where the pt will be stored
 * @param tag               tag buffer
 * 
 * @retval -1               on error
 * @retval n                number of written bytes
 */
int aes_gcm_decrypt(unsigned char *ciphertext, int ciphertext_len,
                    unsigned char *aad, int aad_len,
                    unsigned char *key,
                    unsigned char *iv,
                    unsigned char *plaintext,
                    unsigned char *tag);

/**
 * @brief Generate a ECDH key
 * 
 * Example of usage:
 * EVP_PKEY *key=NULL;
 * int ret = get_ecdh_key(&key);
 * 
 * @param key   the generated key 
 * @return int  ???
 */
int get_ecdh_key(EVP_PKEY **key);

/**
 * @brief Apply the DHKE to derive a shared secret
 * 
 * @param my_key        first key
 * @param peer_pubkey   second key
 * @param shared_key    output buffer location (unallocated), it will contained the shared key
 * @return int          shared_key length
 */
int dhke(EVP_PKEY *my_key, EVP_PKEY *peer_pubkey, unsigned char *shared_key);

/**
 * @brief Get a random number
 * 
 * @return int  the random number
 */
int get_rand();

/**
 * @brief Load a certificate from file
 * 
 * @param file_name     file name of the certificate
 * @return X509*        the certificate ptr, NULL if not read correctly
 */
X509 *load_cert_file(char *file_name);

/**
 * @brief Load certificate revocation list from file
 * 
 * @param file_name    file name
 * @return X509_CRL*   the CRL, NULL if not read correctly
 */
X509_CRL *load_crl_file(char *file_name);

/**
 * @brief Build a CA store from CA certificate and CRL
 * 
 * @param cacert        CA certificate
 * @param crl           CRL
 * @return X509_STORE*  the store
 */
X509_STORE *build_store(X509 *cacert, X509_CRL *crl);


/**
 * @brief 
 * 
 * @param store     Certificate store
 * @param cert      Certificate
 * @return true     if validation is successful
 * @return false    otherwise
 */
bool verify_peer_cert(X509_STORE *store, X509 *cert);

/**
 * @brief Calculate HMAC of the msg
 * 
 * @param msg       message of which we need the HMAC
 * @param msg_len   size of said message
 * @param key       key to use for the HMAC
 * @param keylen    size of said key
 * @param hmac      output buffer (uninitialized, it will be allocated)
 * @return int      size of the HMAC
 */
int hmac(char *msg, int msg_len, char *key, unsigned int keylen,
         unsigned char *hmac);

/**
 * @brief Compare two HMAC in a secure way
 * 
 * @param hmac_expected     Expected HMAC
 * @param hmac_rcv          Received HMAC
 * @param len               Length of the buffers to compare
 * @return true             if they are the same
 * @return false            otherwise
 */
bool compare_hmac(unsigned char *hmac_expected, unsigned char *hmac_rcv, unsigned int len);

/**
 * @brief Apply HKDF, takes only one info field
 * 
 * @param key               Key to use
 * @param key_len           Size of said key
 * @param info              Info field to use
 * @param info_len          Size of said info field
 * @param out               Output buffer (allocated)
 * @param outlen            Output len
 */
void hkdf_one_info(unsigned char *key, size_t key_len,
                   unsigned char *info, size_t info_len,
                   unsigned char *out, size_t outlen);

/**
 * @brief Apply HKDF, takes two nonces and a label field
 * 
 * @param key               Key to use
 * @param key_len           Size of said key
 * @param nonce1            First nonce
 * @param nonce2            Second nonce
 * @param label             Label field
 * @param out               Output buffer (allocated)
 * @param outlen            Output len
 */
void hkdf(unsigned char *key, size_t key_len,
          nonce_t nonce1, nonce_t nonce2,
          char *label,
          unsigned char *out, size_t outlen);

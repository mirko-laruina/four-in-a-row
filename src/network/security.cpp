#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <string.h>
#include <openssl/hmac.h>
#include <openssl/kdf.h>

typedef uint32_t nonce_t;

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

int aes_gcm_encrypt(unsigned char *plaintext, int plaintext_len,
                    unsigned char *aad, int aad_len,
                    unsigned char *key,
                    unsigned char *iv,
                    unsigned char *ciphertext,
                    unsigned char *tag)
{
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /* Initialise the encryption operation. */
    if (1 != EVP_EncryptInit(ctx, EVP_aes_128_gcm(), key, iv))
        handleErrors();

    /*
     * Provide any AAD data. This can be called zero or more times as
     * required
     */
    if (1 != EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len))
        handleErrors();

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;

    /*
     * Finalise the encryption. Normally ciphertext bytes may be written at
     * this stage, but this does not occur in GCM mode
     */
    if (1 != EVP_EncryptFinal(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;

    /* Get the tag */
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
        handleErrors();

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int aes_gcm_decrypt(unsigned char *ciphertext, int ciphertext_len,
                    unsigned char *aad, int aad_len,
                    unsigned char *key,
                    unsigned char *iv,
                    unsigned char *plaintext,
                    unsigned char *tag
                    )
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    int ret;

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /* Initialise the decryption operation. */
    if (!EVP_DecryptInit(ctx, EVP_aes_128_gcm(), key, iv))
        handleErrors();

    /*
     * Provide any AAD data. This can be called zero or more times as
     * required
     */
    if (!EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len))
        handleErrors();

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    if (!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /* Set expected tag value. Works in OpenSSL 1.0.1d and later */
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag))
        handleErrors();

    /*
     * Finalise the decryption. A positive return value indicates success,
     * anything else is a failure - the plaintext is not trustworthy.
     */
    ret = EVP_DecryptFinal(ctx, plaintext + len, &len);

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    if (ret > 0)
    {
        /* Success */
        plaintext_len += len;
        return plaintext_len;
    }
    else
    {
        /* Verify failed */
        return -1;
    }
}

int get_ecdh_key(EVP_PKEY **keypair)
{
    EVP_PKEY *dh_params = NULL;
    EVP_PKEY_CTX *ctx_params;
    int ret;

    ctx_params = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    if (!ctx_params)
    {
        handleErrors();
    }
    ret = EVP_PKEY_paramgen_init(ctx_params);
    if (!ret)
    {
        handleErrors();
    }
    //Using NID_X9_62_prime256v1 curve
    ret = EVP_PKEY_CTX_set_ec_paramgen_curve_nid(
        ctx_params,
        NID_X9_62_prime256v1);
    if (!ret)
    {
        handleErrors();
    }

    ret = EVP_PKEY_paramgen(ctx_params, &dh_params);
    if (!ret)
    {
        handleErrors();
    }
    //check
    if (dh_params == NULL)
    {
        handleErrors();
        return ret;
    }

    EVP_PKEY_CTX_free(ctx_params);

    // creating the context for key generation
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(dh_params, NULL);
    if (ctx == NULL)
    {
        handleErrors();
    }
    // Generating the key
    ret = EVP_PKEY_keygen_init(ctx);
    if (!ret)
    {
        handleErrors();
    }

    ret = EVP_PKEY_keygen(ctx, keypair);
    if (!ret)
    {
        handleErrors();
    }

    //check
    if (keypair == NULL)
    {
        handleErrors();
        return ret;
    }

    EVP_PKEY_CTX_free(ctx);
    return ret;
}

/** shared_key will be allocated, pass an uninitialized ptr */
int dhke(EVP_PKEY *my_key, EVP_PKEY *peer_pubkey, unsigned char *shared_key)
{
    EVP_PKEY_CTX *derivation_ctx;
    size_t shared_key_len;

    derivation_ctx = EVP_PKEY_CTX_new(my_key, NULL);
    if (derivation_ctx == NULL)
    {
        handleErrors();
    }
    if (EVP_PKEY_derive_init(derivation_ctx) <= 0)
    {
        handleErrors();
    }

    if (EVP_PKEY_derive_set_peer(derivation_ctx, peer_pubkey) <= 0)
    {
        handleErrors();
    }

    // "Dummy" derivation to extract key len
    EVP_PKEY_derive(derivation_ctx, NULL, &shared_key_len);
    shared_key = (unsigned char *)malloc(shared_key_len);
    if (!shared_key)
    {
        handleErrors();
    }

    // Real derivation
    if (EVP_PKEY_derive(derivation_ctx, shared_key, &shared_key_len) <= 0)
    {
        handleErrors();
    }

    EVP_PKEY_CTX_free(derivation_ctx);
    return 0;
}

int get_rand()
{
    RAND_poll();
    int random_num;
    RAND_bytes((unsigned char *)&random_num, sizeof(random_num));
    return random_num;
}

X509 *load_cert_file(char *file_name)
{
    FILE *cert_file = fopen(file_name, "r");
    if (!cert_file)
    {
        return NULL;
    }
    X509 *cert = PEM_read_X509(cert_file, NULL, NULL, NULL);
    fclose(cert_file);
    return cert;
}

X509_CRL *load_crl_file(char *file_name)
{
    FILE *crl_file = fopen(file_name, "r");
    if (!crl_file)
    {
        return NULL;
    }
    X509_CRL *crl = PEM_read_X509_CRL(crl_file, NULL, NULL, NULL);
    fclose(crl_file);
    return crl;
}

X509_STORE *build_store(X509 *cacert, X509_CRL *crl)
{
    X509_STORE *store = X509_STORE_new();
    if (!store)
    {
        handleErrors();
    }
    if (1 != X509_STORE_add_cert(store, cacert))
    {
        handleErrors();
    }
    if (1 != X509_STORE_add_crl(store, crl))
    {
        handleErrors();
    }
    if (1 != X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK))
    {
        handleErrors();
    }

    return store;
}

bool verify_peer_cert(X509_STORE *store, X509 *cert)
{
    X509_STORE_CTX *verify_ctx = X509_STORE_CTX_new();
    if (!verify_ctx)
    {
        handleErrors();
    }

    if (1 != X509_STORE_CTX_init(verify_ctx, store, cert, NULL))
    {
        handleErrors();
    }

    if (1 != X509_verify_cert(verify_ctx))
    {
        ERR_print_errors_fp(stderr);
        return false;
    }
    return true;
}

/** hmac will be allocated, pass an unitialized ptr */
int hmac(char *msg, int msg_len, char *key, unsigned int keylen,
         unsigned char *hmac)
{
    const EVP_MD *md = EVP_sha256();
    unsigned int hash_size = EVP_MD_size(md);
    unsigned int hmac_len;
    hmac = (unsigned char *)malloc(hash_size);

    HMAC_CTX *ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, key, keylen, md, NULL);

    HMAC_Update(ctx, (unsigned char *)msg, sizeof(msg));
    HMAC_Final(ctx, hmac, &hash_size);

    HMAC_CTX_free(ctx);
    /*
    while ((bytes_read = fread(buffer, 1, hash_size, file)) > 0)
    {
        HMAC_Update(ctx, buffer, bytes_read);
        printf("len: %d\n", bytes_read);
    }
    */
    return hash_size;
}

bool compare_hmac(unsigned char *hmac_expected, unsigned char *hmac_rcv, unsigned int len)
{
    if (0 != CRYPTO_memcmp(hmac_expected, hmac_rcv, len))
    {
        return false;
    }
    else
    {
        return true;
    }
}

void hkdf_one_info(unsigned char *key, size_t key_len,
                   unsigned char *info, size_t info_len,
                   unsigned char *out, size_t outlen)
{
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, NULL);
    if (!ctx)
    {
        handleErrors();
    }

    if (EVP_PKEY_derive_init(ctx) <= 0)
    {
        handleErrors();
    }

    if (EVP_PKEY_CTX_set_hkdf_md(ctx, EVP_sha256()) <= 0)
    {
        handleErrors();
    }
    if (EVP_PKEY_CTX_set1_hkdf_key(ctx, key, key_len) <= 0)
    {
        handleErrors();
    }
    if (EVP_PKEY_CTX_add1_hkdf_info(ctx, (unsigned char *)info, info_len) <= 0)
    {
        handleErrors();
    }
    if (EVP_PKEY_derive(ctx, out, &outlen) <= 0)
    {
        handleErrors();
    }
    EVP_PKEY_CTX_free(ctx);
}

void hkdf(unsigned char *key, size_t key_len,
          nonce_t nonce1, nonce_t nonce2,
          char *label,
          unsigned char *out, size_t outlen)
{
    // label is a string, we remove the termination null char
    size_t info_len = sizeof(nonce_t) * 2 + strlen(label);
    unsigned char *info = (unsigned char *)malloc(info_len);
    unsigned char *info_buf = info;
    strcpy((char *)info_buf, label);
    info_buf += strlen(label);
    memcpy(info_buf, (void *)&nonce1, sizeof(nonce1));
    info_buf += sizeof(nonce1);
    memcpy(info_buf, (void *)&nonce2, sizeof(nonce2));

    hkdf_one_info(key, key_len, info, info_len, out, outlen);
    free(info);
}
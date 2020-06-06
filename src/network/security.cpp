#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <string.h>

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
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /* Initialise the encryption operation. */
    if(1 != EVP_EncryptInit(ctx, EVP_aes_128_gcm(), key, iv))
        handleErrors();

    /*
     * Provide any AAD data. This can be called zero or more times as
     * required
     */
    if(1 != EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len))
        handleErrors();

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;

    /*
     * Finalise the encryption. Normally ciphertext bytes may be written at
     * this stage, but this does not occur in GCM mode
     */
    if(1 != EVP_EncryptFinal(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;

    /* Get the tag */
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
        handleErrors();

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int aes_gcm_decrypt(unsigned char *ciphertext, int ciphertext_len,
                unsigned char *aad, int aad_len,
                unsigned char *tag,
                unsigned char *key,
                unsigned char *iv,
                unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    int ret;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /* Initialise the decryption operation. */
    if(!EVP_DecryptInit(ctx, EVP_aes_128_gcm(), key, iv))
        handleErrors();

    /*
     * Provide any AAD data. This can be called zero or more times as
     * required
     */
    if(!EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len))
        handleErrors();

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary
     */
    if(!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /* Set expected tag value. Works in OpenSSL 1.0.1d and later */
    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag))
        handleErrors();

    /*
     * Finalise the decryption. A positive return value indicates success,
     * anything else is a failure - the plaintext is not trustworthy.
     */
    ret = EVP_DecryptFinal(ctx, plaintext + len, &len);

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    if(ret > 0) {
        /* Success */
        plaintext_len += len;
        return plaintext_len;
    } else {
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
    if(!ctx_params){
        handleErrors();
    }
    ret = EVP_PKEY_paramgen_init(ctx_params);
    if(!ret){
        handleErrors();
    }
    //Using NID_X9_62_prime256v1 curve
    ret = EVP_PKEY_CTX_set_ec_paramgen_curve_nid(
        ctx_params,
        NID_X9_62_prime256v1);
    if(!ret){
        handleErrors();
    }

    ret = EVP_PKEY_paramgen(ctx_params, &dh_params);
    if(!ret){
        handleErrors();
    }
    //check
    if (dh_params == NULL){
        handleErrors();
        return ret;
    }
    
    EVP_PKEY_CTX_free(ctx_params);

    // creating the context for key generation
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(dh_params, NULL);
    if(ctx == NULL){
        handleErrors();
    }
    // Generating the key
    ret = EVP_PKEY_keygen_init(ctx);
    if(!ret){
        handleErrors();
    }
    
    ret = EVP_PKEY_keygen(ctx, keypair);
    if(!ret){
        handleErrors();
    }

    //check
    if (keypair == NULL){
        handleErrors();
        return ret;
    }

    EVP_PKEY_CTX_free(ctx);
    return ret;
}


/** shared_key will be allocated, pass an uninitialized ptr */
int dhke(EVP_PKEY* my_key, EVP_PKEY* peer_pubkey, unsigned char* shared_key){
    EVP_PKEY_CTX *derivation_ctx;
    size_t shared_key_len;
    
    derivation_ctx = EVP_PKEY_CTX_new(my_key, NULL);
    if(derivation_ctx == NULL){
        handleErrors();
    }
    if(EVP_PKEY_derive_init(derivation_ctx) <= 0){
        handleErrors();
    }

    if(EVP_PKEY_derive_set_peer(derivation_ctx, peer_pubkey) <= 0){
        handleErrors();
    }

    // "Dummy" derivation to extract key len
    EVP_PKEY_derive(derivation_ctx, NULL, &shared_key_len);
    shared_key = (unsigned char*) malloc(shared_key_len);
    if(!shared_key){
        handleErrors();
    }

    // Real derivation
    if(EVP_PKEY_derive(derivation_ctx, shared_key, &shared_key_len) <= 0){
        handleErrors();
    }

    EVP_PKEY_CTX_free(derivation_ctx);
    return 0;
}

int get_rand(){
    RAND_poll();
    int random_num;
    RAND_bytes((unsigned char *)&random_num, sizeof(random_num));
    return random_num;
}
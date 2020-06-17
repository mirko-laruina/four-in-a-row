#include "security/crypto.h"
#include <cstdlib>
#include <cstring>

using namespace std;

static char aad[] = "LeChuck";
static char plaintext[] = "How much wood would a woodchuck chuck if a woodchuck could chuck wood?";
static char long_plaintext[] = "M1 A->B: \"How much wood would a woodchuck chuck if a woodchuck could chuck wood?\"\n M2A B->A: \"A woodchuck would chuck as much wood as a woodchuck could chuck if a woodchuck could chuck wood.\"\n OR        \"So much wood would a woodchuck chuck, if a woodchuck could chuck wood!\"\n OR        \"He would chuck, he would, as much as he could, and chuck as much wood as a woodchuck would if a woodchuck could chuck wood.\"";
static char key[128] = "Guybrush Threepwood";
static char iv[256];

static char certfile[] = "Your Organisation CA_cert.pem";
static char crlfile[] = "Your Organisation CA_crl.pem";

int main(){
    char ct[1024], tag[16], pt[1024];
    int ret;

    // Single block encryption/decryption
    
    ret = aes_gcm_encrypt(plaintext, strlen(plaintext)+1,
                          aad, strlen(aad)+1,
                          key, iv, 
                          ct, tag        
    );

    if (ret <= 0){
        printf("AES_GCM encryption failed");
        return 1;
    }

    ret = aes_gcm_decrypt(ct, ret,
                          aad, strlen(aad)+1,
                          key,
                          iv,
                          pt,
                          tag

    );

    if (ret <= 0){
        printf("AES_GCM decryption failed");
        return 1;
    }

    strncpy(tag, "Tag is wrong   ", 16);

    ret = aes_gcm_decrypt(ct, ret,
                          aad, strlen(aad)+1,
                          key,
                          iv,
                          pt,
                          tag

    );

    printf("%d\n", ret);

    if (ret > 0){
        printf("AES_GCM flagged wrong tag as authentic!\n");
        return 1;
    }

    // printf((char*)pt);

    if (strcmp(plaintext, pt) != 0){
        printf("AES_GCM decryption gave a wrong result");
        return 1;
    }

    // Multiple block encryption/decryption

    ret = aes_gcm_encrypt(long_plaintext, strlen(long_plaintext)+1,
                          aad, strlen(aad)+1,
                          key, iv, 
                          ct, tag        
    );

    if (ret <= 0){
        printf("AES_GCM long encryption failed");
        return 1;
    }

    ret = aes_gcm_decrypt(ct, ret,
                          aad, strlen(aad)+1,
                          key,
                          iv,
                          pt,
                          tag

    );

    if (ret <= 0){
        printf("AES_GCM long decryption failed");
        return 1;
    }

    // printf((char*)pt);

    if (strcmp(long_plaintext, pt) != 0){
        printf("AES_GCM decryption gave a wrong result\n");
        return 1;
    }

    // Generate ECDH keys
    EVP_PKEY *keyA=NULL, *keyB=NULL;
    
    ret = get_ecdh_key(&keyA);
    if (ret != 1){
        printf("ERROR: get_ecdh_key\n");
        return 1;
    }
    
    ret = get_ecdh_key(&keyB);
    if (ret != 1){
        printf("ERROR: get_ecdh_key\n");
        return 1;
    }

    // ECDH
    char *secretA=NULL, *secretB=NULL;
    int lenA, lenB;

    lenA = dhke(keyA, keyB, &secretA);
    lenB = dhke(keyA, keyB, &secretB);
    printf("DHKE secret len: %d, %d\n", lenA, lenB);
    if (lenA != lenB || memcmp(secretA, secretB, lenA) != 0){
        printf("ECDH secret is different\n");
        return 1;
    }

    // random
    int nonceA, nonceB;
    nonceA = get_rand();
    nonceB = get_rand();
    if (nonceA == nonceB){
        printf("PRNG are the same, you either won a lottery or made a mistake in the code!\n");
        return 1;
    }

    // load cert, crl and build store
    X509* cacert = load_cert_file(certfile);
    X509_CRL* crl = load_crl_file(crlfile);
    X509_STORE* store = build_store(cacert, crl);
    
    if (cacert == NULL || crl == NULL || store == NULL){
        printf("Store initialization failed!\n");
        return 1;
    }

    // verify certificate
    X509* mirko_cert = load_cert_file("mirko_cert.pem");
    X509* up_cert = load_cert_file("up_cert.pem");
    X509* mrloucipher_cert = load_cert_file("mrloucipher_cert.pem");

    if (!verify_peer_cert(store, mirko_cert)){
        printf("Valid Mirko certificate flagged as invalid!\n");
        return 1;
    }

    if (!verify_peer_cert(store, up_cert)){
        printf("Valid Up certificate flagged as invalid!\n");
        return 1;
    }

    if (verify_peer_cert(store, mrloucipher_cert)){
        printf("Invalid Mr. Lou Cipher certificate flagged as valid!\n");
        return 1;
    }

    //hmac
    char hmac1[1024], hmac2[1024];
    ret = hmac(plaintext, strlen(plaintext)+1,
                key, strlen(key)+1,
                hmac1);
    if (ret <= 0){
        printf("hmac ERROR\n");
        return 1;
    }
    ret = hmac(plaintext, strlen(plaintext)+1,
                key, strlen(key)+1,
                hmac2);
    if (ret <= 0){
        printf("hmac ERROR\n");
        return 1;
    }
    if (!compare_hmac(hmac1, hmac2, ret)){
        printf("HMAC are different!\n");
        return 1;
    }

    // hkdf
    char key1[256], key2[256];
    hkdf(key, strlen(key)+1, nonceA, nonceB,
        "elaine", key1, 256);
    hkdf(key, strlen(key)+1, nonceA, nonceB,
        "elaine", key2, 256);
    if (memcmp(key1, key2, 256) != 0){
        printf("HKDF produced different keys!\n");
        return 1;
    }
    hkdf(key, strlen(key)+1, nonceA, nonceB,
        "lechuck", key2, 256);
    if (memcmp(key1, key2, 256) == 0){
        printf("HKDF produced same key with different paramenters!\n");
        return 1;
    }

    // dsa
    EVP_PKEY* mirko_key = load_key_file("mirko_key.pem", NULL);
    char *signature;
    int sign_len = dsa_sign(plaintext, 
            strlen(plaintext)+1, 
            &signature, mirko_key
    );
    printf("Signature length: %d", sign_len);
    bool vfy = dsa_verify(plaintext, 
            strlen(plaintext)+1, 
            signature, sign_len,
            X509_get_pubkey(mirko_cert));
    if (!vfy){
        printf("DSA verify failed!\n");
        return 1;
    }
    vfy = dsa_verify(plaintext, 
            strlen(plaintext)+1, 
            signature, sign_len,
            X509_get_pubkey(up_cert));
    if (vfy){
        printf("DSA verify succeeded with wrong certificate!\n");
        return 1;
    }




    return 0;
}
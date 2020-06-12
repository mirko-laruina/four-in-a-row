#include "security/crypto_utils.h"
#include "security/crypto.h"
#include "dirent.h"

//TODO: prevent memcpy
int pkey2buf(EVP_PKEY **key, char* buf, int buflen){
    char *bio_buf;
    long len;

    BIO *bio = BIO_new(BIO_s_mem());
    if (bio == NULL){
        handleErrors();
        return -1;
    }

    if (PEM_write_bio_PUBKEY(bio, *key) != 1){
        handleErrors();
        return -1;
    }

    len = BIO_get_mem_data(bio, &bio_buf);
    if (len <= 0){
        handleErrors();
        return -1;
    }

    if (len > buflen){
        LOG(LOG_ERR, "Buffer is too small: %ld > %d", len, buflen);
        BIO_free(bio);
        return -1;
    }

    memcpy(buf, bio_buf, len);
    BIO_free(bio);

    LOG(LOG_DEBUG, "Writing %ld bytes to buffer", len);

    return len;
}


int buf2pkey(char* buf, int buflen, EVP_PKEY **key){
    BIO* bio = BIO_new_mem_buf(buf, buflen);
    if (bio == NULL){
        handleErrors();
        return -1;
    }

    if (PEM_read_bio_PUBKEY(bio, key, NULL, NULL) == NULL){
        handleErrors();
    }

    BIO_free(bio);
    return KEY_BIO_SIZE;
}

string usernameFromCert(X509* cert){
    string username;
    X509_NAME* subj_name = X509_get_subject_name(cert);
    char* subj_name_cstr = X509_NAME_oneline(subj_name, NULL, 0);

    string subj_name_str(subj_name_cstr);
    size_t pos = subj_name_str.find("/CN=");
    if (pos != string::npos){
        username = subj_name_str.substr(pos+4);
        LOG(LOG_DEBUG, "%s has size %ld", username.c_str(), username.size());
    } else{
        LOG(LOG_WARN, "Common name not found in cert: %s", subj_name_str.c_str());
        username = subj_name_str;
    }

    free(subj_name_cstr);
    return username;
}

map<string,X509*> buildCertMapFromDirectory(string dir_name){
    const char* PATTERN = "_cert.pem";
    char path[1024]; //should be always big enough
    map<string,X509*> cert_map;
    DIR *dir;
    struct dirent *ent;
    if((dir = opendir(dir_name.c_str())) != NULL) {
        while((ent = readdir (dir)) != NULL) {
            LOG(LOG_DEBUG, "%s", ent->d_name);
            if (strstr(ent->d_name, PATTERN) != NULL){
                LOG(LOG_DEBUG, "Match");
                snprintf(path, 1024, "%s/%s", dir_name.c_str(), ent->d_name);
                X509* cert = load_cert_file(path);
                string username = usernameFromCert(cert);
                cert_map.insert(pair<string,X509*>(username, cert));
            }
        }
        closedir(dir);
    } else {
        LOG(LOG_ERR, "Could not open certificate directory");
    }
    return cert_map;
}
#ifndef OPENSSL_CERT_H
#define OPENSSL_CERT_H

#include <boost/container/vector.hpp>

#include <openssl/pem.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>

class openssl_cert
{
public:
    openssl_cert(void);
    ~openssl_cert(void);
    
    bool make(boost::container::vector<unsigned char>& cert_buff, boost::container::vector<unsigned char>& key_buff);
    
private:
    X509 *x509_;
    RSA *rsa_;
    BIGNUM *bne_;
    EVP_PKEY *pkey_;
};

#endif //OPENSSL_CERT_H

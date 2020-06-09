#!/bin/bash
# This test tests the crypto primitives

dir=$(dirname $0)
cd ${dir}/security
g++ -g -lcrypto -I ../../include crypto.cpp ../../src/security/security.cpp -o test_crypto
./test_crypto
RET=$?
cd -
exit $RET


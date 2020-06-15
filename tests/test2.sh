#!/bin/bash
# This test tests single player functionality

function cleanup {
    killall client
    rm "/tmp/clientSP.out"
}
trap cleanup EXIT


dir=$(dirname $0)

cd $dir

# TODO remove certs
../dist/client/client ../certs/mirko_cert.pem ../certs/mirko_key.pem ../certs/ca_cert.pem ../certs/ca_crl.pem < test2/movesSP.txt  > /tmp/clientSP.out &

wait

output=$(grep -i "you won\|you lost\|is a draw" "/tmp/clientSP.out" | wc -l)

cd -

if [ "$output" -eq "1" ]
then  
    exit 0
else
    exit 1
fi

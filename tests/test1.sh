#!/bin/bash
# This test direct peer communication

function cleanup {
    killall client
    rm "/tmp/clientS.out"
    rm "/tmp/clientC.out"
}
trap cleanup EXIT

dir=$(dirname $0)

cd "$dir"

../dist/client/client ../certs/up_cert.pem ../certs/up_key.pem ../certs/ca_cert.pem ../certs/ca_crl.pem < test1/movesS.txt > /tmp/clientS.out &
sleep 1
../dist/client/client ../certs/mirko_cert.pem ../certs/mirko_key.pem ../certs/ca_cert.pem ../certs/ca_crl.pem < test1/movesC.txt  > /tmp/clientC.out &
wait

outputS=$(grep -i "you won"  "/tmp/clientS.out" | wc -l)
outputC=$(grep -i "you lost" "/tmp/clientC.out" | wc -l)

cd -

if [ "$outputS" -eq "1" ] && [ "$outputC" -eq "1" ]
then  
    exit 0
else
    exit 1
fi

#!/bin/bash
# This test tests multiplayer functionality with relay server

function cleanup {
    killall client server
    rm "/tmp/server.out"
    rm "/tmp/clientCA.out"
    rm "/tmp/clientCB.out"
}
trap cleanup EXIT

dir=$(dirname $0)

cd "$dir"

timeout 10 ../dist/server/server 11225 ../certs/server_cert.pem ../certs/server_key.pem ../certs/ca_cert.pem ../certs/ca_crl.pem ../certs/ > /tmp/server.out &

sleep 1

sh test3/movesCB.sh | ../dist/client/client ../certs/up_cert.pem ../certs/up_key.pem ../certs/ca_cert.pem ../certs/ca_crl.pem  > /tmp/clientCB.out &

sleep 1

sh test3/movesCA.sh | ../dist/client/client ../certs/mirko_cert.pem ../certs/mirko_key.pem ../certs/ca_cert.pem ../certs/ca_crl.pem > /tmp/clientCA.out &

wait

outputA=$(grep -i "you lost" "/tmp/clientCA.out" | wc -l)
outputB=$(grep -i "you won"  "/tmp/clientCB.out" | wc -l)

cd -

if [ "$outputA" -eq "1" ] && [ "$outputB" -eq "1" ]
then  
    exit 0
else
    exit 1
fi

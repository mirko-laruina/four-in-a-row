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

timeout 30 "$dir/../dist/server/server" 11225 > "/tmp/server.out" &

sleep 1

sh "$dir/test3/movesCB.sh" | "$dir/../dist/client/client" > "/tmp/clientCB.out" &

sleep 5

sh "$dir/test3/movesCA.sh" | "$dir/../dist/client/client" > "/tmp/clientCA.out" &

wait

outputA=$(grep -i "you lost"  "/tmp/clientCA.out" | wc -l)
outputB=$(grep -i "you won" "/tmp/clientCB.out" | wc -l)

if [ "$outputA" -eq "1" ] && [ "$outputB" -eq "1" ]
then  
    exit 0
else
    exit 1
fi

#!/bin/bash
# This test direct peer communication

function cleanup {
    killall client
    rm "/tmp/clientS.out"
    rm "/tmp/clientC.out"
}
trap cleanup EXIT

dir=$(dirname $0)

"$dir/../dist/client/client" < "$dir/test1/movesS.txt" > "/tmp/clientS.out" &
sleep 1
"$dir/../dist/client/client" < "$dir/test1/movesC.txt"  > "/tmp/clientC.out" &
wait

outputS=$(grep -i "you won"  "/tmp/clientS.out" | wc -l)
outputC=$(grep -i "you lost" "/tmp/clientC.out" | wc -l)

if [ "$outputS" -eq "1" ] && [ "$outputC" -eq "1" ]
then  
    exit 0
else
    exit 1
fi

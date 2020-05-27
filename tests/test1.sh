#!/bin/bash
# This test direct peer communication

dir=$(dirname $0)

$dir/../dist/client/client < $dir/movesS.txt > /tmp/clientS.out &
sleep 1
$dir/../dist/client/client < $dir/movesC.txt  > /tmp/clientC.out &
wait

outputS=$(grep -i "you won" /tmp/clientS.out | wc -l)
outputC=$(grep -i "you lost" /tmp/clientC.out | wc -l)

rm /tmp/clientS.out /tmp/clientC.out

if [ "$outputS" -eq "1" ] && [ "$outputC" -eq "1" ]
then  
    exit 0
else
    exit 1
fi

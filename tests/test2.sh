#!/bin/bash
# This test tests single player functionality

dir=$(dirname $0)

$dir/../dist/client/client < $dir/movesSP.txt > /tmp/clientSP.out &
wait

output=$(grep -i "you won\|you lost\|is a draw" /tmp/clientSP.out | wc -l)

# rm /tmp/clientSP.out

if [ "$output" -eq "1" ]
then  
    exit 0
else
    exit 1
fi

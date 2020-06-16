#!/bin/bash
echo "up:    Connecting to server" 1>&2
echo server 127.0.0.1 11225 ../certs/server_cert.pem
sleep 1
echo "up:    Listing users" 1>&2
echo list
sleep 3 # wait for challenge
echo "up:    Answering challenge from mirko" 1>&2
echo y
sleep 2
echo "up:    Playing" 1>&2
echo X
echo 1
echo 2
echo 3
echo 4
echo "up:    Finishing" 1>&2

#!/bin/bash
echo "mirko: Connecting to server" 1>&2
echo server 127.0.0.1 11225 ../certs/server_cert.pem
sleep 1
echo "mirko: Listing users" 1>&2
echo list
sleep 1
echo "mirko: Sending challenge to up" 1>&2
echo challenge up
sleep 2 # wait for up (USERB) to setup a serverr
echo "mirko: Connecting to up (USERB)" 1>&2
echo O
echo 1
echo 2
echo 3
echo 4
echo "mirko: Finish" 1>&2

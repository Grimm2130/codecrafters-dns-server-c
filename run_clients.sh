#!/usr/bin/env bash

SRV="bin/dns-server"
CLIENT="tests/udp_client.py"

./"$SRV" &

sleep 3

for i in {1..10}; do
    python3 "$CLIENT" &
done

wait
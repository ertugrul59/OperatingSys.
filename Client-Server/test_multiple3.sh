#!/bin/bash

### CONFIG ###
PORT=1234
LINES=3000
CLIENTS=26 #maximum 26 (=alphabet)
TIMEOUT=60
KILL_TIME="80"
##############

set -e

echo "-- test_multiple3 --"
echo "testing $CLIENTS clients with serverThreaded (input numbered and checked):"

#start server
test -x ./serverThreaded || ( echo "serverThreaded not found"; exit 1 )
test -x ./client || ( echo "client not found"; exit 1 )
./serverThreaded $PORT /tmp/logfile.txt &>/dev/null &
sleep 5

#collect all processes
set +e

#test with clients
alphabet=({a..z})
pidlist=()
for i in $(seq 0 $(( $CLIENTS-1 )) ); do
    {
        python3 -c "list(map(print, list('${alphabet[$i]}'+str(i) for i in range($LINES)) ))" | pv -L 1000 -B 10 -q | timeout --kill-after=$KILL_TIME --foreground $TIMEOUT ./client 127.0.0.1 $PORT &>/dev/null
    } &
    pidlist[$i]=$!
done


for c in ${pidlist[@]}; do
    wait $c
    if [ $? -eq 124 ]; then
        echo "client $c timeout"
    fi
done

sleep 15

#stop all background processes
for j in $(jobs -rp); do
    kill -0 $j 2>/dev/null
    if [ $? -eq 0 ]; then
        kill -PIPE $j
    fi
done
set -e

#check output
python3 ./test_check.py /tmp/logfile.txt $CLIENTS $LINES 2>&1
echo OK

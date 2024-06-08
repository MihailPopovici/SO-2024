#!/bin/bash
fis=$(realpath $1)
./subordinates/worker1 &
sleep 2
./coordonator/supervisor $fis
exit 0

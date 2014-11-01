#!/bin/bash
./echo_svr --uri=tcp://127.0.0.1:9091 --minloglevel=1 &
./echo_svr --uri=tcp://127.0.0.1:9092 --minloglevel=1 &

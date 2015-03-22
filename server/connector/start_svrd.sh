#!/bin/bash
./connector_server_test --uri=tcp://127.0.0.1:9091 --pid_file=svrd1.pid --minloglevel=1 &
./connector_server_test --uri=tcp://127.0.0.1:9092 --pid_file=svrd2.pid --minloglevel=1 &

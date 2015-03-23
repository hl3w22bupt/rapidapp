#!/bin/bash
./connector_server_test --uri=tcp://127.0.0.1:9091 --pid_file=svrd1.pid --ctrl_file=svrd1.sock --minloglevel=1 --daemon
./connector_server_test --uri=tcp://127.0.0.1:9092 --pid_file=svrd2.pid --ctrl_file=svrd2.sock --minloglevel=1 --daemon

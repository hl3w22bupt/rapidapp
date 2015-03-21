#!/bin/bash
./connector_server_test --uri=tcp://127.0.0.1:9091 --minloglevel=1 &
./connector_server_test --uri=tcp://127.0.0.1:9092 --minloglevel=1 &

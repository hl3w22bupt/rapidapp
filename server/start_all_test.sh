#!/bin/bash

./start_svrd.sh
sleep 1
./start_connector.sh
sleep 1
./start_client.sh

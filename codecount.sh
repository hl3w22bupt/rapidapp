#!/bin/bash

find ./ -path "./thirdparty/*/*" -prune -o -type f -name "*.cpp" | xargs wc -l
find ./ -path "./thirdparty/*/*" -prune -o -type f -name "*.h" | xargs wc -l

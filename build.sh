#!/usr/bin/env bash

set -e

sh clean_autogen.sh
sh autogen.sh

CFLAGS="-O3 -mtune=native -march=native -Wall" ./configure

make -j 4

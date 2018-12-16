#!/usr/bin/env bash

set -e

aclocal
autoheader
automake --foreign --add-missing --force-missing --copy
autoconf


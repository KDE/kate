#!/bin/bash

# want build dir as first parameter
BUILDDIR=$1

# go to build dir or die
cd $BUILDDIR || exit 1

# start Xvfb
Xvfb :7 -ac >/dev/null 2>&1 &

# save pid for later kill
XPID=$!

# wait bit for X
echo "Waiting for Xvfb with pid $XPID to launch..."
sleep 2

# execute tests on that screen
DISPLAY=:7 make test

# kill the poor X again
echo "Killing Xvfb with pid $XPID..."
kill $XPID

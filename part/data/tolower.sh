#!/bin/bash
# $1 will contain the correct dcop application that we are looking for.

# get code from the main application
DATAIN=`dcop $1 EditInterface#1 text`
# convert the text to all lowercase
DATAOUT=`echo $DATAIN | tr "A-Z" "a-z"`
dcop $1 EditInterface#1 setText "$DATAOUT"




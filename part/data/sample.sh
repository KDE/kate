#!/bin/bash
DATA=`ls -l`
RESULT=`dcop $1 EditInterface#1 setText "$DATA"`


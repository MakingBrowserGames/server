#!/bin/bash
LOCALE=$1
FILE=$2
DIR=$3
shift 3

ulimit -t 5
cd $DIR
LANG=en_US.ISO-8859-1 $HOME/echeck/echeck -P$HOME/echeck -w0 -u -L$LOCALE -x $FILE $* 2>&1 | iconv -f latin1 -t utf8


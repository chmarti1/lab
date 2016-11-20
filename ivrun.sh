#!/bin/bash

ERROR_FILE=ivchar.err
MONITOR_CMD=./monitor.bin
IVCHAR_CMD=./ivchar.bin

# Execute monitor with the error redirected to the error file
$MONITOR_CMD 2> $ERROR_FILE

# Execute IVCHAR
$IVCHAR_CMD 2>> $ERROR_FILE

# Rename the new data file
DATA_FILE=ivchar`date +%Y%m%d%H%M%S`.dat
mv ivchar.dat $DATA_FILE

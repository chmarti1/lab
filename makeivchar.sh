#!/bin/bash
ROOT=ivchar
DEST=build
BINARY=$DEST/$ROOT.bin
SOURCE=$ROOT.c
CONFIG=$ROOT.conf

if [ ! -d $DEST ];
then
    mkdir $DEST
else
    rm -v $BINARY
fi

# Compile source
echo -n "Compiling Source [$BINARY].."
gcc $SOURCE -lm -lLabJackM -lljacklm -o $BINARY
chmod 770 $BINARY
echo "..done."

# Look for a corresponding configuration file
if [ -f $CONFIG ];
then
    echo "Copying $CONFIG to $DEST/$CONFIG"
    cp -vf $CONFIG $DEST/$CONFIG
else
    echo "No configuration file found"
    echo "Creating an empty configuration file $DEST/$CONFIG"
    touch $DEST/$CONFIG
fi


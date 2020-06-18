#!/bin/bash

NAME="AIKA-`cat GUI/VERSION`"
rm -rf $NAME
cp -r GUI $NAME
rm -rf `find ./$NAME -name CVS`
tar cfj $NAME.tar.bz2 $NAME
rm -rf $NAME

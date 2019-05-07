#!/bin/sh
# to start hd24connect on Mac
export HD24PATH=`echo $0|sed -e 's/starthd24connect\.command//'|sed -e 's/\/$//'`
export DYLD_LIBRARY_PATH=$HD24PATH/hd24connect.app/Contents/MacOS/:$DYLD_LIBRARY_PATH
open $HD24PATH/hd24connect.app $*

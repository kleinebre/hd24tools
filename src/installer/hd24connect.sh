#!/bin/sh
export MYPATH=`dirname $0`
/usr/bin/osascript -e 'do shell script "$MYPATH/hd24connect-bin" with administrator privileges'
if [ $? == 0 ]; then
    exit 0;
fi
$MYPATH/hd24connect-bin

#!/bin/bash
PKGNAME=$1;
RELEASENUM=$2;
echo Making $1-$2_i386.deb
sudo apt-get remove hd24tools; 
rm -f *.deb *.rpm; 
if [ "`whoami`" != "root" ]; 
then 
	echo "make installer (for linux) must be run as root."; 
	exit 1; 
fi 
CURRDIR=`pwd`; 
TARGETBINDIR=usr/bin/;
TARGETSHAREDIR=usr/share/hd24tools/;
TARGETDOCDIR=usr/share/doc/hd24tools/;
rm -rf /tmp/package;
mkdir -p /tmp/package;
mkdir -p /tmp/package/$TARGETBINDIR;
mkdir -p /tmp/package/$TARGETSHAREDIR;
cp hd24connect /tmp/package/$TARGETBINDIR;
cp hd24hexview /tmp/package/$TARGETBINDIR;
cp hd24wavefix /tmp/package/$TARGETBINDIR;
cp images/longliverec.h24 /tmp/package/$TARGETSHAREDIR;
cp images/unquickformat.h24 /tmp/package/$TARGETSHAREDIR;
cd /tmp/package/;
for i in `find -type f`;
do 
        fn=`echo $i|sed -e 's#^\./##'`;
        SUM=`md5sum "$fn"`;
        echo $SUM >md5sums;
done;
echo 2.0 >/tmp/package/debian-binary;
echo Package: $PKGNAME >>control;
echo Version: $RELEASENUM >>control;
echo Architecture: i386 >>control;
echo Maintainer: Marc Brevoort \<mrjb@dnd.utwente.nl\> >>control;
echo Installed-Size: `du -sk usr|awk '{print $1}'` >> control;
echo Depends: libsndfile1 \(\>= 1.0.14\), libportaudio2 \(\>= 19\) >> control;
echo Recommends: jackd >> control;
echo Replaces: $PKGNAME >>control;
echo Provides: $PKGNAME >>control;
echo Section: multimedia >>control;
echo Priority: optional >> control;
echo Homepage: http://ringbreak.dnd.utwente.nl/~mrjb/hd24tools/ >>control;
echo Description: Permits working with Alesis ADAT HD24 drives >>control;
echo " This package is an interoperability suite which permits reading" >>control;
echo " drives formatted with Alesis' FST file system." >>control;
echo " ." >>control;
echo " Features include bidirectional transfers, audio preview, mixer," >>control;
echo " streaming to JACK, read-only recovery, and much more." >> control;
echo Original-Maintainer: Marc Brevoort \<kleinebre@hotmail.com\> >>control;
tar cfz data.tar.gz usr/;
tar cfz control.tar.gz md5sums control;
rm control md5sums;
rm -rf usr/;
ar rcs $PKGNAME-$RELEASENUM\_i386.deb debian-binary control.tar.gz data.tar.gz;
mv $PKGNAME-$RELEASENUM\_i386.deb $CURRDIR;
cd $CURRDIR;
#	sudo alien --to-rpm $PKGNAME*.deb;
	#mv $PKGNAME_$RELEASENUM-1_i386.deb $PKGNAME_$(RELEASENUM)_i386.deb
#	mv $PKGNAME-$RELEASENUM-2.i386.rpm $PKGNAME-$(RELEASENUM)_i386.rpm


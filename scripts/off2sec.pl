#!/usr/bin/perl --
# Given an offset of a byte number on disk,
# this little script converts the given offset to the sector number
# where the audio starts that is allocated by the given byte in the
# usage table.
# It is assumed that the drive usage table starts on offset 0xa00
# (sector 5).
# The blocks per cluster must be looked up in the boot record of the
# drive before using this script; it varies based on drive capacity.
my $blockspercluster=5;
my $sectorsperblock=0x480;
my $firstaudiosec=0x1397f6;
my $usagetablestart=0xa00;
my $recordingoffset=hex($ARGV[0]);
my $bitsperbyte=8;
my $startcluster=$recordingoffset-$usagetablestart;
my $audiosec=$firstaudiosec+($sectorsperblock*$blockspercluster*$bitsperbyte*$startcluster);

print sprintf("%x",$audiosec)."\n";

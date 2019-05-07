#!/usr/bin/perl --
# sl2o -> converts Sectornumber and Length to Offset
# Given a sector number and block count (as mentioned in an allocation
# entry of a song), this little script tries to calculate which
# range of byte offsets on the drive correspond with the usage
# info of that allocation entry. This is useful in combination with
# hd24hexview.
# Blocks per cluster must be configured depending on the drive capacity.
# Look up the correct number in the superblock of the drive.
use POSIX floor;
my $sectorsperblock=0x480;
my $firstaudiosec=0x1397f6;
my $usagetablestart=0xa00;
my $blockspercluster=5;
my $bitsperbyte=8;
my $audiosec=hex($ARGV[0]);
my $seclen=floor(hex($ARGV[1])/($blockspercluster*$bitsperbyte));


my $offs=($audiosec-$firstaudiosec)/0x480/$blockspercluster/$bitsperbyte+$usagetablestart;

print sprintf("%x",$offs)."-";
print sprintf("%x",$offs+$seclen)."\n";

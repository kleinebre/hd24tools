#!/usr/bin/perl --
# Given an audio sector number (based on alloc info of a song),
# this script converts that sector number to the byte offset on
# the drive where that sector number should be allocated.
# This is useful in combination with hd24hexview.
# Blocks per cluster must be looked up in the superblock of the drive;
# it varies depending on drive capacity.
my $blockspercluster=5;
my $sectorsperblock=0x480;
my $firstaudiosec=0x1397f6;
my $usagetablestart=0xa00;
my $audiosec=hex($ARGV[0]);
my $bitsperbyte=8;

my $offs=($audiosec-$firstaudiosec)/0x480/$blockspercluster/$bitsperbyte+$usagetablestart;

print sprintf("%x",$offs)."\n";

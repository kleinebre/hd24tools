#!/usr/bin/perl --

sub dec2hex {
    # parameter passed to
    # the subfunction
    my $decnum = $_[0];
	if ($decnum==0) {
	return "0";
	}
    # the final hex number
    my $hexnum;
    my $tempval;
    while ($decnum != 0) {
    # get the remainder (modulus function)
    # by dividing by 16
    $tempval = $decnum % 16;
    # convert to the appropriate letter
    # if the value is greater than 9
    if ($tempval > 9) {
    $tempval = chr($tempval + 55);
    }
    # 'concatenate' the number to 
    # what we have so far in what will
    # be the final variable
    $hexnum = $tempval . $hexnum ;
    # new actually divide by 16, and 
    # keep the integer value of the 
    # answer
    $decnum = int($decnum / 16); 
    # if we cant divide by 16, this is the
    # last step
    if ($decnum < 16) {
    # convert to letters again..
    if ($decnum > 9) {
    $decnum = chr($decnum + 55);
    }
    
    # add this onto the final answer.. 
    # reset decnum variable to zero so loop
    # will exit
    $hexnum = $decnum . $hexnum; 
    $decnum = 0 
    }
    }
    return $hexnum;
} # end sub

sub writebackupblock ($$$$) {
my $i;
	my $sector=shift;
	my $blocksize=shift;
	my $currentcount=shift;
	my $remark=shift;
	print "-0 ".$remark." ".$currentcount."\n";
	for ($i=1;$i<=$blocksize;$i++) {
		print "d".dec2hex($sector+$i-1)."\n";
		print "bb\nbe\n";
		print "d-".dec2hex($sector+1+$blocksize-$i)."\n";
		print "p\nws\n";
	}
}
$sector=0;
$blocksize=1;
$count=1;
writebackupblock($sector,$blocksize,$count,"Backup superblock");

$sector+=$blocksize*$count;
$blocksize=1;
$count=1;
writebackupblock ($sector,$blocksize,$count,"Backup Drive info");

$sector+=$blocksize*$count;
$blocksize=3;
$count=1;
writebackupblock ($sector,$blocksize,$count,"Backup undo (?) usage");

$sector+=$blocksize*$count;
$blocksize=15;
$count=1;
writebackupblock ($sector,$blocksize,$count,"Backup drive usage table");

$sector+=$blocksize*$count;
$blocksize=1;
$count=99;
for ($i=1;$i<=$count;$i++) {
	writebackupblock ($sector+$i-1,$blocksize,$i,"Backup project ");
}

$sector+=$blocksize*$count;
$count=99;
for ($i=1;$i<=$count;$i++) {
	$blocksize=2;
	writebackupblock ($sector+(7*($i-1)),$blocksize,$i,"Backup song ");
	$blocksize=5;
	writebackupblock ($sector+(7*($i-1))+2,$blocksize,$i,"Backup song alloc info ");
}
print "q\n";


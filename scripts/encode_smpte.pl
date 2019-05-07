#!/usr/bin/perl --
#
#http://en.wikipedia.org/wiki/SMPTE_time_code
sub bitnum {
	my ($val,$bit)=@_;
	while (1==1) {
		if ($bit<=0) {
			return $val % 2;
		}
		$val=int($val/2);
		$bit--;
	}
}
my $lastframe=-1;
my @smpteword;

my $prevbitnum=-1;
my $prevhalfbit=-1;
my $prevoutval=0;
sub smpte_modulate {
	my ($currbitval,$bitnum,$halfbit)=@_;
	if (($bitnum!=$prevbitnum) || (($halfbit!=$prevhalfbit) && ($currbitval==1)))
	{
		$prevoutval=1-$prevoutval;
	}
	$prevbitnum=$bitnum;
	$prevhalfbit=$halfbit;
	return $prevoutval;
}

sub smpte_timecodebit {
	my ($insamplerate,$framerate,$nondrop,$insamnum)=@_;
	my $samnum=$insamnum;
	my $samplerate=$insamplerate;
	if ($insamplerate!=48000) 
	{
		$samplerate=48000;
		$samnum=$insamnum*(48000/$insamplerate);	
	}
	my $sampleinsecond=$samnum % $samplerate;
	my $currentsecond=($samnum-$sampleinsecond)/$samplerate;
	my $currenthour=int($currentsecond/3600);
	$currentsecond-=3600*$currenthour;
	my $currentminute=int($currentsecond/60);
	$currentsecond-=60*$currentminute;
	my $bitsperframe=80;
	my $bitspersecond=$bitsperframe*$framerate;
	my $samplesperbit=$samplerate/$bitspersecond;
	my $samplesperframe=$samplerate/$framerate;
	my $currentframe=int($sampleinsecond/$samplesperframe);
	my $sampleinframe=$sampleinsecond-($currentframe*$samplesperframe);
	my $bitinframe=int($sampleinframe/$samplesperbit);
	my $subbit=int($sampleinframe-($bitinframe*$samplesperbit));
	my $halfbit=($samplesperbit/2);
	
	my $frameunits=($currentframe%10);
	my $frametens=int(($currentframe-$frameunits)/10);

	my $secondsunits=($currentsecond%10);
	my $secondstems=int(($currentsecond-$secondunits)/10);

	my $minuteunits=($currentminute%10);
	my $minutetens=int(($currentminute-$minuteunits)/10);

	my $hourunits=($currenthour%10);
	my $hourtens=int(($currenthour-$hourunits)/10);

	$smpteword[0]=bitnum($frameunits,0);
	$smpteword[1]=bitnum($frameunits,1);
	$smpteword[2]=bitnum($frameunits,2);
	$smpteword[3]=bitnum($frameunits,3);
	$smpteword[4]=0;
	$smpteword[5]=0;
	$smpteword[6]=0;
	$smpteword[7]=0;
	$smpteword[8]=bitnum($frametens,0);
	$smpteword[9]=bitnum($frametens,1);
	$smpteword[10]=0;
	$smpteword[11]=0;
	$smpteword[12]=0;
	$smpteword[13]=0;
	$smpteword[14]=0;
	$smpteword[15]=0;
	$smpteword[16]=bitnum($secondsunits,0);
	$smpteword[17]=bitnum($secondsunits,1);
	$smpteword[18]=bitnum($secondsunits,2);
	$smpteword[19]=bitnum($secondsunits,3);
	$smpteword[20]=0;
	$smpteword[21]=0;
	$smpteword[22]=0;
	$smpteword[23]=0;
	$smpteword[24]=bitnum($secondstens,0);
	$smpteword[25]=bitnum($secondstens,1);
	$smpteword[26]=bitnum($secondstens,2);
	$smpteword[27]=0; #biphasemark, fill in later
	$smpteword[28]=0;
	$smpteword[29]=0;
	$smpteword[30]=0;
	$smpteword[31]=0;
	$smpteword[32]=bitnum($minuteunits,0);
	$smpteword[33]=bitnum($minuteunits,1);
	$smpteword[34]=bitnum($minuteunits,2);
	$smpteword[35]=bitnum($minuteunits,3);
	$smpteword[36]=0;
	$smpteword[37]=0;
	$smpteword[38]=0;
	$smpteword[39]=0;
	$smpteword[40]=bitnum($minutetens,0);
	$smpteword[41]=bitnum($minutetens,1);
	$smpteword[42]=bitnum($minutetens,2);
	$smpteword[43]=0; # binary group flag bit
	$smpteword[44]=0;
	$smpteword[45]=0;
	$smpteword[46]=0;
	$smpteword[47]=0;
	$smpteword[48]=bitnum($hourunits,0);
	$smpteword[49]=bitnum($hourunits,1);
	$smpteword[50]=bitnum($hourunits,2);
	$smpteword[51]=bitnum($hourunits,3);
	$smpteword[52]=0;
	$smpteword[53]=0;
	$smpteword[54]=0;
	$smpteword[55]=0;
	$smpteword[56]=bitnum($hourtens,0);
	$smpteword[57]=bitnum($hourtens,1);
	$smpteword[58]=0; # reserved, must be 0
	$smpteword[59]=0; # binary group flag bit
	$smpteword[60]=0;
	$smpteword[61]=0;
	$smpteword[62]=0;
	$smpteword[63]=0;

        # sync word follows
	$smpteword[64]=0;
	$smpteword[65]=0;
	$smpteword[66]=1;
	$smpteword[67]=1;
	$smpteword[68]=1;
	$smpteword[69]=1;
	$smpteword[70]=1;
	$smpteword[71]=1;
	$smpteword[72]=1;
	$smpteword[73]=1;
	$smpteword[74]=1;
	$smpteword[75]=1;
	$smpteword[76]=1;
	$smpteword[77]=1;
	$smpteword[78]=0;
	$smpteword[79]=1;
	my $parity=0;
	for ($i=0;$i<=79;$i++) {
		$parity+=$smpteword[$i];
	}
	$smpteword[27]=$parity & 1;
	
	return $smpteword[$bitinframe];
	# Enable this to also perform bit modulation:
	# 	###########################
	if ($subbit<$halfbit) {
		return smpte_modulate($smpteword[$bitinframe],$bitinframe,0);
	}
	return smpte_modulate($smpteword[$bitinframe],$bitinframe,1);
	##################################
	# or only use this to return the raw SMPTE bits.
}

my $i;
my $samplerate=48000;
my $framerate=30;
$startsam=0; #($samplerate/$framerate);
my $nondrop=true;
for ($samnum=$startsam;$samnum<1500000;$samnum+=10) {
	$x=smpte_timecodebit($samplerate,$framerate,$nondrop,$samnum);
	if ($x==0) {
		print "0"; #chr(0);
	} else {
		print "1"; #chr(255);
	}

}
	

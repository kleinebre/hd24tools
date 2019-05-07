What is the HD24tools suite?
============================

The HD24tools suite is a simple software suite that allows 
Linux users to read disks of their Alesis HD24 ADAT recorders. 
Developed in a cross-platform manner, it currently also compiles 
for Windows and Mac, providing users with a free alternative to 
Alesis' offering.

Open source policy
==================

The software was written as an independent effort. It contains
no code by Alesis, and is not supported by Alesis.

In the development of this software, no Alesis-confidential
information was used. Instead, the necessary parts were 
reverse-engineered, which is legal in the Netherlands.

However, as a result, the correctness nor completeness of the 
file system specifications can not be guaranteed. 
Therefore, you are reminded that you use this software at your
own risk.

AnalogFilter Class
==================
The author of ZynAddSubFX has granted me permission to use his
AnalogFilter class in HD24tools in closed-source form:

"You may use the AnalogFilter class, but please acknowledge the 
copyright of the AnalogFilter class (and tell that I give you a 
specific permission to do so, to avoid confusion about GPL 2 
license)"

This permission is no longer relevant because
1. I never ended up using this class; and
2. HD24tools is now open-source.

General operation
=================
Most of the programs automatically detect and access the presence 
of HD24 disks.

If multiple HD24 disks are present, the first HD24 disk found will
be used by default. The tools search for HD24 disks in the following 
order:

/dev/hda,/dev/hdb,/dev/hdc,/dev/hdd
/dev/sda,/dev/sdb,/dev/sdc,/dev/sdd

This should find most HD24 disks, regardless of the way that they
are connected to your PC (be it using a VIPowER drive bay, USB
connection or firewire interface). Should you be using another
device as disk, you can use the --dev=x option to point to
HD24 device with device path x. 

To be able to use the tools, the user running the programs must
have direct read access to the disk device. It is possible to
re-assign access rights of the disk device each time, but it
is probably more practical to update your group permissions and/or
add an entry to your /etc/fstab, for instance:

/dev/hdc        none            auto    devmode=0664    0       0

(I have not extensively tested this for optimal security).
You can ignore any boot time complaints about linux not being able
to mount the drive; it will try all the file system types it 
knows but of course doesn't know how to work with HD24 drives.

When using an internal drivebay, keep in mind that it may not
support hot-swapping. You probably need to switch off power to
exchange drives. In other words, if you fry your motherboard, 
don't complain to me.

The HD24 only accepts drives configured as master. As such, if
you use an internal drive bay, you probably end up with the HD24
disk as /dev/hdc. 

The programs
============
hd24hexview	A debug-style hex viewer for (binary files and) hd24 disks.
		
		Without command line arguments, accesses the first
		found hd24 disk in hex mode. 

		Type ? in the program for help.

		Call the program with --dev=/dev/hdc to use
		/dev/hdc as hd24 disk. If this disk is not
		recognized as hd24 disk, use the --force option
		to view the disk contents anyway. Use --expert
		to enable write mode.

		All other programs that directly access the disk
		support the --dev=x option but not the --force 
		option.

hd24connect	A program to download files from HD24 disks.

hd24info	A program that displays some info about the
		hd24 disk. Mostly useful to detect the presence
		or absence of hd24 disks.
		(obsolete)

hd24wavefix	A program that attempts to fix corrupted audio that
		sometimes occurs in long live recordings, usually when
 		recording to very high capacity drives.

genbackupscript.pl
		
		A perl script to generate a script for hd24hexview
		which will create a backup of the file system information
		to the end of the drive. Now obsolete; this functionality
		is now built-in into the HD24 library.

syx2bin		A program that attempts to decode the 7 bit HD24 .syx file
		to a 8 bit binary file.

hd24towav.cpp	A program that converts raw hd24 data to wav files.
		(obsolete)



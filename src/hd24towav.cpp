#ifdef WINDOWS
namespace std {}
#endif
using namespace std;
#include <string>
#include <iostream>
#include "convertlib.h"
/*
** Copyright (C) 2001-2005 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include	<stdio.h>

/* Include this header file to use functions from libsndfile. */
#include	<sndfile.h>

/*    This will be the length of the buffer used to hold.frames while
**    we process them.
*/
#define		BUFFER_LEN	1024

/* libsndfile can handle more than 6 channels but we'll restrict it to 6. */
#define		MAX_CHANNELS	6
string inputfilename;
string outputfilename;
string format;
long rate;
long bits;
int convertfile(string inputfilename) {
/* This is a buffer of double precision floating point values
 * which will hold our data while we process it.
 */
	format="wav";
	cout << "Converting " << inputfilename << endl;
    if (rate==0) {
	if (
		(inputfilename.find("44k1",0) != string::npos)
		||(inputfilename.find("44K1",0) != string::npos)
		||(inputfilename.find("44100",0) != string::npos)
	) {
		rate=44100;
	}
	if (
		(inputfilename.find("48k",0) != string::npos)
		||(inputfilename.find("48K",0) != string::npos)
		||(inputfilename.find("48000",0) != string::npos)
	) {
		rate=48000;
	}
	if (
		(inputfilename.find("88k2",0) != string::npos)
		||(inputfilename.find("88K2",0) != string::npos)
		||(inputfilename.find("88200",0) != string::npos)
	) {
		rate=88200;
	}
	if (
		(inputfilename.find("96k",0) != string::npos)
		||(inputfilename.find("96K",0) != string::npos)
		||(inputfilename.find("96000",0) != string::npos)
	) {
		rate=96000;
	}
    }
    bits=24;
    static int data [BUFFER_LEN] ;

    /* A SNDFILE is very much like a FILE in the Standard C library. The
    ** sf_open function return an SNDFILE* pointer when they sucessfully
	** open the specified file.
    */
    SNDFILE      *infile, *outfile ;

    /* A pointer to an SF_INFO stutct is passed to sf_open.
    ** On read, the library fills this struct with information about the file.
    ** On write, the struct must be filled in before calling sf_open.
    */
    SF_INFO		sfinfoin ;
    SF_INFO		sfinfoout;
    int			readcount ;
	if (inputfilename=="") {
		cout << "Must specify --input filename" << endl;
		return 1;
	}
	if (outputfilename=="") {
		outputfilename=inputfilename;
		string ext="";
		if (format=="wav") {
			ext=".wav";
		}
		if (outputfilename.substr(outputfilename.length()-4,4)==".raw") {
			outputfilename=outputfilename.substr(0,outputfilename.length()-4)+ext;
		}
		if (outputfilename=="") {
			cout << "Must specify --output filename" << endl;
			return 1;
		}
	}
    const char	*infilename = inputfilename.c_str();
    const char	*outfilename = outputfilename.c_str();

    /* Here's where we open the input file. We pass sf_open the file name and
    ** a pointer to an SF_INFO struct.
    ** On successful open, sf_open returns a SNDFILE* pointer which is used
    ** for all subsequent operations on that file.
    ** If an error occurs during sf_open, the function returns a NULL pointer.
	**
	** If you are trying to open a raw headerless file you will need to set the
	** format and channels fields of sfinfo before calling sf_open(). For
	** instance to open a raw 16 bit stereo PCM file you would need the following
	** two lines:
	**
	**		sfinfo.format   = SF_FORMAT_RAW | SF_FORMAT_PCM_16 ;
	**		sfinfo.channels = 2 ;
    */
	sfinfoin.format   = SF_FORMAT_RAW | SF_FORMAT_PCM_24|SF_ENDIAN_BIG ;
	sfinfoin.channels = 1 ;

	if (format=="wav") {
		sfinfoout.format   = SF_FORMAT_WAV ;
	}
	if (bits==16) {
		sfinfoout.format|=SF_FORMAT_PCM_16;
	}
	if (bits==24) {
		sfinfoout.format|=SF_FORMAT_PCM_24;
	}

	sfinfoout.samplerate=rate;
	sfinfoout.channels = 1 ;
    if (! (infile = sf_open (infilename, SFM_READ, &sfinfoin)))
    {   /* Open failed so print an error message. */
        printf ("Not able to open input file %s.\n", infilename) ;
        /* Print the error message from libsndfile. */
        puts (sf_strerror (NULL)) ;
        return  1 ;
        } ;

    /* Open the output file. */
    if (! (outfile = sf_open (outfilename, SFM_WRITE, &sfinfoout)))
    {   printf ("Not able to open output file %s.\n", outfilename) ;
        puts (sf_strerror (NULL)) ;
        return  1 ;
        } ;

    /* While there are.frames in the input file, read them, process
    ** them and write them to the output file.
    */
    while ((readcount = sf_read_int (infile, data, BUFFER_LEN)))
    {   
        sf_write_int (outfile, data, readcount) ;
        } ;

    /* Close input and output files. */
    sf_close (infile) ;
    sf_close (outfile) ;
	return 0;
}

int parsecommandline(int argc, char **argv) 
{
	int invalid = 0;
	
	for (int c = 1; c < argc; c++) {
		string arg = argv[c];
		
		if (arg.substr(0, 1) != "-") {
			inputfilename = arg;
			outputfilename = "";
			convertfile(inputfilename);		
			continue;
		}
		
		if (arg.substr(0,strlen("--input=")) == "--input=") {
			inputfilename = arg.substr(strlen("--input="));
			continue;
		}
		
		if (arg.substr(0,strlen("--output=")) == "--output=") {
			outputfilename = arg.substr(strlen("--output="));
			continue;
		}
		
		if (arg.substr(0,strlen("--rate=")) == "--rate=") {
			rate = Convert::str2long(arg.substr(strlen("--rate=")));
			continue;
		}
		
		if (arg.substr(0,strlen("--bits=")) == "--bits=") {
			bits = Convert::str2long(arg.substr(strlen("--bits=")));
			continue;
		}

		cout << "Invalid argument: " << arg << endl;
		invalid = 1;
	}
	return invalid;
}

int main (int argc, char **argv) 
{
	rate = 0;
	int invalid = parsecommandline(argc, argv);
	
	if (invalid!=0) {
		return invalid;
	}

    return 0 ;
} /* main */


/*
** Do not edit or modify anything in this comment block.
** The arch-tag line is a file identity tag for the GNU Arch
** revision control system.
**
** arch-tag: de9fdd1e-b807-41ef-9d51-075ba383e536
*/

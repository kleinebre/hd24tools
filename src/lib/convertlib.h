#ifndef __convert_h__
#define __convert_h__

using namespace std;

#include "config.h"
#include <string>
#include <iostream>

class Convert 
{
	public:
		Convert() {};
		~Convert() {};
		
		static bool		isnibble(string x);
		static long		str2long(string hexstr);
		static double		str2dbl(string hexstr);
		static long		hex2long(string hexstr);
		static string*		byte2hex(unsigned char x);
		static string*		int2str(int x,unsigned int pad,string padchar);
		static string*		int2str(int x);
		static string*		int32tostr(__uint32 x);
		static string*		int64tostr(__sint64 x);
		static string*		int64tohex(__sint64 x);
		static string*		int32tohex(unsigned long x);
		static string*		readstring(unsigned char * orig,int offset,int len);
		static string*		padright(string & strinput,int inlen,string strpad);
		static string*		padleft(string & strinput,int inlen,string strpad);
		static unsigned int	getint32(unsigned char * buf,int loc);
		static unsigned int	getint24(unsigned char * buf,int loc);
		static void		setint32(unsigned char * buf,int loc,__uint32 newval);
		static void 		setfloat80(unsigned char * buf, int loc, __uint32 newval);	/* Only for use in AIFF files */
		static unsigned char	hex2byte(string hexstr);
		static unsigned char	safebyte(unsigned char x);
		static string*		trim(string* strinput);
};

#endif

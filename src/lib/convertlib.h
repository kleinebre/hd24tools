#ifndef __convert_h__
#define __convert_h__

using namespace std;

#include <stdint.h>
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
		static string*		int2str(int x,uint32_t pad,string padchar);
		static string*		int2str(int x);
		static string*		int32tostr(uint32_t x);
		static string*		int64tostr(int64_t x);
		static string*		int64tohex(int64_t x);
		static string*		int32tohex(uint32_t x);
		static string*		readstring(unsigned char * orig,int offset,int len);
		static string*		padright(string & strinput,int inlen,string strpad);
		static string*		padleft(string & strinput,int inlen,string strpad);
		static uint32_t	getint32(unsigned char * buf,int loc);
		static uint32_t	getint24(unsigned char * buf,int loc);
		static void		setint32(unsigned char * buf,int loc,uint32_t newval);
		static void 		setfloat80(unsigned char * buf, int loc, uint32_t newval);	/* Only for use in AIFF files */
		static unsigned char	hex2byte(string hexstr);
		static unsigned char	safebyte(unsigned char x);
		static string*		trim(string* strinput);
};

#endif

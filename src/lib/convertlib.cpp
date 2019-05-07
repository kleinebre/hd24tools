#include <stdlib.h>
#include "convertlib.h"

bool Convert::isnibble(string x) 
{
	if ((x >= "0") && (x <= "9")) return true;
	if ((x >= "a") && (x <= "f")) return true;
	if ((x >= "A") && (x <= "F")) return true;
	return false;
}

string* Convert::byte2hex(unsigned char x) 
{
	int b = x % 16;
	int a = x >> 4;
	
	string hex = "0123456789ABCDEF";
	string * newst = new string("");
	*newst = *newst + hex.substr(a, 1);
	*newst = *newst + hex.substr(b, 1);
	return newst;
}

long Convert::str2long(string decstr) 
{
	return strtol(decstr.c_str(), 0, 10);
}

double Convert::str2dbl(string decstr) 
{
	return strtod(decstr.c_str(), 0);
}

string* Convert::int2str(int x, unsigned int pad, string padchar) 
{
	string* newst = int2str(x);
	
	if (pad > 1) 
	{
		while (newst->length() < pad) 
		{
			*newst = padchar + *newst;
		}
	}
	
	return newst;
}

string* Convert::int64tostr(__sint64 x) 
{
	bool isneg=false;
	string * newst = new string("");
	if (x == 0) 
	{
		*newst = "0";
	} 
	else 
	{
		if (x<0)
		{
			isneg=true;
			x=0-x;
		}
		while (x != 0) 
		{
			long long digit = x % (long long) 10;
			string dec = "0123456789";
			*newst = dec.substr(digit, 1) + *newst;
			
			x -= (long long) (x % (long long) 10);
			x /= (long long) 10;
		}
	}
	if (isneg)
	{
		*newst="-"+*newst;
	}
	return newst;
}

string* Convert::int2str(int x) 
{
	long long y = x;
	return int64tostr(y);
}

string* Convert::int32tostr(__uint32 x) 
{
	long long y = x;
	return int64tostr(y);
}

long Convert::hex2long(string hexstr) 
{
	return strtol(hexstr.c_str(), 0, 16);
}

unsigned char Convert::hex2byte(string hexstr) 
{
	long x = hex2long(hexstr);
	return (unsigned char) x % 256;
}

unsigned char Convert::safebyte(unsigned char x) 
{
	if (x < 32) return '.';
	return x;
}

string* Convert::int64tohex(__sint64 x) 
{
	__sint64 q = x;
	unsigned char a = q % 256; q = q >> 8;
	unsigned char b = q % 256; q = q >> 8;
	unsigned char c = q % 256; q = q >> 8;
	unsigned char d = q % 256; q = q >> 8;
	unsigned char e = q % 256; q = q >> 8;
	unsigned char f = q % 256; q = q >> 8;
	unsigned char g = q % 256; q = q >> 8;
	unsigned char h = q % 256; 
	string* b2h8 = byte2hex(h);
	string* b2h7 = byte2hex(g);
	string* b2h6 = byte2hex(f);
	string* b2h5 = byte2hex(e);
	string* b2h4 = byte2hex(d);
	string* b2h3 = byte2hex(c);
	string* b2h2 = byte2hex(b);
	string* b2h1 = byte2hex(a);
	string* newst = new string();
	*newst += *b2h8 + *b2h7 + ":" + *b2h6 + *b2h5 + "." + *b2h4 + *b2h3 + ":" + *b2h2 + *b2h1;
	delete (b2h1);
	delete (b2h2);
	delete (b2h3);
	delete (b2h4);
	delete (b2h5);
	delete (b2h6);
	delete (b2h7);
	delete (b2h8);
	return newst;
}

string* Convert::int32tohex(unsigned long x) 
{
	__sint64 q = x;
	unsigned char a = q % 256; q = q >> 8;
	unsigned char b = q % 256; q = q >> 8;
	unsigned char c = q % 256; q = q >> 8;
	unsigned char d = q % 256; q = q >> 8;
	string* b2h4 = byte2hex(d);
	string* b2h3 = byte2hex(c);
	string* b2h2 = byte2hex(b);
	string* b2h1 = byte2hex(a);
	string* newst = new string();
	*newst += *b2h4 + *b2h3 + ":" + *b2h2 + *b2h1;
	delete (b2h1);
	delete (b2h2);
	delete (b2h3);
	delete (b2h4);
	return newst;
}

unsigned int Convert::getint24(unsigned char * buf, int loc) 
{
	unsigned int q = 0;

	q = buf[loc + 2] 
		+ (buf[loc + 1] << 8) 
		+ (buf [loc + 0] << 16);
	return q;
}


unsigned int Convert::getint32(unsigned char * buf, int loc) 
{
	unsigned int q = 0;

	q = buf[loc + 3] 
		+ (buf[loc + 2] << 8) 
		+ (buf [loc + 1] << 16)
		+ (buf [loc] << 24); 
	return q;
}

void Convert::setint32(unsigned char * buf, int loc, __uint32 newval)
{
        buf[loc + 0] = (newval >> 24) % 256;
        buf[loc + 1] = (newval >> 16) % 256;
        buf[loc + 2] = (newval >> 8) % 256;
        buf[loc + 3] = newval % 256;
}

void Convert::setfloat80(unsigned char * buf, int loc, __uint32 newval)
{
	/* Thanks, Erik */
        unsigned int mask = 0x40000000 ;
        int     count ;

	for (int i=0;i<10;i++) 
	{
		buf[loc+i]=0;
	}
        if (newval <= 1)
        {       buf[loc+0] = 0x3F ;
                buf[loc+1] = 0xFF ;
                buf[loc+2] = 0x80 ;
                return ;
                } ;

        buf[loc+0] = 0x40 ;

        if (newval >= mask)
        {       buf[loc+1] = 0x1D ;
                return ;
                } ;

        for (count = 0 ; count <= 32 ; count ++)
        {       if (newval & mask)
                        break ;
                mask >>= 1 ;
                } ;

        newval <<= count + 1 ;
        buf[loc+1] = 29 - count ;
        buf[loc+2] = (newval >> 24) & 0xFF ;
        buf[loc+3] = (newval >> 16) & 0xFF ;
        buf[loc+4] = (newval >> 8) & 0xFF ;
        buf[loc+5] = newval & 0xFF ;
	return;
}

string* Convert::readstring(unsigned char * orig, int offset, int len) 
{
	string * newst = new string("");
	int i;
	
	for (i = offset; i < offset + len; i++) 
	{
		if (i >= offset + len)
			return newst;

		if (orig[i] == 0) break;
		*newst += orig[i];
	}
	
	return newst;
}

string* Convert::padright(string & strinput, int inlen, string strpad) 
{
	int currlen = strinput.length();

	if (currlen > inlen) 
	{
                strinput=strinput.substr(0,inlen);
		return new string(strinput);
	}
	
	int pad = inlen - currlen;
	if (pad > 0)
	{
		for (int i = 0; i < pad; i++) 
		{
			strinput += strpad;
		}
	}
	
	return new string(strinput);
}

string* Convert::padleft(string & strinput, int inlen, string strpad) 
{
	int currlen = strinput.length();

	if (currlen > inlen) 
	{
		return new string(strinput.substr(0, inlen));
	}
	
	int pad = inlen - currlen;
	if (pad > 0)
	{
		string strtmp = "";
		for (int i = 0; i < pad; i++) 
		{
			strtmp += strpad;
		}
		strinput = strtmp + strinput;
	}
	return new string(strinput);
}

string* Convert::trim(string* strinput)
{
	if (strinput->length() == 0) 
	{
	   string* x = new string("");
	   return x;
	}

	string* strresult = new string(*strinput);
	while (strresult->substr(0, 1) == " ") 
	{
 		*strresult = strresult->substr(1, strresult->length() - 1);
	}

	while (strresult->substr(strresult->length() - 1, 1) == " ") 
	{
		*strresult = strresult->substr(0, strresult->length() - 1);
	}
	return strresult;
}

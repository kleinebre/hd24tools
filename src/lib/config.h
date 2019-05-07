#ifdef WINDOWS
#	define __sint64 signed __int64
#	define __uint64 unsigned __int64
#else
#	define __sint64 signed long long
#	define __uint64 unsigned long long
#endif

#define __sint32 signed long
#define __uint32 unsigned long


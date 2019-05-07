#ifdef LIBJACK
#	include <jack/jack.h>
#	include <jack/transport.h>
#else
#	include "../lib/nojack.h"
#endif

#ifdef LIBPORTAUDIO
#	include "portaudio.h"
#	define portaudiostreamtype PaStream
#else
#	define portaudiostreamtype void
#	define PaTime unsigned long
#endif
#define PaUnanticipatedHostError -9999

//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Dec 22 20:12:18 PST 1998
// Last Modified: Tue Dec 22 20:12:23 PST 1998
// Filename:      insimple.c
// Syntax:        C; pthread
// $Smake:        gcc -O3 -Wall -o %b %f -static -lpthread && strip %b 
//

#include <sys/soundcard.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define  MIDI_DEVICE  "/dev/sequencer"


int main(void) {
   unsigned char inpacket[4];

   // first open the sequencer device for reading.
   int seqfd = open(MIDI_DEVICE, O_RDONLY);
   if (seqfd == -1) {
      printf("Error: cannot open %s\n", MIDI_DEVICE);
      exit(1);
   } 

   // now just wait around for MIDI bytes to arrive and print them to screen.
   while (1) {
      read(seqfd, &inpacket, sizeof(inpacket));
 
      // print the MIDI byte if this input packet contains one
      if (inpacket[0] == SEQ_MIDIPUTC) {
         printf("received MIDI byte: %d\n", inpacket[1]);
      }
   }
      
   return 0;
}


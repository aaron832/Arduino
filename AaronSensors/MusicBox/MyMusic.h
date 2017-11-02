#ifndef MyMusic_h
#define MyMusic_h

#include "Playtune.h"

// Playtune bytestream for file "pb2.mid" created by MIDITONES V1.6 on Tue Oct 31 19:56:16 2017
// command line: miditonesV1.6.exe pb2 
const byte PROGMEM pbscore [] = {
7,83, 0x90,69, 1,212, 0x80, 0x90,69, 1,212, 0x80, 0x90,67, 1,212, 0x80, 0x90,69, 5,126, 
0x80, 0x90,65, 1,212, 0x80, 0x90,70, 3,169, 0x80, 0x90,70, 1,212, 0x80, 0x90,72, 1,212, 0x80, 
0x90,69, 0,234, 0x80, 0x90,67, 0,234, 0x80, 0x90,65, 7,83, 0x80, 0x90,69, 1,212, 0x80, 0x90,69, 
1,212, 0x80, 0x90,67, 1,212, 0x80, 0x90,69, 5,126, 0x80, 0x90,65, 1,212, 0x80, 0x90,67, 1,212, 
0x80, 0x90,67, 3,169, 0x80, 0x90,65, 1,212, 0x80, 0x90,69, 5,126, 0x80, 0xf0};
// This score contains 102 bytes, and 1 tone generator is used.

// Playtune bytestream for file "koko-short.mid" created by MIDITONES V1.6 on Wed Nov  1 18:55:27 2017
// command line: miditonesV1.6.exe koko-short 
const byte PROGMEM kokoscore [] = {
0x90,67, 0,0, 0x80, 0,0, 0x90,67, 0,0, 0x80, 0,0, 0x90,67, 0,0, 0x80, 0,0, 
0x90,69, 0,0, 0x80, 0,0, 0x90,71, 0,0, 0x80, 0,0, 0x90,67, 0,0, 0x80, 0x90,65, 
0,0, 0x80, 0x90,64, 0,0, 0x80, 0,0, 0x90,65, 0,0, 0x80, 0,0, 0x90,65, 0,0, 
0x80, 0,0, 0x90,65, 0,0, 0x80, 0,0, 0x90,67, 0,0, 0x80, 0,0, 0x90,69, 0,0, 
0x80, 0,0, 0x90,67, 0,0, 0x80, 0,0, 0x90,65, 0,0, 0x80, 0,0, 0x90,60, 0,0, 
0x80, 0,0, 0x90,65, 0,0, 0x80, 0,0, 0x90,67, 0,0, 0x80, 0,0, 0x90,65, 0,0, 
0x80, 0,0, 0x90,65, 0,0, 0x80, 0,0, 0x90,64, 0,0, 0x80, 0,0, 0x90,64, 0,0, 
0x80, 0,0, 0x90,64, 0,0, 0x80, 0,0, 0x90,65, 0,0, 0x80, 0,0, 0x90,67, 0,0, 
0x80, 0,0, 0x90,69, 0,0, 0x80, 0,0, 0x90,62, 0,0, 0x80, 0,0, 0x90,62, 0,0, 
0x80, 0,0, 0x90,62, 0,0, 0x80, 0,0, 0x90,65, 0,0, 0x80, 0,0, 0x90,64, 0,0, 
0x80, 0,0, 0x90,65, 0,0, 0x80, 0x90,64, 0,0, 0x80, 0,0, 0x90,60, 0,0, 0x80, 0,0, 
0x90,60, 0,0, 0x80, 0xf0};
// This score contains 237 bytes, and 1 tone generator is used.

Playtune pt;

void MusicStartLogic()
{
	pt.tune_initchan (4); 
	//pt.tune_initchan (5);
	
	pt.tune_playscore (pbscore); /* start playing */
}

void MusicStopLogic()
{
	pt.tune_stopscore();
	pt.tune_stopchans();
}

#endif //MyMusic_h

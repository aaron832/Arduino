#ifndef MyMusic_h
#define MyMusic_h

#include "Playtune.h"

const byte PROGMEM pbscore [] = {
7,83, 0x90,69, 1,212, 0x80, 0x90,69, 1,212, 0x80, 0x90,67, 1,212, 0x80, 0x90,69, 5,126, 
0x80, 0x90,65, 1,212, 0x80, 0x90,70, 3,169, 0x80, 0x90,70, 1,212, 0x80, 0x90,72, 1,212, 0x80, 
0x90,69, 0,234, 0x80, 0x90,67, 0,234, 0x80, 0x90,65, 7,83, 0x80, 0x90,69, 1,212, 0x80, 0x90,69, 
1,212, 0x80, 0x90,67, 1,212, 0x80, 0x90,69, 5,126, 0x80, 0x90,65, 1,212, 0x80, 0x90,67, 1,212, 
0x80, 0x90,67, 3,169, 0x80, 0x90,65, 1,212, 0x80, 0x90,69, 5,126, 0x80, 0xf0};

// Playtune bytestream for file "bach_bwv798.mid" created by MIDITONES V1.1 on Sun Feb 27 12:45:00 2011
const byte PROGMEM score [] = {
0,2, 0x90,59, 0,2, 0x91,51, 0x92,40, 0,13, 0x81, 0,186, 0x82, 0x91,51, 0,4, 0x92,42, 
0,19, 0x81, 0,70, 0x82, 0,12, 0x91,42, 0,9, 0x92,51, 0,13, 0x82, 0,23, 0x81, 0,47, 
0x91,51, 0,2, 0x92,42, 0,31, 0x81, 0,140, 0x82, 0,34, 0x91,42, 0,20, 0x92,51, 0,13, 
0x82, 0,79, 0x92,51, 0,24, 0x82, 0,76, 0x80, 0,13, 0x90,60, 0,1, 0x92,51, 0,13, 
0x82, 0,145, 0x81, 0,44, 0x91,40, 0,6, 0x92,51, 0,13, 0x82, 0,90, 0x92,51, 0,13, 
0x82, 0,67, 0x81, 0,15, 0x91,40, 0,9, 0x92,51, 0,13, 0x82, 0,167, 0x81, 0,14, 0x91,40, 
0,2, 0x92,51, 0,24, 0x82, 0,91, 0x92,51, 0,13, 0x82, 0,44, 0x81, 0,34, 0x80, 0,3, 
0x90,61, 0,3, 0x91,40, 0,4, 0x92,51, 0,13, 0x82, 0,147, 0x81, 0,41, 0x91,43, 0,6, 
0x92,51, 0,13, 0x82, 0,61, 0x81, 0,21, 0x91,43, 0,8, 0x92,51, 0,13, 0x82, 0,49, 
0x81, 0,31, 0x91,43, 0,10, 0x92,51, 0,13, 0x82, 0,171, 0x81, 0,13, 0x91,43, 0,13, 
0x92,51, 0,13, 0x82, 0,91, 0x92,51, 0,13, 0x82, 0,83, 0x80, 0,1, 0x90,60, 0,6, 
0x92,51, 0,13, 0x82, 0,147, 0x81, 0,41, 0x91,42, 0,6, 0x92,51, 0,13, 0x82, 0,91, 
0x92,51, 0,13, 0x82, 0,53, 0x81, 0,36, 0x91,42, 0,1, 0x92,51, 0,13, 0x82, 0,179, 
0x81, 0,11, 0x91,42, 0,6, 0x92,51, 0,12, 0x82, 0,92, 0x92,51, 0,13, 0x82, 0,53, 
0x81, 0,28, 0x91,40, 0,2, 0x80, 0,1, 0x90,59, 0,5, 0x92,51, 0,13, 0x82, 0,178, 
0x81, 0,13, 0x91,42, 0,5, 0x92,51, 0,13, 0x82, 0,48, 0x81, 0,35, 0x91,42, 0,6, 
0x92,51, 0,13, 0x82, 0,42, 0x81, 0,31, 0x91,42, 0,16, 0x92,51, 0,13, 0x82, 0,161, 
0x81, 0,26, 0x91,42, 0,8, 0x92,51, 0,13, 0x82, 0,91, 0x92,51, 0,13, 0x82, 0,83, 
0x80, 0,2, 0x90,60, 0,6, 0x92,51, 0,12, 0x82, 0,157, 0x81, 0,27, 0x91,40, 0,12, 
0x92,51, 0,13, 0x82, 0,91, 0x92,51, 0,13, 0x82, 0,55, 0x81, 0,21, 0x91,40, 0,13, 
0x92,51, 0,13, 0x82, 0,162, 0x81, 0,22, 0x91,40, 0,10, 0x92,51, 0,13, 0x82, 0,91, 
0x92,51, 0,13, 0x82, 0,54, 0x81, 0,30, 0x80, 0x90,40, 0,3, 0x91,61, 0,3, 0x92,51, 
0,13, 0x82, 0,142, 0x80, 0,45, 0x90,43, 0,9, 0x92,51, 0,13, 0x82, 0,34, 0x80, 0,43, 
0x90,43, 0,13, 0x92,51, 0,13, 0x82, 0,43, 0x80, 0,13, 0x90,43, 0,34, 0x92,51, 0,13, 
0x82, 0,167, 0x80, 0,20, 0x90,43, 0,6, 0x92,51, 0,13, 0x82, 0,90, 0x92,51, 0,13, 
0x82, 0,90, 0x81, 0,1, 0x91,51, 0x92,60, 0,13, 0x81, 0,126, 0x80, 0,66, 0x90,42, 0,3, 
0x91,51, 0,13, 0x81, 0,92, 0x91,51, 0,12, 0x81, 0,41, 0x80, 0,41, 0x90,42, 0,9, 
0x91,51, 0,13, 0x81, 0,161, 0x80, 0,27, 0x90,42, 0,6, 0x91,51, 0,13, 0x81, 0,90, 
0x91,51, 0,13, 0x81, 0,61, 0x80, 0,25, 0x82, 0,2, 0x90,40, 0x91,59, 0,1, 0x92,51, 
0,13, 0x82, 0,167, 0x80, 0,20, 0x90,42, 0,8, 0x92,51, 0,13, 0x82, 0,43, 0x80, 0,31, 
0x90,42, 0,16, 0x92,51, 0,13, 0x82, 0,39, 0x80, 0,21, 0x90,42, 0,31, 0x92,51, 0,12, 
0x82, 0,142, 0x80, 0,28, 0x90,42, 0,26, 0x92,51, 0,13, 0x82, 0,91, 0x92,51, 0,13, 
0x82, 0,83, 0x81, 0,1, 0x91,60, 0,6, 0x92,51, 0,13, 0x82, 0,162, 0x80, 0,26, 0x90,40, 
0,6, 0x92,51, 0,13, 0x82, 0,91, 0x92,51, 0,13, 0x82, 0,66, 0x80, 0,14, 0x90,40, 
0,10, 0x92,51, 0,13, 0x82, 0,172, 0x80, 0,18, 0x90,40, 0,6, 0x92,51, 0,12, 0x82, 
0,92, 0x92,51, 0,13, 0x82, 0,51, 0x80, 0,33, 0x81, 0x90,40, 0,1, 0x91,61, 0,5, 
0x92,51, 0,13, 0x82, 0,161, 0x80, 0,27, 0x90,43, 0,6, 0x92,51, 0,13, 0x82, 0,54, 
0x80, 0,25, 0x90,43, 0,11, 0x92,51, 0,13, 0x82, 0,53, 0x80, 0,34, 0x90,43, 0,3, 
0x92,51, 0,13, 0x82, 0,151, 0x80, 0,38, 0x90,43, 0,6, 0x92,51, 0,13, 0x82, 0,92, 
0x92,51, 0,12, 0x82, 0,92, 0x81, 0x91,51, 0,1, 0x92,60, 0,11, 0x81, 0,161, 0x80, 0,29, 
0x90,42, 0,5, 0x91,51, 0,13, 0x81, 0,91, 0x91,51, 0,13, 0x81, 0,54, 0x80, 0,29, 
0x90,41, 0,6, 0x91,51, 0,13, 0x81, 0,118, 0x80, 0,68, 0x90,40, 0,8, 0x91,51, 0,13, 
0x81, 0,91, 0x91,51, 0,13, 0x81, 0,46, 0x80, 0,41, 0x82, 0x90,51, 0,0, 0x91,59, 0,2, 
0x92,51, 0,13, 0x82, 0,139, 0x80, 0,50, 0x90,50, 0,6, 0x92,51, 0,13, 0x82, 0,91, 
0x92,51, 0,13, 0x82, 0,91, 0x92,51, 0,13, 0x82, 0,195, 0x92,51, 0,13, 0x82, 0,91, 
0x92,51, 0,13, 0x82, 0,90, 0x81, 0,1, 0x91,51, 0x92,60, 0,13, 0x81, 0,197, 0x91,51, 
0,12, 0x81, 0,92, 0x91,51, 0,12, 0x81, 0,28, 0x80, 0,56, 0x90,47, 0,6, 0x91,51, 
0,13, 0x81, 0,166, 0x80, 0,30, 0x90,51, 0x91,45, 0,13, 0x80, 0,90, 0x90,51, 0,13, 
0x80, 0,55, 0x81, 0,27, 0x82, 0,0, 0x90,61, 0,6, 0x91,51, 0,6, 0x92,47, 0,6, 
0x81, 0,196, 0x91,51, 0,13, 0x81, 0,91, 0x91,51, 0,13, 0x81, 0,92, 0x91,51, 0,12, 
0x81, 0,197, 0x91,51, 0,13, 0x81, 0,91, 0x91,51, 0,13, 0x81, 0,89, 0x80, 0,1, 0x90,51, 
0x91,60, 0,13, 0x80, 0,195, 0x90,51, 0,13, 0x80, 0,91, 0x90,51, 0,13, 0x80, 0,91, 
0x90,51, 0,13, 0x80, 0,197, 0x90,51, 0,12, 0x80, 0,76, 0x82, 0,16, 0x90,51, 0,13, 
0x80, 0,88, 0x81, 0xf0};
// This score contains 1133 bytes, and 3 tone generators are used.

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

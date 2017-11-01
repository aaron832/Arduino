#ifndef MyMatrix_h
#define MyMatrix_h

#include <LedControl.h>

LedControl lc=LedControl(A0,A2,A1,1);  // Pins: DIN,CLK,CS, # of Display connected
// Put values in arrays
byte invader1a[] =
{
   B00011000,  // First frame of invader #1
   B00111100,
   B01111110,
   B11011011,
   B11111111,
   B00100100,
   B01011010,
   B10100101
};

byte heart_empty_top[] =
{
   B01100110,  // First frame of invader #1
   B10011001,
   B10000001,
   B01000010,
   B00100100,
   B00011000,
   B00000000,
   B00000000
};

void loadMatrixData(byte* passedData)
{
  for (int i = 0; i < 8; i++)  
  {
    lc.setColumn(0,i,passedData[i]);
  }
}

void PlayMatrixInvaderA()
{
	loadMatrixData(heart_empty_top);
}

void MatrixStartLogic()
{
	lc.shutdown(0,false);  // Wake up displays
	lc.setIntensity(0,5);  // Set intensity levels
	lc.clearDisplay(0);  // Clear Displays
	PlayMatrixInvaderA();
}

void MatrixStopLogic()
{
  lc.clearDisplay(0);
  lc.shutdown(0,true);
}

#endif //MyMatrix_h

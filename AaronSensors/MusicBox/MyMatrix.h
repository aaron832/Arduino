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

void loadMatrixData(byte* passedData)
{
  for (int i = 0; i < 8; i++)  
  {
    lc.setRow(0,i,passedData[i]);
  }
}

void PlayMatrixInvaderA()
{
	loadMatrixData(invader1a);
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
#ifndef MyMatrix_h
#define MyMatrix_h

#include <LedControl.h>
#include <avr/pgmspace.h>

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

const uint64_t PROGMEM beatingHeart1[] =
{
  0x0000182442819966,
  0x0000183c66e7ff66,
  0x0000183c7effff66,
  0x9142183c7effff66,
  0x0018244281996600,
  0x00183c66dbff6600,
  0x00183c7effff6600,
  0x81183c7effff6681,
  0x1824428199660000,
  0x18245abd99660000,
  0x183c7effff660000,
  0x183c7effff6618a5
};
const int beatingHeart1_len = sizeof(beatingHeart1)/8;

void loadMatrixDataHexFrame(const uint64_t *_framearray, int _frame)
{
  uint64_t image;
  memcpy_P(&image, &_framearray[_frame], 8);
  for(int i=0; i<8; i++) {
    byte row = (image >> i * 8) & 0xFF;
    lc.setColumn(0,i,row);
  }
}

void loadMatrixDataBinary(byte* passedData)
{
  for (int i = 0; i < 8; i++)  
  {
    lc.setColumn(0,i,passedData[i]);
  }
}

void PlayMatrixInvaderA()
{
	loadMatrixDataBinary(invader1a);
}

void PlayBeatingHeart1(int _frame) 
{
    loadMatrixDataHexFrame(beatingHeart1, _frame);
}

void MatrixStartLogic()
{
	lc.shutdown(0,false);  // Wake up displays
	lc.setIntensity(0,5);  // Set intensity levels
	lc.clearDisplay(0);  // Clear Displays
}

void MatrixStopLogic()
{
  lc.clearDisplay(0);
  lc.shutdown(0,true);
}

#endif //MyMatrix_h

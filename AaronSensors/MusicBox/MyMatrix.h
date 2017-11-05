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

const uint64_t PROGMEM sunkissheart[] = {
  0x8142183c3c184281,
  0x40211a3c3c588402,
  0x20201b3c3cd80404,
  0x1010183ffc180808,
  0x080818fc3f181010,
  0x0404d83c3c1b2020,
  0x0284583c3c1a2140,
  0x0042183c3c184200,
  0x0000183c3c180000,
  0x000000183c3c1800,
  0x00000000183c3c18,
  0xc300000000183c3c,
  0xe7c300000000183c,
  0xe7e7c30000000018,
  0xc3e7e7c300000000,
  0x81c3e7e7c3000000,
  0xdf81c3e7e7c30000,
  0xfbdf81c3e7e7c300,
  0xc3fbdf81c3e7e7c3,
  0x637f7f6163f7f763,
  0x667e7e6666ffff66,
  0x7effff7e66ffff66,
  0xffffffffffffff66,
  0xffffffffffffffff,
  0xffffffe7e7ffffff,
  0xffffe7c3c3e7ffff,
  0xffe7c3818181dbff,
  0xffe7c381000099ff,
  0xc399244281996699,
  0x8118244281996681,
  0x0018244281996600,
  0x00183c66c3ff6600,
  0x00183c7effff6600
};
const int sunkissheart_len = sizeof(sunkissheart)/8;

const uint64_t PROGMEM fishswim[] = {
  0x0000397f7f394000,
  0x00397f7f39200040,
  0x397f7f3940002000,
  0x00397f7f39200040,
  0x0000397f7f392000,
  0x000000397f7f3900
};
const int fishswim_len = sizeof(fishswim)/8;

const uint64_t PROGMEM fishtojelly[] = {
  0x0c00000072fefe72,
  0x3c18000000e4fcfc,
  0xfc7830000000c8f8,
  0xf8f8f06000000090,
  0xa3f1f0e0c0000000,
  0x4747e3e1c0800000,
  0x058f8fc7c3800000,
  0x0a0a1f1f8f870000,
  0x0a14153f3f1f0e00,
  0x15152a2a7f7f3e1c
};
const int fishtojelly_len = sizeof(fishtojelly)/8;

const uint64_t PROGMEM jellydance[] = {
  0x2a2a5454fefe7c38, //right
  0x54545454fefe7c38, //stand right
  0x54542a2a7f7f3e1c, //left
  0x2a2a2a2a7f7f3e1c //stand left
};
const int jellydance_len = sizeof(jellydance)/8;


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

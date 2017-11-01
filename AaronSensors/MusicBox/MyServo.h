#ifndef MyServo_h
#define MyServo_h

#include <Servo.h>

Servo servo1;

void ServoSetupLogic()
{
	servo1.attach(14+3);
  servo1.write(7);
  delay(1000);
  servo1.detach();
}

void ServoStartLogic()
{
  servo1.attach(14+3);
	//servo1.write(180);
}

void ServoStopLogic()
{
	servo1.write(7);
  delay(1000);
  servo1.detach();
}

void SetServo(int x)
{
  servo1.write(x);
}

#endif

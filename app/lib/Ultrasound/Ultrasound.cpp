#include <Arduino.h>
#include "Ultrasound.h"

//HC-SR04
#define pin_trig 13
#define pin_echo 15

Ultrasound::Ultrasound()
{ //inicializamos los motores
  pinMode(pin_trig, OUTPUT);
  pinMode(pin_echo, INPUT);
}

int Ultrasound::distance()
{
  int distance;
  digitalWrite(pin_trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pin_trig, LOW);

  long time = pulseIn(pin_echo, HIGH);

  if (time == 0)
  {
    distance = 0;
  }
  else
  {
    distance = time / 59;
    // limitamos la distancia medida a 5 metros
    if (distance > 500)
    {
      distance = 500;
    }
  }

  return distance;
}

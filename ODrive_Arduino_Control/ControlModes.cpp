#include "Arduino.h"
#include "ControlModes.h"

#define linearHist 2.0            //Histéresis para evitar rebotes (cm)
#define rampControlThreshold 5.0  //Umbral inicial para aplicar un control de rampa (inicio suave)

bool boxFlag[2] = {false, false};

//Función control de corriente (control de torque): Regula la corriente para aplicar el torque suavemente en el inicio.
//Inputs:
//  float linearPosition: Posición lineal (en cm)
//  float current: Corriente configurada (en A)
//Output:
//  float currentControlValue: Valor de corriente resultante (en A)
float currentControlValue(float linearPosition, float current)
{
  if(linearPosition > (0.0 + linearHist)) return 0.0;
  else if(linearPosition < (0.0 - linearHist))
  {
    float calCurrent;
    if (linearPosition > (0.0 - linearHist - rampControlThreshold))
    {
      calCurrent = current*(-(linearPosition + linearHist)/rampControlThreshold); 
      //Serial.println(calCurrent);
    } 
    else calCurrent = current;
    //Serial << calCurrent << '\n';
    return calCurrent;
  }
}

//Función demo boxeo: Aplica un "golpe digital" a una distancia determinada.
//Inputs:
//  float linearPosition: Posición lineal (en cm)
//  float motorNum: Número de motor
//Output:
//  float currentHapticsBox: Valor de corriente resultante (en A)
float currentHapticsBox(float linearPosition, int motorNum)
{
  float current = 2.0;
  float threshold = -110.0;
  if(linearPosition > (0.0 + linearHist)) return 0.0;
  else if(linearPosition < (0.0 - linearHist))
  {
    float calCurrent;
    if (linearPosition > (0.0 - linearHist - rampControlThreshold))
    {
      calCurrent = current*(-(linearPosition + linearHist)/rampControlThreshold); 
      //Serial.println(calCurrent);
    } 
    else 
    {
      //Serial << linearPosition << ' ' << threshold << '\n';
      if (linearPosition > threshold)
      {
        calCurrent = 2.0;
        boxFlag[motorNum] = false;
      }
      else
      {
        if(boxFlag[motorNum] == false)
        {
          calCurrent = 12.0;
          boxFlag[motorNum] = true;
        }
        else
        {
          calCurrent = 2.0;
          delay(50);
        }
      }
    }
    return calCurrent;
  }
}

//Función para provocar vibración
//Inputs:
//  float linearPosition: Posición lineal (en cm)
//  float lastCurrent: Corriente configurada en el ciclo anterior (en A)
//  int mode: Modo actual de control
//Output:
//  float currentHapticsVibration: Valor de corriente resultante (en A)
float currentHapticsVibration(float linearPosition, float lastCurrent, int mode)
{
  float current = 2.0;
  if(linearPosition > (0.0 + linearHist)) return 0.0;
  else if(linearPosition < (0.0 - linearHist))
  {
    float calCurrent;
    if (linearPosition > (0.0 - linearHist - rampControlThreshold))
    {
      calCurrent = current*(-(linearPosition + linearHist)/rampControlThreshold); 
      //Serial.println(calCurrent);
    } 
    else 
    {
      int dly = (int)((-(linearPosition + linearHist + rampControlThreshold) /50.0)*100.0);
      //Serial << dly << '\n';
      //Serial << linearPosition << ' ' << threshold << '\n';
      if (lastCurrent == 2.0) 
      {
        if(mode == 1) {
          delay(dly);
          calCurrent = 10.0;
        }
        else calCurrent = 8.0;
      }
      else
      {
        if(mode == 1) {
          delay(dly);
          calCurrent = 2.0;
        }
        else calCurrent = 2.0;
      }
    }
    return calCurrent;
  }
}

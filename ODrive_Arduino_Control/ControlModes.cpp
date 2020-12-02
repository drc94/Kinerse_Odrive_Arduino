#include "Arduino.h"
#include "ControlModes.h"

#define linearHist 2.0            //Histéresis para evitar rebotes (cm)
#define rampControlThreshold 5.0  //Umbral inicial para aplicar un control de rampa (inicio suave)

bool boxFlag[2] = {false, false};

//Función control de corriente (control de torque): Regula la corriente para aplicar el torque suavemente en el inicio.
//Inputs:
//  float linearPosition: Posición lineal (en cm)
//  float torque: Corriente configurada (en A)
//Output:
//  float torqueControlValue: Valor de corriente resultante (en A)
float torqueControlValue(float linearPosition, float torque)
{
  if(linearPosition > (0.0 + linearHist)) return 0.0;
  else if(linearPosition < (0.0 - linearHist))
  {
    float calTorque;
    if (linearPosition > (0.0 - linearHist - rampControlThreshold))
    {
      calTorque = torque*(-(linearPosition + linearHist)/rampControlThreshold); 
      //Serial.println(calTorque);
    } 
    else calTorque = torque;
    //Serial << calTorque << '\n';
    return calTorque;
  }
}

//Función demo boxeo: Aplica un "golpe digital" a una distancia determinada.
//Inputs:
//  float linearPosition: Posición lineal (en cm)
//  float motorNum: Número de motor
//Output:
//  flott corqueHapticsBox: Valor de corriente resultante (en A)
float torqueHapticsBox(float linearPosition, int motorNum)
{
  float torque = 0.167;
  float threshold = -110.0;
  if(linearPosition > (0.0 + linearHist)) return 0.0;
  else if(linearPosition < (0.0 - linearHist))
  {
    float calTorque;
    if (linearPosition > (0.0 - linearHist - rampControlThreshold))
    {
      calTorque = torque*(-(linearPosition + linearHist)/rampControlThreshold); 
      //Serial.println(calTorque);
    } 
    else 
    {
      //Serial << linearPosition << ' ' << threshold << '\n';
      if (linearPosition > threshold)
      {
        calTorque = 0.167;
        boxFlag[motorNum] = false;
      }
      else
      {
        if(boxFlag[motorNum] == false)
        {
          calTorque = 1.6;
          boxFlag[motorNum] = true;
        }
        else
        {
          calTorque = 0.167;
          delay(50);
        }
      }
    }
    return calTorque;
  }
}

//Función para provocar vibración
//Inputs:
//  float linearPosition: Posición lineal (en cm)
//  float lastTorque: Corriente configurada en el ciclo anterior (en A)
//  int mode: Modo actual de control
//Output:
//  float torqueHapticsVibration: Valor de corriente resultante (en A)
float torqueHapticsVibration(float linearPosition, float lastTorque, int mode)
{
  float torque = 0.167;
  if(linearPosition > (0.0 + linearHist)) return 0.0;
  else if(linearPosition < (0.0 - linearHist))
  {
    float calTorque;
    if (linearPosition > (0.0 - linearHist - rampControlThreshold))
    {
      calTorque = torque*(-(linearPosition + linearHist)/rampControlThreshold); 
      //Serial.println(calTorque);
    } 
    else 
    {
      int dly = (int)((-(linearPosition + linearHist + rampControlThreshold) /50.0)*100.0);
      //Serial << dly << '\n';
      //Serial << linearPosition << ' ' << threshold << '\n';
      if (lastTorque == 0.167) 
      {
        if(mode == 1) {
          delay(dly);
          calTorque = 0.83;
        }
        else calTorque = 0.67;
      }
      else
      {
        if(mode == 1) {
          delay(dly);
          calTorque = 0.167;
        }
        else calTorque = 0.167;
      }
    }
    return calTorque;
  }
}


bool downDir(float linearPosition, float initPos, float finalPos, float velocity, int motorNum)
{
  static bool dir[2];
  if(((-linearPosition) <= initPos)&&(((-velocity) >= 0.2)))
  {
    dir[motorNum] = false;
  }
  else 
  {
    if((-linearPosition) >= finalPos) dir[motorNum] = true;
    //else{}
  }
  /*Serial.print(" dir ");
  Serial.print(dir);*/
  return dir[motorNum];
}

bool flagFriend(bool downDir, float linearPosition, float initPos, float finalPos, float vel, int motorNum)
{
  static bool flag[2];
  if(downDir == false)
  {
    if(((-vel) <= 0.1) && ((-linearPosition) >= initPos*1.2)) flag[motorNum] = true;
    //else{}
  }
  else flag[motorNum] = false;
  
  return flag[motorNum];
}

//Función para ayudar al entrenamiento al fallo
//Inputs:
//  float linearPosition: Posición lineal (en cm)
//  float initPos: Posición inicial (en cm)
//  float finalPos: Posición final del ejercicio (en cm)
//  float velocity: Velocidad actual (cm/s)
//  float torqueSetpoint: Corriente objetivo (A)
//Output:
//  float torqueFriend: Valor de corriente resultante (en A)
float torqueFriend(float linearPosition, float initPos, float finalPos, float velocity, float torqueSetpoint, int motorNum)
{ 
  float outputTorque;
  bool downDirBool = downDir(linearPosition, initPos, finalPos, velocity, motorNum);
  bool flagFriendBool = flagFriend(downDirBool, linearPosition, initPos, finalPos, velocity, motorNum);
  if((-linearPosition) >= (initPos*0.8))
  {
    if(flagFriendBool == true)
    {
      outputTorque = torqueSetpoint*0.75;
    }
    else
    {
      outputTorque = torqueSetpoint;
    }
  }
  else
  {
    outputTorque = 0.8; //(Nm)
  }
  //Serial.print(" torque ");
  //Serial.println(output);
  return outputTorque;
}

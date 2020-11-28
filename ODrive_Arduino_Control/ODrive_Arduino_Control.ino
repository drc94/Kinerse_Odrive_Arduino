#include "ODriveArduino.h"
#include "ControlModes.h"
#include "Communications.h"

#define ESP32_UART2_PIN_TX 17
#define ESP32_UART2_PIN_RX 16
// ODrive uses 115200 baud
#define BAUDRATE 115200

// Printing with stream operator
template<class T> inline Print& operator <<(Print &obj,     T arg) { obj.print(arg);    return obj; }
template<>        inline Print& operator <<(Print &obj, float arg) { obj.print(arg, 4); return obj; }

const float currentLimit = 20.0f;             //Límite de corriente
const float velLimit = 20000.0f;              //Límite de velocidad

float posOffset[2] = {0.0, 0.0};        //Offset para corregir la posicion inicial
float linearPosition[2] = {0.0, 0.0};   //Posición lineal (cm)
float current[2] = {0.0, 0.0};          //Corriente (A)
float lastCurrentValue[2] = {0.0, 0.0}; //Último valor de corriente enviado al controlador (A)
int motorMode = 0;                      //Modo del motor

// ODrive object
ODriveArduino odrive(Serial1);

void setup() {
  delay(2000);  //Espera para empezar a calibrar el motor automáticamente
  // Serial to the ODrive
  // ODrive uses 115200 baud
  Serial1.begin(BAUDRATE, SERIAL_8N1, ESP32_UART2_PIN_TX, ESP32_UART2_PIN_RX);

  initBT(); // Inicializa bluetooth del esp32
  // Serial to PC
  Serial.begin(BAUDRATE);
  
  while (!Serial) ;  // wait for Arduino Serial 0 Monitor to open
  Serial.println("Serial 0 Ready...");
  while (!Serial1) ; // wait for Arduino Serial 1 
  Serial.println("Serial 1 Ready...");

  // See the documentation or play around in odrivetool to see the available parameters
  for (int axis = 0; axis < 2; ++axis) {
    Serial1 << "w axis" << axis << ".controller.config.vel_limit " << velLimit << '\n';
    Serial1 << "w axis" << axis << ".motor.config.current_lim " << currentLimit << '\n';
  }

  delay(2000);  //Espera para empezar a calibrar el motor automáticamente
  initCalibration(&odrive); //Secuencia de calibración de motores
  posOffset[0] = initPosition(&odrive, 0); //Inicializa posición motor 0
  //initPosition(odrive, 1); //Inicializa posición motor 1

  Serial.println("Ready!");
  Serial.println("Send the character '0' or '1' to calibrate respective motor (you must do this before you can command movement)");
  Serial.println("Send the character 'l' to apply the loop control");
  Serial.println("Send the character 'c' to enter in current control mode");
  Serial.println("Send the character 's' to stop motor");
  Serial.println("Send the character 'b' to read bus voltage");
  Serial.println("Send the character 'p' to read linear position");
  Serial.println("Send the character 'e' to set current linear position to 0");
  Serial.println("Send the character 'h' followed by the mode to set the haptics test");
}

void loop() {
  serialCOM(&odrive, &motorMode, &current[0], &posOffset[0], &linearPosition[0]);   //Gestiona las comunicaciones entre el PC y el arduino a través del puerto serie 1
  serialBT(&odrive, &motorMode, &current[0], &posOffset[0], &linearPosition[0]);    //Gestiona las comunicaciones entre el módulo BT y el arduino a través del puerto serie 3

  linearPosition[0] = 2*PI*2*(odrive.GetPosition(0)/4000) - posOffset[0]; //2cm radio, 2pi = 4000 counts
  linearPosition[1] = 2*PI*2*(odrive.GetPosition(1)/4000) - posOffset[1]; //2cm radio, 2pi = 4000 counts
  if(motorMode == 1) { //Boxing
    currentControl(currentHapticsBox(linearPosition[0],0), lastCurrentValue[0],0);
    currentControl(currentHapticsBox(linearPosition[1],1), lastCurrentValue[1],1);
  }
  else if(motorMode == 2){ //Vibration
    currentControl(currentHapticsVibration(linearPosition[0], lastCurrentValue[0], 0), lastCurrentValue[0],0);
    currentControl(currentHapticsVibration(linearPosition[1], lastCurrentValue[1], 0), lastCurrentValue[1],1);
  }  
  else if(motorMode == 3){ //Vibration ramp
    currentControl(currentHapticsVibration(linearPosition[0], lastCurrentValue[0], 1), lastCurrentValue[0],0);
    currentControl(currentHapticsVibration(linearPosition[1], lastCurrentValue[1], 1), lastCurrentValue[1],1);
  }
  else if(motorMode == 4){ //Friend mode
    currentControl(currentFriend(linearPosition[0], 50.0, 100.0, odrive.GetVelocity(0), currentControlValue(linearPosition[0], current[0]), 0), lastCurrentValue[0], 0);
    currentControl(currentFriend(linearPosition[1], 50.0, 100.0, odrive.GetVelocity(1), currentControlValue(linearPosition[1], current[1]), 1), lastCurrentValue[1], 1);
  }
  else {
    currentControl(currentControlValue(linearPosition[0], current[0]), lastCurrentValue[0],0);
    currentControl(currentControlValue(linearPosition[1], current[1]), lastCurrentValue[1],1);
  }

  delay(50);
}

void currentControl(float current, float lastCurrent, int motorNum)
{
  if(current != lastCurrent) 
  {
    odrive.SetCurrent(motorNum, current);
    //Serial.println(current);
    lastCurrentValue[motorNum] = current;
  }
}

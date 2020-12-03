#include "ODriveArduino.h"
#include "ControlModes.h"
#include "Communications.h"

// Printing with stream operator
template<class T> inline Print& operator <<(Print &obj,     T arg) { obj.print(arg);    return obj; }
template<>        inline Print& operator <<(Print &obj, float arg) { obj.print(arg, 4); return obj; }

const float currentLimit = 24.0;              //Límite de corriente
const float calCurrent = 10.0;              //Corriente de calibración
const float velLimit = 50.0;               //turns/s
const float brakeRes = 1.7;                   //Resistencia de disipación
const float polePairs = 20.0;                         //Polos del motor
const float torqueConstant = 0.0827;                   //Resistencia de disipación
const float cpr = 4000.0;                         // CPR del encoder

float posOffset[2] = {0.0, 0.0};        //Offset para corregir la posicion inicial
float linearPosition[2] = {0.0, 0.0};   //Posición lineal (cm)
float torque[2] = {0.0, 0.0};          //Torque (Nm)
float lastTorqueValue[2] = {0.0, 0.0}; //Último valor de corriente enviado al controlador (A)
int motorMode = 0;                      //Modo del motor

// ODrive object
ODriveArduino odrive(Serial2);

void setup() {
  delay(4000);  //Espera para empezar a calibrar el motor automáticamente
  // Serial to the ODrive
  // ODrive uses 115200 baud
  Serial2.begin(115200);

  //HC05 Bluetooth module uses 9600 baud
  Serial3.begin(9600);

  // Serial to PC
  Serial.begin(115200);
  while (!Serial) ; // wait for Arduino Serial Monitor to open

  Serial.println("ODriveArduino");
  Serial.println("Setting parameters...");

  // In this example we set the same parameters to both motors.
  // You can of course set them different if you want.
  // See the documentation or play around in odrivetool to see the available parameters
  for (int axis = 0; axis < 2; ++axis) {
    Serial2 << "w axis" << axis << ".controller.config.vel_limit " << 10.0 << '\n';
    Serial2 << "w axis" << axis << ".motor.config.current_lim " << currentLimit << '\n';
  /*  Serial2 << "w axis" << axis << ".config.brake_resistance " << brakeRes << '\n';
    Serial2 << "w axis" << axis << ".config.pole_pairs " << polePairs << '\n';
    Serial2 << "w axis" << axis << ".motor.config.torque_constant " << torqueConstant << '\n';
    Serial2 << "w axis" << axis << ".encoder.config.cpr " << cpr << '\n';
    Serial2 << "w axis" << axis << ".motor.config.calibration_current " << calCurrent << '\n';
    // This ends up writing something like "w axis0.motor.config.current_lim 10.0\n"*/
  }

  delay(1000);  //Espera para empezar a calibrar el motor automáticamente
  initCalibration(&odrive); //Secuencia de calibración de motores NO NECESARIO
  posOffset[0] = initPosition(&odrive, 0); //Inicializa posición motor 0  (invertido)
  //posOffset[1] = initPosition(odrive, 1); //Inicializa posición motor 1
  delay(1000);
  for (int axis = 0; axis < 2; ++axis) {
    Serial2 << "w axis" << axis << ".controller.config.vel_limit " << velLimit << '\n';
  }

  Serial.println("Ready!");
  Serial.println("Send the character '0' or '1' to calibrate respective motor (you must do this before you can command movement)");
  Serial.println("Send the character 'l' to apply the loop control");
  Serial.println("Send the character 'c' to enter in torque control mode");
  Serial.println("Send the character 's' to stop motor");
  Serial.println("Send the character 'b' to read bus voltage");
  Serial.println("Send the character 'p' to read linear position");
  Serial.println("Send the character 'e' to set current linear position to 0");
  Serial.println("Send the character 'h' followed by the mode to set the haptics test");
}

void loop() {
  serialCOM(&odrive, &motorMode, &torque[0], &posOffset[0], &linearPosition[0]);   //Gestiona las comunicaciones entre el PC y el arduino a través del puerto serie 1
  serialBT(&odrive, &motorMode, &torque[0], &posOffset[0], &linearPosition[0]);    //Gestiona las comunicaciones entre el módulo BT y el arduino a través del puerto serie 3

  linearPosition[0] = -(2*PI*2*(odrive.GetPosition(0)) - posOffset[0]); //2cm radio
  linearPosition[1] = 2*PI*2*(odrive.GetPosition(1)) - posOffset[1]; //2cm radio
  if(motorMode == 1) { //Boxing
    torqueControl(torqueHapticsBox(linearPosition[0],0), lastTorqueValue[0],0);
    torqueControl(torqueHapticsBox(linearPosition[1],1), lastTorqueValue[1],1);
  }
  else if(motorMode == 2){ //Vibration
    torqueControl(torqueHapticsVibration(linearPosition[0], lastTorqueValue[0], 0), lastTorqueValue[0],0);
    torqueControl(torqueHapticsVibration(linearPosition[1], lastTorqueValue[1], 0), lastTorqueValue[1],1);
  }  
  else if(motorMode == 3){ //Vibration ramp
    torqueControl(torqueHapticsVibration(linearPosition[0], lastTorqueValue[0], 1), lastTorqueValue[0],0);
    torqueControl(torqueHapticsVibration(linearPosition[1], lastTorqueValue[1], 1), lastTorqueValue[1],1);
  }
  else if(motorMode == 4){ //Friend mode
    torqueControl(torqueFriend(linearPosition[0], 50.0, 100.0, odrive.GetVelocity(0), torqueControlValue(linearPosition[0], torque[0]), 0), lastTorqueValue[0], 0);
    torqueControl(torqueFriend(linearPosition[1], 50.0, 100.0, odrive.GetVelocity(1), torqueControlValue(linearPosition[1], torque[1]), 1), lastTorqueValue[1], 1);
    /*Serial.print(value2);
    Serial.print(' ');
    Serial.print(value);
    Serial.print(' ');
    Serial.print(lastTorqueValue[0]);
    Serial.print(' ');
    Serial.print(-odrive.GetVelocity(0));
    Serial.print(' ');
    Serial.println(-linearPosition[0]);*/
  }
  else {
    torqueControl(torqueControlValue(linearPosition[0], torque[0]), lastTorqueValue[0],0);
    torqueControl(torqueControlValue(linearPosition[1], torque[1]), lastTorqueValue[1],1);
  }

  delay(5);
}

void torqueControl(float torque, float lastTorque, int motorNum)
{
  if(torque != lastTorque) 
  {
    if(motorNum == 0) odrive.SetTorque(motorNum, -torque);
    else odrive.SetTorque(motorNum, torque);
    //Serial.println(torque);
    lastTorqueValue[motorNum] = torque;
  }
}

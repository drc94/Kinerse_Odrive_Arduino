
#include "ODriveArduino.h"

// Printing with stream operator
template<class T> inline Print& operator <<(Print &obj,     T arg) { obj.print(arg);    return obj; }
template<>        inline Print& operator <<(Print &obj, float arg) { obj.print(arg, 4); return obj; }

float pos_offset = 0.0;

float linearPosition = 0.0;
float linearHist = 2.0;
float rampControlThreshold = 5.0;
float current = 0.0;
float lastCurrentValue = 0.0;
#define current_limit 15.0

int motorMode = 0;

// ODrive object
ODriveArduino odrive(Serial2);

void setup() {
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
    Serial2 << "w axis" << axis << ".controller.config.vel_limit " << 20000.0f << '\n';
    Serial2 << "w axis" << axis << ".motor.config.current_lim " << 20.0f << '\n';
    // This ends up writing something like "w axis0.motor.config.current_lim 10.0\n"
  }

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

  if (Serial.available()) {
    delay(10);
    char c = Serial.read();

    // Run calibration sequence
    if (c == '0' || c == '1') {
      int motornum = c-'0';
      int requested_state;

      requested_state = ODriveArduino::AXIS_STATE_MOTOR_CALIBRATION;
      Serial << "Axis" << c << ": Requesting state " << requested_state << '\n';
      odrive.run_state(motornum, requested_state, true);

      requested_state = ODriveArduino::AXIS_STATE_ENCODER_OFFSET_CALIBRATION;
      Serial << "Axis" << c << ": Requesting state " << requested_state << '\n';
      odrive.run_state(motornum, requested_state, true);

      requested_state = ODriveArduino::AXIS_STATE_CLOSED_LOOP_CONTROL;
      Serial << "Axis" << c << ": Requesting state " << requested_state << '\n';
      odrive.run_state(motornum, requested_state, false); // don't wait
    }

    // Current mode
    if (c == 'c') {
      Serial << "Current control mode: ";
      char d = Serial.read();
      String command = "";
      motorMode = 0;
      if(isWhitespace(d))
      {
        if(Serial.available()) d = Serial.read();
        command = command + char(d);
        while(!isWhitespace(d) && Serial.available())
        {
          d = Serial.read();
          command = command + char(d);
        }
        current = command.toFloat();
        if((current > current_limit) || (current < 0.0)) Serial << "Overcurrent error" << '\n';
        else
        {
          Serial << "Current = " << current << "A" << '\n';
          //odrive.SetCurrent(0, current);
        }
      }
      else
      {
        Serial << "Command error" << '\n';
      }
    }

    // Stop motor
    if (c == 's') {
      int requested_state;
      requested_state = ODriveArduino::AXIS_STATE_IDLE;
      odrive.run_state(0, requested_state, false);
      Serial.println("Control stopped");
    }

    // Loop control
    if (c == 'l') {
      int requested_state;
      requested_state = ODriveArduino::AXIS_STATE_CLOSED_LOOP_CONTROL;
      odrive.run_state(0, requested_state, false); // don't wait
      Serial.println("Running loop control");
    }

    // Encoder offset calibration
    if (c == 'e') {
      int requested_state;
      pos_offset = 2*PI*2*(odrive.GetPosition(0)/4000);
      Serial.println("Set home position");
    }

    // Read bus voltage
    if (c == 'b') {
      Serial2 << "r vbus_voltage\n";
      delay(500);
      Serial << "Vbus voltage: " << odrive.readFloat() << "V" << '\n';
    }

    // print motor position
    if (c == 'p') {
      Serial << "Linear position: " << linearPosition << "cm" << '\n';
    }

    // Loop control
    if (c == 'h') {
      Serial << "Haptic test: " << '\n';
      char d = Serial.read();
      String command = "";
      if(isWhitespace(d))
      {
        if(Serial.available()) d = Serial.read();
        command = command + char(d);
        while(!isWhitespace(d) && Serial.available())
        {
          d = Serial.read();
          command = command + char(d);
        }
        
        if(command.toInt() > 3) Serial << "Command error" << '\n';
        else
        {
          motorMode  = command.toInt();
          if(motorMode == 1) Serial << "BOXING MODE" << '\n';
          if(motorMode == 2) Serial << "VIBRATION MODE 1" << '\n';
          if(motorMode == 3) Serial << "VIBRATION MODE 2" << '\n';
        }
      }
      else
      {
        Serial << "Command error" << '\n';
      }
    }
  }

  if(Serial3.available()){
    delay(10);
    char c = Serial3.read();

    // Run calibration sequence
    if (c == '0' || c == '1') {
      int motornum = c-'0';
      int requested_state;

      Serial3 << "CAL MOT ";

      requested_state = ODriveArduino::AXIS_STATE_MOTOR_CALIBRATION;
      //Serial << "Axis" << c << ": Requesting state " << requested_state << '\n';
      odrive.run_state(motornum, requested_state, true);

      requested_state = ODriveArduino::AXIS_STATE_ENCODER_OFFSET_CALIBRATION;
      //Serial << "Axis" << c << ": Requesting state " << requested_state << '\n';
      odrive.run_state(motornum, requested_state, true);

      requested_state = ODriveArduino::AXIS_STATE_CLOSED_LOOP_CONTROL;
      //Serial << "Axis" << c << ": Requesting state " << requested_state << '\n';
      odrive.run_state(motornum, requested_state, false); // don't wait
    }

    // Current mode
    if (c == 'c') {
      //Serial << "Current control mode: ";
      char d = Serial3.read();
      String command = "";
      motorMode = 0;
      if(isWhitespace(d))
      {
        if(Serial3.available()) d = Serial3.read();
        command = command + char(d);
        while(!isWhitespace(d) && Serial3.available())
        {
          d = Serial3.read();
          command = command + char(d);
        }
        current = command.toFloat();
        if((current > current_limit) || (current < 0.0)) Serial3 << "OC ERROR";
        else
        {
          Serial3 << "CURRENT ";
          //odrive.SetCurrent(0, current);
        }
      }
      else
      {
        Serial3 << "CM ERROR";
      }
    }

    // Stop motor
    if (c == 's') {
       Serial3 << "MOT STOP";
      int requested_state;
      requested_state = ODriveArduino::AXIS_STATE_IDLE;
      odrive.run_state(0, requested_state, false);
    }

    // Loop control
    if (c == 'l') {
      Serial3 << "LOOP CON";
      int requested_state;
      requested_state = ODriveArduino::AXIS_STATE_CLOSED_LOOP_CONTROL;
      odrive.run_state(0, requested_state, false); // don't wait
    }
  }

  linearPosition = 2*PI*2*(odrive.GetPosition(0)/4000) - pos_offset; //2cm radio, 2pi = 4000 counts
  if(motorMode == 1) {
    currentControl(currentHapticsBox(linearPosition), lastCurrentValue);
  }
  else if(motorMode == 2){
    currentControl(currentHapticsVibration(linearPosition, lastCurrentValue, 0), lastCurrentValue);
  }  
  else if(motorMode == 3){
    currentControl(currentHapticsVibration(linearPosition, lastCurrentValue, 1), lastCurrentValue);
  }
  else {
    currentControl(currentControlValue(linearPosition, lastCurrentValue), lastCurrentValue);
  }

  delay(10);
}

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
    return calCurrent;
  }
}

void currentControl(float current, float lastCurrent)
{
  if(current != lastCurrent) 
  {
    odrive.SetCurrent(0, current);
    //Serial.println(current);
    lastCurrentValue = current;
  }
}

float currentHapticsBox(float linearPosition)
{
  float current = 2.0;
  float threshold = -25.0;
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
      }
      else
      {
        calCurrent = 12.0;
      }
    }
    return calCurrent;
  }
}

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
      //Serial << linearPosition << ' ' << threshold << '\n';
      if (lastCurrent == 2.0) 
      {
        if(mode == 1) {
          delay(50);
          calCurrent = 6.0;
        }
        else calCurrent = 8.0;
      }
      else
      {
        if(mode == 1) {
          delay(50);
          calCurrent = 2.0;
        }
        else calCurrent = 2.0;
      }
    }
    return calCurrent;
  }
}

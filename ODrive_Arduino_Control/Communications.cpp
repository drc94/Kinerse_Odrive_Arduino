#include "Arduino.h"
#include "Communications.h"

// Printing with stream operator
template<class T> inline Print& operator <<(Print &obj,     T arg) { obj.print(arg);    return obj; }
template<>        inline Print& operator <<(Print &obj, float arg) { obj.print(arg, 4); return obj; }

#define currentLimit 20.0               //Límite de corriente

int serialCOM(ODriveArduino* odrive, int* motorMode, float* current, float* posOffset, float* linearPosition)
{
  if (Serial.available()) {
    delay(10);
    char c = Serial.read();

    // Run calibration sequence
    if (c == '0' || c == '1') {
      int motornum = c-'0';
      int requested_state;

      requested_state = ODriveArduino::AXIS_STATE_MOTOR_CALIBRATION;
      Serial << "Axis" << c << ": Requesting state " << requested_state << '\n';
      odrive->run_state(motornum, requested_state, true);

      requested_state = ODriveArduino::AXIS_STATE_ENCODER_OFFSET_CALIBRATION;
      Serial << "Axis" << c << ": Requesting state " << requested_state << '\n';
      odrive->run_state(motornum, requested_state, true);

      requested_state = ODriveArduino::AXIS_STATE_CLOSED_LOOP_CONTROL;
      Serial << "Axis" << c << ": Requesting state " << requested_state << '\n';
      odrive->run_state(motornum, requested_state, false); // don't wait
    }

    // Current mode
    if (c == 'c') {
      Serial << "Current control mode: ";
      char d = Serial.read();
      String command = "";
      *motorMode = 0;
      if(isWhitespace(d))
      {
        if(Serial.available()) d = Serial.read();
        command = command + char(d);
        while(!isWhitespace(d) && Serial.available())
        {
          d = Serial.read();
          command = command + char(d);
        }
        *current = command.toFloat();
        *(current+1) = *current;
        if((*current > currentLimit) || (*current < 0.0)) Serial << "Overcurrent error" << '\n';
        else
        {
          Serial << "Current = " << *current << "A" << '\n';
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
      odrive->run_state(0, requested_state, false);
      delay(100);
      odrive->run_state(1, requested_state, false);
      Serial.println("Control stopped");
    }

    // Loop control
    if (c == 'l') {
      int requested_state;
      requested_state = ODriveArduino::AXIS_STATE_CLOSED_LOOP_CONTROL;
      odrive->run_state(0, requested_state, false); // don't wait
      delay(100);
      odrive->run_state(1, requested_state, false); // don't wait
      Serial.println("Running loop control");
    }

    // Encoder offset calibration
    if (c == 'e') {
      int requested_state;
      *posOffset = 2*PI*2*(odrive->GetPosition(0)/4000);
      delay(100);
      *(posOffset+1) = 2*PI*2*(odrive->GetPosition(1)/4000);
      Serial.println("Set home position");
    }

    // Read bus voltage
    if (c == 'b') {
      Serial2 << "r vbus_voltage\n";
      delay(500);
      Serial << "Vbus voltage: " << odrive->readFloat() << "V" << '\n';
    }

    // print motor position
    if (c == 'p') {
      Serial << "Linear position M0: " << *linearPosition << "cm" << '\n';
      delay(100);
      Serial << "Linear position M1: " << *(linearPosition+1) << "cm" << '\n';
    }

    // Haptics mode
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
          *motorMode  = command.toInt();
          if(*motorMode == 1) Serial << "BOXING MODE" << '\n';
          if(*motorMode == 2) Serial << "VIBRATION MODE 1" << '\n';
          if(*motorMode == 3) Serial << "VIBRATION MODE 2" << '\n';
        }
      }
      else
      {
        Serial << "Command error" << '\n';
      }
    }
  }
}

int serialBT(ODriveArduino* odrive, int* motorMode, float* current, float* posOffset, float* linearPosition)
{
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
      odrive->run_state(motornum, requested_state, true);

      requested_state = ODriveArduino::AXIS_STATE_ENCODER_OFFSET_CALIBRATION;
      //Serial << "Axis" << c << ": Requesting state " << requested_state << '\n';
      odrive->run_state(motornum, requested_state, true);

      requested_state = ODriveArduino::AXIS_STATE_CLOSED_LOOP_CONTROL;
      //Serial << "Axis" << c << ": Requesting state " << requested_state << '\n';
      odrive->run_state(motornum, requested_state, false); // don't wait
    }

    // Current mode
    if (c == 'c') {
      //Serial << "Current control mode: ";
      char d = Serial3.read();
      String command = "";
      *motorMode = 0;
      if(isWhitespace(d))
      {
        if(Serial3.available()) d = Serial3.read();
        command = command + char(d);
        while(!isWhitespace(d) && Serial3.available())
        {
          d = Serial3.read();
          command = command + char(d);
        }
        *current = command.toFloat();
        *(current+1) = *current;
        if((*current > currentLimit) || (*current < 0.0)) Serial3 << "OC ERROR";
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
      odrive->run_state(0, requested_state, false);
      delay(100);
      odrive->run_state(1, requested_state, false);      
    }

    // Loop control
    if (c == 'l') {
      Serial3 << "LOOP CON";
      int requested_state;
      requested_state = ODriveArduino::AXIS_STATE_CLOSED_LOOP_CONTROL;
      odrive->run_state(0, requested_state, false); // don't wait
      delay(100);
      odrive->run_state(1, requested_state, false); // don't wait
    }

     // Haptics mode
    if (c == 'h') {
      char d = Serial3.read();
      String command = "";
      if(isWhitespace(d))
      {
        if(Serial3.available()) d = Serial3.read();
        command = command + char(d);
        while(!isWhitespace(d) && Serial3.available())
        {
          d = Serial3.read();
          command = command + char(d);
        }
        
        if(command.toInt() > 3) Serial3 << "CM ERROR" << '\n';
        else
        {
          *motorMode  = command.toInt();
          if(*motorMode == 1) Serial3 << "BOXING  ";
          if(*motorMode == 2) Serial3 << "VIBRAT 1";
          if(*motorMode == 3) Serial3 << "VIBRAT 2";
        }
      }
      else
      {
        Serial3 << "CM ERROR";
      }
    }
  }
}

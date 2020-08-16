
#include "ODriveArduino.h"

// Printing with stream operator
template<class T> inline Print& operator <<(Print &obj,     T arg) { obj.print(arg);    return obj; }
template<>        inline Print& operator <<(Print &obj, float arg) { obj.print(arg, 4); return obj; }

float pos_offset = 0.0;

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
    Serial2 << "w axis" << axis << ".controller.config.vel_limit " << 10000.0f << '\n';
    Serial2 << "w axis" << axis << ".motor.config.current_lim " << 10.0f << '\n';
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
      if(isWhitespace(d))
      {
        if(Serial.available()) d = Serial.read();
        command = command + char(d);
        while(!isWhitespace(d) && Serial.available())
        {
          d = Serial.read();
          command = command + char(d);
        }
        float current = command.toFloat();
        if((current > 10.0) || (current < -10.0)) Serial << "Overcurrent error" << '\n';
        else
        {
          Serial << "Current = " << current << "A" << '\n';
          odrive.SetCurrent(0, current);
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
      //odrive.GetPosition(0);
      float encoder_position = 2*PI*2*(odrive.GetPosition(0)/4000) - pos_offset; //2cm radio, 2pi = 4000 counts
      Serial << "Linear position: " << encoder_position << "cm" << '\n';
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
      if(isWhitespace(d))
      {
        if(Serial3.available()) d = Serial3.read();
        command = command + char(d);
        while(!isWhitespace(d) && Serial3.available())
        {
          d = Serial3.read();
          command = command + char(d);
        }
        float current = command.toFloat();
        if((current > 10.0) || (current < -10.0)) Serial3 << "OC ERROR";
        else
        {
          Serial3 << "CURRENT ";
          odrive.SetCurrent(0, current);
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
}

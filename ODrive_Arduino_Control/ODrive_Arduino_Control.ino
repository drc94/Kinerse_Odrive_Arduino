
#include <SoftwareSerial.h>
#include "ODriveArduino.h"

// Printing with stream operator
template<class T> inline Print& operator <<(Print &obj,     T arg) { obj.print(arg);    return obj; }
template<>        inline Print& operator <<(Print &obj, float arg) { obj.print(arg, 4); return obj; }

// Serial to the ODrive
SoftwareSerial odrive_serial(8, 9); //RX (ODrive TX), TX (ODrive RX)
// Note: you must also connect GND on ODrive to GND on Arduino!

// ODrive object
ODriveArduino odrive(odrive_serial);

void setup() {
  // ODrive uses 115200 baud
  odrive_serial.begin(115200);

  // Serial to PC
  Serial.begin(115200);
  while (!Serial) ; // wait for Arduino Serial Monitor to open

  Serial.println("ODriveArduino");
  Serial.println("Setting parameters...");

  // In this example we set the same parameters to both motors.
  // You can of course set them different if you want.
  // See the documentation or play around in odrivetool to see the available parameters
  for (int axis = 0; axis < 2; ++axis) {
    odrive_serial << "w axis" << axis << ".controller.config.vel_limit " << 4000.0f << '\n';
    odrive_serial << "w axis" << axis << ".motor.config.current_lim " << 10.0f << '\n';
    // This ends up writing something like "w axis0.motor.config.current_lim 10.0\n"
  }

  Serial.println("Ready!");
  Serial.println("Send the character '0' or '1' to calibrate respective motor (you must do this before you can command movement)");
  Serial.println("Send the character 'l' to apply the loop control");
  Serial.println("Send the character 'c' to enter in current control mode");
  Serial.println("Send the character 's' to stop motor");
  Serial.println("Send the character 'b' to read bus voltage");
  Serial.println("Send the character 'p' to read motor positions in a 10s loop");
}

void loop() {

  if (Serial.available()) {
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

    // Read bus voltage
    if (c == 'b') {
      odrive_serial << "r vbus_voltage\n";
      delay(500);
      Serial << "Vbus voltage: " << odrive.readFloat() << '\n';
    }

    // print motor positions in a 10s loop
    if (c == 'p') {
      static const unsigned long duration = 10000;
      unsigned long start = millis();
      while(millis() - start < duration) {
        for (int motor = 0; motor < 2; ++motor) {
          odrive_serial << "r axis" << motor << ".encoder.pos_estimate\n";
          Serial << odrive.readFloat() << '\t';
        }
        Serial << '\n';
      }
    }
  }
}

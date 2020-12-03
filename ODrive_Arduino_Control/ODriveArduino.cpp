#include "Arduino.h"
#include "ODriveArduino.h"

static const int kMotorOffsetFloat = 2;
static const int kMotorStrideFloat = 28;
static const int kMotorOffsetInt32 = 0;
static const int kMotorStrideInt32 = 4;
static const int kMotorOffsetBool = 0;
static const int kMotorStrideBool = 4;
static const int kMotorOffsetUint16 = 0;
static const int kMotorStrideUint16 = 2;

// Print with stream operator
template<class T> inline Print& operator <<(Print &obj,     T arg) { obj.print(arg);    return obj; }
template<>        inline Print& operator <<(Print &obj, float arg) { obj.print(arg, 4); return obj; }

ODriveArduino::ODriveArduino(Stream& serial)
    : serial_(serial) {}

void ODriveArduino::SetPosition(int motor_number, float position) {
    SetPosition(motor_number, position, 0.0f, 0.0f);
}

void ODriveArduino::SetPosition(int motor_number, float position, float velocity_feedforward) {
    SetPosition(motor_number, position, velocity_feedforward, 0.0f);
}

void ODriveArduino::SetPosition(int motor_number, float position, float velocity_feedforward, float current_feedforward) {
    serial_ << "p " << motor_number  << " " << position << " " << velocity_feedforward << " " << current_feedforward << "\n";
}

void ODriveArduino::SetVelocity(int motor_number, float velocity) {
    SetVelocity(motor_number, velocity, 0.0f);
}

void ODriveArduino::SetVelocity(int motor_number, float velocity, float current_feedforward) {
    serial_ << "v " << motor_number  << " " << velocity << " " << current_feedforward << "\n";
}

void ODriveArduino::SetTorque(int motor_number, float torque) {
    serial_ << "c " << motor_number << " " << torque << "\n";
}

void ODriveArduino::TrapezoidalMove(int motor_number, float position){
    serial_ << "t " << motor_number << " " << position << "\n";
}

float ODriveArduino::readFloat() {
    return readString().toFloat();
}

float ODriveArduino::GetVelocity(int motor_number){
  serial_<< "r axis" << motor_number << ".encoder.vel_estimate\n";
  return ODriveArduino::readFloat();
}

float ODriveArduino::GetPosition(int motor_number){
  serial_<< "r axis" << motor_number << ".encoder.pos_estimate\n";
  return ODriveArduino::readFloat();
}

int32_t ODriveArduino::readInt() {
    return readString().toInt();
}

bool ODriveArduino::run_state(int axis, int requested_state, bool wait) {
    int timeout_ctr = 100;
    serial_ << "w axis" << axis << ".requested_state " << requested_state << '\n';
    if (wait) {
        do {
            delay(100);
            serial_ << "r axis" << axis << ".current_state\n";
        } while (readInt() != AXIS_STATE_IDLE && --timeout_ctr > 0);
    }
    return timeout_ctr > 0;
}

String ODriveArduino::readString() {
    String str = "";
    static const unsigned long timeout = 1000;
    unsigned long timeout_start = millis();
    for (;;) {
        while (!serial_.available()) {
            if (millis() - timeout_start >= timeout) {
                return str;
            }
        }
        char c = serial_.read();
        if (c == '\n')
            break;
        str += c;
    }
    return str;
}

void initCalibration(ODriveArduino* odrive){
   int requested_state = ODriveArduino::AXIS_STATE_MOTOR_CALIBRATION;
   odrive->run_state(0, requested_state, true);
   odrive->run_state(1, requested_state, true);

   requested_state = ODriveArduino::AXIS_STATE_ENCODER_OFFSET_CALIBRATION;
   odrive->run_state(0, requested_state, true);
   odrive->run_state(1, requested_state, true);
/*
   requested_state = ODriveArduino::AXIS_STATE_CLOSED_LOOP_CONTROL;  // No es necesario para la calibración
   odrive.run_state(0, requested_state, false); // don't wait
   odrive.run_state(1, requested_state, false); // don't wait*/
}

float initPosition(ODriveArduino* odrive, int motor){
  float pos_offset = 0;
  int requested_state = ODriveArduino::AXIS_STATE_CLOSED_LOOP_CONTROL;
  odrive->run_state(motor, requested_state, false); // don't wait
  if(motor == 0)
  {
    odrive->SetVelocity(motor, -3.0, -1.0);
    delay(500);
    while(odrive->GetVelocity(motor) < -0.1);
    pos_offset = 2*PI*2*(odrive->GetPosition(motor));  
    delay(200);
    odrive->SetTorque(motor, -0.166);
    delay(500);
    odrive->SetTorque(motor, -0.083);
    delay(500);
    odrive->SetTorque(motor, -0.0415);
    delay(500);
    odrive->SetTorque(motor, -0.0);
  }
  else
  {
    odrive->SetVelocity(motor, 3.0, 1.0);
    delay(500);
    while(odrive->GetVelocity(motor) > 0.1);
    pos_offset = 2*PI*2*(odrive->GetPosition(motor));  
    delay(200);
    odrive->SetTorque(motor, 0.166);
    delay(500);
    odrive->SetTorque(motor, 0.083);
    delay(500);
    odrive->SetTorque(motor, 0.0415);
    delay(500);
    odrive->SetTorque(motor, 0.0);
  }
  //delay(200);
  //odrive->SetVelocity(motor, 2.0, 0.0);
  delay(200);
  requested_state = ODriveArduino::AXIS_STATE_IDLE;
  odrive->run_state(motor, requested_state, false);
  return pos_offset;
}

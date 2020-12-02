float torqueHapticsVibration(float linearPosition, float lastTorque, int mode);
float torqueHapticsBox(float linearPosition, int motorNum);
float torqueControlValue(float linearPosition, float torque);
float torqueFriend(float linearPosition, float initPos, float finalPos, float velocity, float torqueSetpoint, int motorNum);

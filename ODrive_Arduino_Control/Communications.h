#include "ODriveArduino.h"

int serialCOM(ODriveArduino* odrive, int* motorMode, float* current, float* posOffset, float* linearPosition);
int serialBT(ODriveArduino* odrive, int* motorMode, float* current, float* posOffset, float* linearPosition);
void initBT();

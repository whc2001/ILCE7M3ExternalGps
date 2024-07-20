#ifndef __SERIAL_DEBUG_H_
#define __SERIAL_DEBUG_H_

#ifdef DEBUG_BT_SERIAL
    #include "BluetoothSerial.h"
    #define SerialDebug SerialBT
#else
    #define SerialDebug Serial
#endif

#ifdef DEBUG_BT_SERIAL
BluetoothSerial SerialBT;
#endif

#endif
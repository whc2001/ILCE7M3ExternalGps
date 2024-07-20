#define BLE_DEVICE_ADDRESS "48:eb:62:e2:a8:10"  // Your camera's bluetooth MAC
#define GPS_MODULE_BAUDRATE 9600    // GPS module baudrate
#define GPS_MODULE_TX_PIN 14    // Connect to GPS module's RX
#define GPS_MODULE_RX_PIN 12    // Connect to GPS module's TX
//#define DEBUG_BT_SERIAL   // Uncomment to enable bluetooth serial, comment to enable UART0 (USB on dev board)

#include "CameraBle.h"
#include "SerialDebug.h"
#include <TinyGPS++.h>
//#include "BLEScan.h"

TinyGPSPlus nmea;
TinyGPSCustom isFix(nmea, "GNRMC", 2);

bool scanning = true;
BLEAdvertisedDevice *deviceFound = NULL;
bool connected = false;

bool isSendTzDstOffset = false;
unsigned long lastTransmitPacket = 0;
unsigned long lastLatitudeMul = 0, lastLongitudeMul = 0;
unsigned int lastYear = 0;
unsigned char lastMonth = 0, lastDay = 0, lastHour = 0, lastMinute = 0, lastSecond = 0;

void OnScanDone(bool isSuccess, BLEAdvertisedDevice *device)
{
    SerialDebug.print(F("Scan finished. Result="));
    SerialDebug.println(isSuccess);

    deviceFound = device;
    scanning = false;
}

void OnDisconnected()
{
    SerialDebug.println(F("Camera disconnected"));
    connected = false;
}

void setup()
{

#ifdef DEBUG_BT_SERIAL
    SerialDebug.begin("ILCE7M3ExternalGps");
#else
    SerialDebug.begin(115200);
#endif
    Serial1.begin(GPS_MODULE_BAUDRATE, SERIAL_8N1, GPS_MODULE_RX_PIN, GPS_MODULE_TX_PIN);

    SerialDebug.println(F("Init"));
    BLE_Init("ILCE7M3ExternalGps");
    BLE_SetDisconnectedCallback(OnDisconnected);
    Serial1.begin(GPS_MODULE_BAUDRATE, SERIAL_8N1, GPS_MODULE_RX_PIN, GPS_MODULE_TX_PIN);
    SerialDebug.println(F("Init OK"));

    SerialDebug.println(F("Scanning camera"));
    BLE_BeginScanning(BLEAddress(BLE_DEVICE_ADDRESS), OnScanDone);
}

void loop()
{
    if(Serial1.available())
        nmea.encode(Serial1.read());

    if(!scanning && deviceFound == NULL)
    {
        SerialDebug.println(F("Scanning camera"));
        scanning = true;
        BLE_BeginScanning(BLEAddress(BLE_DEVICE_ADDRESS), OnScanDone);
    }

    if(deviceFound != NULL && !connected)
    {
        SerialDebug.println(F("Connecting camera"));
        unsigned char connectResult = BLE_ConnectCamera(deviceFound);
        switch(connectResult)
        {
            case BLE_CONNECT_RESULT_SUCCESS:
                isSendTzDstOffset = ReadIsSendTzDst();
                SerialDebug.println(F("Connected!"));
                SerialDebug.print(F("Need to send timezone and DST offset: "));
                SerialDebug.println(isSendTzDstOffset);
                connected = true;
                break;
            case BLE_CONNECT_RESULT_DEVICE_ERROR:
                SerialDebug.println(F("Connect failed: Bluetooth connection problem"));
                break;
            case BLE_CONNECT_RESULT_CHARACTERISTICS_ERROR:
                SerialDebug.println(F("Connect failed: Unable to find services or characteristics"));
                break;
            case BLE_CONNECT_RESULT_AUTH_ERROR:
                SerialDebug.println(F("Connect failed: Unable to finish authentication"));
                break;
        }
        delay(50);
    }

    if(isFix.isValid() && isFix.value()[0] == 'A' && nmea.location.isValid() && nmea.date.isValid() && nmea.time.isValid())
    {
        double latitude = nmea.location.lat();
        double longitude = nmea.location.lng();
        unsigned long latitudeMul = latitude * 10000000;
        unsigned long longitudeMul = longitude * 10000000;
        unsigned int year = nmea.date.year();
        unsigned char month = nmea.date.month();
        unsigned char day = nmea.date.day();
        unsigned char hour = nmea.time.hour();
        unsigned char minute = nmea.time.minute();
        unsigned char second = nmea.time.second();

        if(latitudeMul != lastLatitudeMul || longitudeMul != lastLongitudeMul || year != lastYear || month != lastMonth || day != lastDay || hour != lastHour || minute != lastMinute || second != lastSecond)
        {
            lastLatitudeMul = latitudeMul;
            lastLongitudeMul = longitudeMul;
            lastYear = year;
            lastMonth = month;
            lastDay = day;
            lastHour = hour;
            lastMinute = minute;
            lastSecond = second;

            SerialDebug.print(F("GPS Packet Update: Lat="));
            SerialDebug.print(latitude, 6);
            SerialDebug.print(F(" Lon="));
            SerialDebug.print(longitude, 6);
            SerialDebug.print(F(" Speed="));
            SerialDebug.print(nmea.speed.kmph(), 2);

            SerialDebug.print(F("  Now="));
            SerialDebug.print(year);
            SerialDebug.print(F("-"));
            SerialDebug.print(month < 10 ? F("0") : F(""));
            SerialDebug.print(month);
            SerialDebug.print(F("-"));
            SerialDebug.print(day < 10 ? F("0") : F(""));
            SerialDebug.print(day);
            SerialDebug.print(F(" "));
            SerialDebug.print(hour < 10 ? F("0") : F(""));
            SerialDebug.print(hour);
            SerialDebug.print(F(":"));
            SerialDebug.print(minute < 10 ? F("0") : F(""));
            SerialDebug.print(minute);
            SerialDebug.print(F(":"));
            SerialDebug.print(second < 10 ? F("0") : F(""));
            SerialDebug.print(second);

            SerialDebug.println();
        }

        if(connected && millis() - lastTransmitPacket > BLE_LOCATION_PACKET_TRANSMIT_DURATION)
        {
            WriteLocationPayload(latitudeMul, longitudeMul, year, month, day, hour, minute, second, isSendTzDstOffset);
            lastTransmitPacket = millis();
        }
    }

    delay(0);
}

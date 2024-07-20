#ifndef __CAMERA_BLE_H_
#define __CAMERA_BLE_H

#include "SerialDebug.h"
#include "BLEDevice.h"
#include "esp_gap_bt_api.h"

#define BLE_AUTHENTICATE_TIMEOUT 30000             // How long to wait for bluetooth authentication before try again, no need to modify normally.
#define BLE_LOCATION_PACKET_TRANSMIT_DURATION 1000 // How long to send one packet to camera, no need to modify normally.

// ********** BLE Device Scanning **********
static BLEScan *bleScanner;

static BLEAddress scanTargetAddress("00:00:00:00:00:00");
static bool isScanning = false;

void (*scanFinishedCallback)(bool isFound, BLEAdvertisedDevice *device);
// ********************

// ********** BLE Connection and Auth **********
#define BLE_STATE_NOT_CONNECTED 0
#define BLE_STATE_CONNECTING 1
#define BLE_STATE_CONNECT_TIMEOUT 2
#define BLE_STATE_WAITING_CHARACTERISTICS 3
#define BLE_STATE_CHARACTERISTICS_FAILED 4
#define BLE_STATE_WAITING_AUTH 5
#define BLE_STATE_AUTH_FAILED 6
#define BLE_STATE_CONNECTED 7

#define BLE_CONNECT_RESULT_UNDETERMINED 0
#define BLE_CONNECT_RESULT_SUCCESS 1
#define BLE_CONNECT_RESULT_DEVICE_ERROR 2
#define BLE_CONNECT_RESULT_CHARACTERISTICS_ERROR 3
#define BLE_CONNECT_RESULT_AUTH_ERROR 4

static const BLEUUID locationServiceUuid("8000dd00-dd00-ffff-ffff-ffffffffffff");
static const BLEUUID locationCharaReadUuid("0000dd21-0000-1000-8000-00805f9b34fb");
static const BLEUUID locationCharaWriteUuid("0000dd11-0000-1000-8000-00805f9b34fb");

static BLEClient *cameraConnection;
static BLERemoteCharacteristic *locationCharaRead;
static BLERemoteCharacteristic *locationCharaWrite;

static unsigned char connectState = BLE_STATE_NOT_CONNECTED;
static unsigned char connectResult = BLE_CONNECT_RESULT_UNDETERMINED;
static bool isAuthenticating = false;

void (*cameraDisconnectedCallback)(void);
// ********************

// ********** BLE Transaction **********
unsigned char sendPayload[] = {
    0x00, 0x5D,                                                                                     // Length
    0x08, 0x02, 0xfc, 0x03, 0x00, 0x00, 0x10, 0x10, 0x10,                                           // Fixed data
    0x00, 0x00, 0x00, 0x00,                                                                         // Lat
    0x00, 0x00, 0x00, 0x00,                                                                         // Lon
    0x00, 0x00,                                                                                     // UTC Year
    0x00,                                                                                           // UTC Month
    0x00,                                                                                           // UTC Day
    0x00,                                                                                           // UTC Hour
    0x00,                                                                                           // UTC Minute
    0x00,                                                                                           // UTC Second
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Zeros
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,
    0x00, 0x00, // Time zone offset
    0x00, 0x00  // DST offset
};

// ********** BLE Device Scanning **********

// BLE device timeout handler
static void _OnBleScanComplete(BLEScanResults scanResults)
{
    if (isScanning)
        isScanning = false;
    scanFinishedCallback(false, NULL);
}

// BLE device found handler
class _BleAdvertisedDeviceHandler : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        if (advertisedDevice.getAddress().equals(scanTargetAddress))
        {
            bleScanner->stop();
            scanFinishedCallback(true, new BLEAdvertisedDevice(advertisedDevice));
        }
    }
};

bool BLE_BeginScanning(BLEAddress targetAddress, void (*onScanningFinished)(bool, BLEAdvertisedDevice*))
{
    if (isScanning || connectState != BLE_STATE_NOT_CONNECTED)
    {
        SerialDebug.println("[BLEScan] Already scanning or connected to device!");
        return false;
    }
    scanTargetAddress = targetAddress;
    scanFinishedCallback = onScanningFinished;
    bleScanner = BLEDevice::getScan();
    bleScanner->setAdvertisedDeviceCallbacks(new _BleAdvertisedDeviceHandler());
    bleScanner->setInterval(1000);
    bleScanner->setWindow(500);
    bleScanner->setActiveScan(true);
    isScanning = true;
    bleScanner->start(60, _OnBleScanComplete, false);
    return true;
}

// ********************

// ********** BLE Transaction **********

unsigned int ReadLocationParam(char **buf)
{
    if(locationCharaRead->canRead())
    {
        std::string ret = locationCharaRead->readValue();
        if(ret.size() == 0)
            return 0;
        unsigned int len = ret.size() - 1;
        *buf = (char*)malloc(sizeof(char) * len);
        memcpy(*buf, ret.data(), len);
        return len;
    }
    else
        SerialDebug.println("Cannot read location param");
    return 0;
}

bool ReadIsSendTzDst()
{
    char *buf;
    unsigned int readLen = ReadLocationParam(&buf);
    bool ret = ((readLen == 6) && ((buf[4] & 0x02) == 0x02));
    free(buf);
    return ret;
}

void WriteLocationPayload(unsigned long latMul, unsigned long lonMul, unsigned int year, unsigned char month, unsigned char day, unsigned char hour, unsigned char minute, unsigned char second, bool isSendTzDstOffset)
{
    // Length
    sendPayload[1] = isSendTzDstOffset ? 0x5D : 0x59;

    // Latitude
    sendPayload[11] = (latMul & 0xFF000000) >> 24;
    sendPayload[12] = (latMul & 0x00FF0000) >> 16;
    sendPayload[13] = (latMul & 0x0000FF00) >> 8;
    sendPayload[14] = (latMul & 0x000000FF);

    // Longitude
    sendPayload[15] = (lonMul & 0xFF000000) >> 24;
    sendPayload[16] = (lonMul & 0x00FF0000) >> 16;
    sendPayload[17] = (lonMul & 0x0000FF00) >> 8;
    sendPayload[18] = (lonMul & 0x000000FF);

    // Time
    sendPayload[19] = (year & 0xFF00) >> 8;
    sendPayload[20] = (year & 0x00FF);
    sendPayload[21] = month;
    sendPayload[22] = day;
    sendPayload[23] = hour;
    sendPayload[24] = minute;
    sendPayload[25] = second;

    locationCharaWrite->writeValue(sendPayload, sendPayload[1], true);
}

// ********************

// ********** BLE Connection **********

/*
void BLE_DeletePairing()
{
    if (esp_ble_remove_bond_device((uint8_t *)cameraMacAddress.getNative()) == ESP_OK)
    {
        SerialDebug.println(F("Delete pairing status success"));
    }
    else
    {
        SerialDebug.println(F("Delete pairing status failed"));
    }
}
*/

// BLE device connected and disconnected callback
class _BleConnectionHandler : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {

    }

    void onDisconnect(BLEClient *pclient)
    {
        if (connectState == BLE_STATE_CONNECTED)
            SerialDebug.println(F("Connection lost"));
        connectState = BLE_STATE_NOT_CONNECTED;
        cameraDisconnectedCallback();
    }
};

// BLE auth callback
class _BleSecurityHandler : public BLESecurityCallbacks
{

    uint32_t onPassKeyRequest()
    {
        return 0;
    }

    void onPassKeyNotify(uint32_t pass_key)
    {
    }
    bool onConfirmPIN(uint32_t pass_key)
    {
        return true;
    }
    bool onSecurityRequest()
    {
        return true;
    }
    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl)
    {
        connectState = auth_cmpl.success ? BLE_STATE_CONNECTED : BLE_STATE_AUTH_FAILED;
    }
};

void _SetBleSecurity()
{
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
    BLEDevice::setSecurityCallbacks(new _BleSecurityHandler());
    BLESecurity *bleSecurity = new BLESecurity();
    bleSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
    bleSecurity->setCapability(ESP_IO_CAP_NONE);
    bleSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
}

void _ConnectCamera(BLEAdvertisedDevice *device)
{
    connectState = BLE_STATE_CONNECTING;
    cameraConnection = BLEDevice::createClient();
    cameraConnection->setClientCallbacks(new _BleConnectionHandler());
    cameraConnection->connect(device);
    if(cameraConnection->isConnected())
        connectState = BLE_STATE_WAITING_CHARACTERISTICS;
    else
        connectState = BLE_STATE_CONNECT_TIMEOUT;
}

void _InitServicesAndCharacteristics()
{
    if (connectState != BLE_STATE_WAITING_CHARACTERISTICS)
    {
        SerialDebug.println(F("[BLEConnect InitServicesAndCharacteristics] Not connected or already done characteristics!"));
        return;
    }

    // Get location service
    BLERemoteService *locationService = cameraConnection->getService(locationServiceUuid);
    if (locationService == nullptr)
    {
        SerialDebug.println(F("ERROR: No specified service"));
        connectState = BLE_STATE_CHARACTERISTICS_FAILED;
        return;
    }

    // Get location read parameter characteristic
    locationCharaRead = locationService->getCharacteristic(locationCharaReadUuid);
    if (locationCharaRead == nullptr)
    {
        SerialDebug.println(F("ERROR: No specified characteristic"));
        connectState = BLE_STATE_CHARACTERISTICS_FAILED;
        return;
    }

    // Get location write coordinate characteristic
    locationCharaWrite = locationService->getCharacteristic(locationCharaWriteUuid);
    if (locationCharaWrite == nullptr)
    {
        SerialDebug.println(F("ERROR: No specified characteristic"));
        connectState = BLE_STATE_CHARACTERISTICS_FAILED;
        return;
    }

    connectState = BLE_STATE_WAITING_AUTH;
}

void _DoBleSecurityAttempt()
{
    if (connectState != BLE_STATE_WAITING_AUTH)
    {
        SerialDebug.println(F("[BLEConnect DoBleSecurityAttempt] Not connected or already authed!"));
        return;
    }

    unsigned long beginTimestamp = millis();
    unsigned long lastPrint = beginTimestamp;
    unsigned long now = 0;
    char *buf = NULL;
    isAuthenticating = true;
    while (1)
    {
        now = millis();

        if (ReadLocationParam(&buf) > 0) // if something is read then no need to auth
        {
            isAuthenticating = false;
            connectState = BLE_STATE_CONNECTED;
            if (buf)
                free(buf);
            return;
        }

        if (connectState == BLE_STATE_CONNECTED) // auth callback returns true
        {
            isAuthenticating = false;
            if (buf)
                free(buf);
            return;
        }

        if (now - lastPrint > 1000)
        {
            lastPrint = now;
            SerialDebug.println(F("Please go to Bluetooth settings and pair the device"));
        }

        if(now - beginTimestamp >= BLE_AUTHENTICATE_TIMEOUT)    // Timeout
        {
            connectState = BLE_STATE_AUTH_FAILED;
            if (buf)
                free(buf);
            return;
        }
        
        if (connectState == BLE_STATE_NOT_CONNECTED) // If got kicked
        {
            if (buf)
                free(buf);
            return;
        }

        delay(50);
    }
}

void BLE_Init(std::string deviceName)
{
    BLEDevice::init(deviceName);
    _SetBleSecurity();
}

void BLE_SetDisconnectedCallback(void (*callback)(void))
{
    cameraDisconnectedCallback = callback;
}

unsigned char BLE_ConnectCamera(BLEAdvertisedDevice *device)
{
    connectResult = BLE_CONNECT_RESULT_UNDETERMINED;
    while (connectResult == BLE_CONNECT_RESULT_UNDETERMINED)
    {
        switch (connectState)
        {
            case BLE_STATE_NOT_CONNECTED:
                if(isAuthenticating)
                {
                    isAuthenticating = false;
                    connectResult = BLE_CONNECT_RESULT_AUTH_ERROR;
                    break;
                }
                SerialDebug.println(F("[BLEConnect ConnectCamera] Connecting to camera..."));
                _ConnectCamera(device);
                break;
            case BLE_STATE_CONNECT_TIMEOUT:
                SerialDebug.println(F("[BLEConnect ConnectCamera] Connect Timeout"));
                connectState = BLE_STATE_NOT_CONNECTED;
                connectResult = BLE_CONNECT_RESULT_DEVICE_ERROR;
                break;
            case BLE_STATE_WAITING_CHARACTERISTICS:
                SerialDebug.println(F("[BLEConnect ConnectCamera] Initializing characteristics..."));
                _InitServicesAndCharacteristics();
                break;
            case BLE_STATE_CHARACTERISTICS_FAILED:
                SerialDebug.println(F("[BLEConnect ConnectCamera] Characteristics FAILED..."));
                cameraConnection->disconnect();
                connectResult = BLE_CONNECT_RESULT_CHARACTERISTICS_ERROR;
                break;
            case BLE_STATE_WAITING_AUTH:
                SerialDebug.println(F("[BLEConnect ConnectCamera] Authenticating..."));
                _DoBleSecurityAttempt();
                break;
            case BLE_STATE_AUTH_FAILED:
                SerialDebug.println(F("[BLEConnect ConnectCamera] Authentication FAILED..."));
                connectResult = BLE_CONNECT_RESULT_AUTH_ERROR;
                break;
            case BLE_STATE_CONNECTED:
                SerialDebug.println(F("[BLEConnect ConnectCamera] SUCCESSFULLY connected!"));
                connectResult = BLE_CONNECT_RESULT_SUCCESS;
                break;
        }
        delay(1);   // DoEvents
    }
    return connectResult;
}

// ********************

#endif

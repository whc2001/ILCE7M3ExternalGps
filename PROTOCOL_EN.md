# 1 Scan Response Manufacturer Data

Must match the first four bytes to identify Sony camera devices.

Status data is grouped in zero or more 3-byte groups, where the first byte is the status type (`0x00` indicates the end), the second byte is a bitmap of status values, and the third byte seems currently unused. See the table below for details.

```
2d01 0300 6400 4531 22eb00 214000 000000000000000003030018
```

|Offset|Content|Remark|
|-|-|-|
|[0:1]|Manufacturer ID|`0x012D` = Sony|
|[2:3]|Device Type ID|`0x0003` = Camera|
|[4:5]|BLE Protocol Version||
|[6:7]|Model Code|ASCII (UTF-8) encoded "E1"|
|[8:10]|Status Data Type `0x22`|Supports Pairing (b7=1), Paired (b6=1), Supports Location Sync (b5=1), Location Sync Disabled (b4=0)|
|[11:13]|Status Data Type `0x21`|Remote Power On Disabled (b7=0), Camera Currently Powered On (b6=1), Does Not Support WifiHandover (b5=0 b4=0)|
|[14:]|Other Data|Unknown for now, possibly unused|

## 1.1 Status Data Type List
|Type|Content|
|-|-|
|`0x21`|**b7** Remote Power On Enabled<br>**b6** Camera Powered On<br>**b5** Supports WifiHandover<br>**b4** WifiHandover Enabled<br>**b3 ~ b0** Unknown |
|`0x22`|**b7** Supports Pairing<br>**b6** Paired<br>**b5** Supports Location Sync<br>**b4** Location Sync Enabled<br>**b3 ~ b0** Unknown |
|`0x23`|**b7** Supports Mobile Remote Control<br>**b6 ~ b5** Mobile Remote Control Enabled (b6=0 b5=1 means enabled)<br>**b4** Supports Mobile Image Transfer<br>**b3 ~ b2** Mobile Image Transfer Enabled (b3=0 b2=1 means enabled)<br>**b1** Supports Push Notifications<br>**b0** Push Notifications Enabled |

# 2 GATT Service

## 2.1 Pairing

Pairing Service UUID: `8000EE00-EE00-FFFF-FFFF-FFFFFFFFFFFF`

  - Write Characteristic UUID: `0xEE01`

Send `0x06 0x08 0x01 0x00 0x00 0x00 0x00` to initiate pairing (must be sent in pairing state, otherwise the camera will disconnect)

## 2.2 Location Service

Location Service UUID: `8000DD00-DD00-FFFF-FFFF-FFFFFFFFFFFF`

  - Notify Status Characteristic UUID: `0xDD01`
  - Write Location Data Characteristic UUID: `0xDD11`
  - Read Configuration Characteristic UUID: `0xDD21`
  - \* Write Characteristic for Lock Location Endpoint UUID: `0xDD30`
  - \* Write Characteristic for Enable Location Update UUID: `0xDD31`

**(Characteristics marked by \* are only supported in firmware version 3.02 and above (protocol version >= 65 in scan response), lower versions do not have these characteristics and can ignore those procedures)**

### 2.2.1 Status Notification

|Payload|Function|
|-|-|
|`0x03 0x01 0x02 0x01`|Location Sync Enabled from Camera Side|
|`0x03 0x01 0x02 0x00`|Location Sync Disabled from Camera Side|

### 2.2.2 Read Configuration
```
0610009c 02 00
```

|Offset|Content|Remark|
|-|-|-|
|[0:3]|Unknown||
|[4]|Some sort of flag [**If bit 2 is 1 then timezone offset and DST offset must be provided when writting coordinate data**]|0x02 & 0x02 = 1, Timezone and DST offset data are required|
|[5]|Unknown||

### 2.2.3 \* Write Lock Location Endpoint

Newer firmware requires writing `0x01` to characteristic `0xDD30` before writing coordinate data, and it is recommended to write `0x00` before disconnecting.

### 2.2.4 \* Write Enable Location Update

Newer firmware requires writing `0x01` to characteristic `0xDD31` before writing coordinate data, and it is recommended to write `0x00` before disconnecting.

### 2.2.5 Write Location Data

**Newer firmware must perform the above two steps 2.2.3 and 2.2.4 in order**

```
005d 0802fc 03 0000101010 0bf79e5e 41c385a7 07e40b0504022a 0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 01e0 0000
```

\* All multi-byte data are in **Big Endian**.

|Offset|Content|Remark|
|-|-|-|
|[0:1]|Payload Length (exclude these two bytes)|`0x5D` = 93 bytes|
|[2:4]|Fixed Data|`0x0802FC`|
|[5]|Flag of transmitting timezone offset and DST offset (0x03 for transmit and 0x00 for do not transmit)|`0x03` Timezone offset and DST offset required|
|[6:10]|Fixed Data|`0x0000101010`|
|[11:14]|Latitude (multiplied by 10000000)|`0x0BF79E5E` = 200777310 / 10000000 = 20.077731|
|[15:18]|Longitude (multiplied by 10000000)|`0x41C385A7` = 1103332775 / 10000000 = 110.3332775|
|[19:20]|UTC Year|`0x07E4` = 2020|
|[21]|UTC Month|`0x0B` = 11|
|[22]|UTC Day|`0x05` = 5|
|[23]|UTC Hour|`0x04` = 4|
|[24]|UTC Minute|`0x02` = 2|
|[25]|UTC Second|`0x2A` = 42|
|[26:90]|Zeros|`0x00`|
|\*[91:92]|Difference from UTC to current timezone in minutes| `0x01E0` = 480 min = 8 h (UTC+8)|
|\*[93:94]|Difference for DST in current timezone in minutes, generally 60 if DST is in effect|`0x00` (DST is not available in China)|

(Fields marked by \* is required only when bit 2 of byte 4 of configuration data read is 1, otherwise omitted and the packet length shortens to 89)

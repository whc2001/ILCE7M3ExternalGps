## GATT Service

Location Service UUID: 8000DD00-DD00-FFFF-FFFF-FFFFFFFFFFFF

  - Write Characteristic UUID: 0xDD11
  - Read Characteristic UUID: 0xDD21
  
## BLE Read Configuration Packet Example
    06 10 00 9C 02 00

|Offset|Content|Remark|
|-|-|-|
|[0:3]|Unknown||
|[4]|Some sort of flag [**If bit 2 is 1 then timezone and DST offset must be provided when writting coordinate data**]|0x02 & 0x02 = 1, Timezone and DST offset data are required|
|[5]|Unknown||

## BLE Write Coordinate Packet Example

    005d 0802fc030000101010 0bf79e5e 41c385a7 07e40b0504022a 0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 01e0 0000

* All multi-byte data are in **Big Endian**.

|Offset|Content|Remark|
|-|-|-|
|[0:1]|Payload Length (exclude these two bytes)|0x5D = 93 bytes|
|[2:10]|Fixed Data|0x0802FC030000101010|
|[11:14]|Latitude (multiplied by 10000000)|0x0BF79E5E = 200777310 / 10000000 = 20.077731|
|[15:18]|Longitude (multiplied by 10000000)|0x41C385A7 = 1103332775 / 10000000 = 110.3332775|
|[19:20]|UTC Year|0x07E4 = 2020|
|[21]|UTC Month|0x0B = 11|
|[22]|UTC Day|0x05 = 5|
|[23]|UTC Hour|0x04 = 4|
|[24]|UTC Minute|0x02 = 2|
|[25]|UTC Second|0x2A = 42|
|[26:90]|Zeros|0x00|
|*[91:92]|Difference between UTC and current timezone in minutes| 0x01E0 = 480min = 8h (UTC+8)|
|*[93:94]|Difference for DST in current timezone in minutes|0 (DST is not available in China)|

(Fields marked by \* is required only when bit 2 of byte 4 of configuration data read is 1.)

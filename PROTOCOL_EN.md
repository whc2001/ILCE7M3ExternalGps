## BLE Location Packet Example

    005d 0802fc030000101010 0bf79e5e 41c385a7 07e40b0504022a 0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000 01e0 0000

## Packet Structure Details
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

(Fields marked by \* is optional, may be absent if timezone is not set or something...)
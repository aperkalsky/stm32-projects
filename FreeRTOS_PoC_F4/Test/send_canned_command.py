import serial

# get FW version command
byte_list = [0x01, 0x00, 0x00, 0x00, 0x00, 0xD4, 0xDC, 0x09, 0x20]

packed_bytes = bytes(byte_list)

ser = serial.Serial(
    port="COM3",
    baudrate=115200,
    timeout=20
)

ser.write(packed_bytes)

print("data sent")

response = ser.read(13)

print("data read")
print(response)

ser.close()


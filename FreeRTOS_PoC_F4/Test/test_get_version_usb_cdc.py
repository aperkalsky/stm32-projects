import serial
import struct
#import zlib

CMD_GET_VERSION = 0x01
RESP_VERSION   = 0x82

POLY = 0x04C11DB7

def stm32_crc32(data):
    crc = 0xFFFFFFFF

    padded = bytearray(data)

    while len(padded) % 4:
        padded.append(0)

    for i in range(0, len(padded), 4):

        word = (
            padded[i]
            | (padded[i+1] << 8)
            | (padded[i+2] << 16)
            | (padded[i+3] << 24)
        )

        crc ^= word

        for _ in range(32):
            if crc & 0x80000000:
                crc = ((crc << 1) ^ POLY) & 0xFFFFFFFF
            else:
                crc = (crc << 1) & 0xFFFFFFFF

    return crc

ser = serial.Serial(
    port="COM3",
    baudrate=115200,
    timeout=20)

seq = 1
payload = b''

packet = struct.pack(
    "<BHH",
    CMD_GET_VERSION,
    len(payload),
    seq)

crc = stm32_crc32(packet) & 0xFFFFFFFF

packet += struct.pack("<I", crc)

ser.write(packet)

#
# Read header
#
hdr = ser.read(5)

if len(hdr) != 5:
    print("Timeout waiting for response")
    exit()

resp_type, length, resp_seq = struct.unpack(
    "<BHH",
    hdr)

payload = ser.read(length)

crc_bytes = ser.read(4)

rx_crc = struct.unpack(
    "<I",
    crc_bytes)[0]

calc_crc = stm32_crc32(
    hdr + payload) & 0xFFFFFFFF

print(f"Type=0x{resp_type:02X}")
print(f"Length={length}")
print(f"Seq={resp_seq}")

if rx_crc != calc_crc:
    print("CRC ERROR")
    exit()

if resp_type == RESP_VERSION:
    version = payload.decode(
        "ascii",
        errors="ignore")

    print("Version:", version)

ser.close()